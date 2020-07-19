#include "neato.h"
#include "sgd.h"
#include "dijkstra.h"
#include "randomkit.h"
#include "neatoprocs.h"
#include <math.h>
#include <stdlib.h>


static float calculate_stress(float *pos, term_sgd *terms, int n_terms) {
    float stress = 0;
    int ij;
    for (ij=0; ij<n_terms; ij++) {
        float dx = pos[2*terms[ij].i] - pos[2*terms[ij].j];
        float dy = pos[2*terms[ij].i+1] - pos[2*terms[ij].j+1];
        float r = sqrt(dx*dx + dy*dy) - terms[ij].d;
        stress += terms[ij].w * (r * r);
    }
    return stress;
}
// it is much faster to shuffle term rather than pointers to term, even though the swap is more expensive
static rk_state rstate;
static void fisheryates_shuffle(term_sgd *terms, int n_terms) {
    int i;
    for (i=n_terms-1; i>=1; i--) {
        // srand48() is called in neatoinit.c, so no need to seed here
        //int j = (int)(drand48() * (i+1));
        int j = rk_interval(i, &rstate);

        term_sgd temp = terms[i];
        terms[i] = terms[j];
        terms[j] = temp;
    }
}

// graph_sgd data structure exists only to make dijkstras faster
static graph_sgd * extract_adjacency(graph_t *G, int model) {
    node_t *np;
    edge_t *ep;
    int n_nodes = 0, n_edges = 0;
    for (np = agfstnode(G); np; np = agnxtnode(G,np)) {
        assert(ND_id(np) == n_nodes);
        n_nodes++;
        for (ep = agfstedge(G, np); ep; ep = agnxtedge(G, ep, np)) {
            if (agtail(ep) != aghead(ep)) { // ignore self-loops and double edges
                n_edges++;
            }
        }
    }
    graph_sgd *graph = N_NEW(1, graph_sgd);
    graph->sources = N_NEW(n_nodes+1, int);
    graph->pinneds = N_NEW(n_nodes, bool);
    graph->targets = N_NEW(n_edges, int);
    graph->weights = N_NEW(n_edges, float);

    graph->n = n_nodes;
    graph->sources[graph->n] = n_edges; // to make looping nice

    n_nodes = 0, n_edges = 0;
    for (np = agfstnode(G); np; np = agnxtnode(G,np)) {
        graph->sources[n_nodes] = n_edges;
        graph->pinneds[n_nodes] = isFixed(np);
        for (ep = agfstedge(G, np); ep; ep = agnxtedge(G, ep, np)) {
            if (agtail(ep) == aghead(ep)) { // ignore self-loops and double edges
                continue;
            }
            node_t *target = (agtail(ep) == np) ? aghead(ep) : agtail(ep); // in case edge is reversed
            graph->targets[n_edges] = ND_id(target);
            graph->weights[n_edges] = ED_dist(ep);
            assert(graph->weights[n_edges] > 0);
            n_edges++;
        }
        n_nodes++;
    }
    assert(n_nodes == graph->n);
    assert(n_edges == graph->sources[graph->n]);
    graph->sources[n_nodes] = n_edges;

    if (model == MODEL_SHORTPATH) {
        // do nothing
    } else if (model == MODEL_SUBSET) {
        // i,j,k refer to actual node indices, while x,y refer to edge indices in graph->targets
        int i;
        bool *neighbours_i = N_NEW(graph->n, bool);
        bool *neighbours_j = N_NEW(graph->n, bool);
        for (i=0; i<graph->n; i++) {
            // initialise to no neighbours
            neighbours_i[i] = false;
            neighbours_j[i] = false;
        }
        for (i=0; i<graph->n; i++) {
            int x;
            int deg_i = 0;
            for (x=graph->sources[i]; x<graph->sources[i+1]; x++) {
                int j = graph->targets[x];
                if (neighbours_i[j] == false) { // ignore multiedges
                    neighbours_i[j] = true; // set up sort of hashset
                    deg_i++;
                }
            }
            for (x=graph->sources[i]; x<graph->sources[i+1]; x++) {
                int j = graph->targets[x];
                int y, intersect = 0;
                int deg_j = 0;
                for (y=graph->sources[j]; y<graph->sources[j+1]; y++) {
                    int k = graph->targets[y];
                    if (neighbours_j[k] == false) { // ignore multiedges
                        neighbours_j[k] = true; // set up sort of hashset
                        deg_j++;
                        if (neighbours_i[k]) {
                            intersect++;
                        }
                    }
                }
                graph->weights[x] = deg_i + deg_j - (2*intersect);
                assert(graph->weights[x] > 0);
                for (y=graph->sources[j]; y<graph->sources[j+1]; y++) {
                    int k = graph->targets[y];
                    neighbours_j[k] = false; // reset sort of hashset
                }
            }
            for (x=graph->sources[i]; x<graph->sources[i+1]; x++) {
                int j = graph->targets[x];
                neighbours_i[j] = false; // reset sort of hashset
            }
        }
        free(neighbours_i);
        free(neighbours_j);
    } else {
        // TODO: model == MODEL_MDS and MODEL_CIRCUIT
        assert(false); // mds and circuit model not supported
    }
    return graph;
}
static void free_adjacency(graph_sgd *graph) {
    free(graph->sources);
    free(graph->pinneds);
    free(graph->targets);
    free(graph->weights);
    free(graph);
}


void sgd(graph_t *G, /* input graph */
        int model /* distance model */)
{
    if (model == MODEL_CIRCUIT) {
        agerr(AGWARN, "circuit model not yet supported in Gmode=sgd, reverting to shortpath model\n");
        model = MODEL_SHORTPATH;
    }
    if (model == MODEL_MDS) {
        agerr(AGWARN, "mds model not yet supported in Gmode=sgd, reverting to shortpath model\n");
        model = MODEL_SHORTPATH;
    }
    int n = agnnodes(G);

    if (Verbose) {
        fprintf(stderr, "calculating shortest paths and setting up stress terms:");
        start_timer();
    }
    // calculate how many terms will be needed as fixed nodes can be ignored
    int i, n_fixed = 0, n_terms = 0;
    for (i=0; i<n; i++) {
        if (!isFixed(GD_neato_nlist(G)[i])) {
            n_fixed++;
            n_terms += n-n_fixed;
        }
    }
    term_sgd *terms = N_NEW(n_terms, term_sgd);
    // calculate term values through shortest paths
    int offset = 0;
    graph_sgd *graph = extract_adjacency(G, model);
    for (i=0; i<n; i++) {
        if (!isFixed(GD_neato_nlist(G)[i])) {
            offset += dijkstra_sgd(graph, i, terms+offset);
        }
    }
    assert(offset == n_terms);
    free_adjacency(graph);
    if (Verbose) {
        fprintf(stderr, " %.2f sec\n", elapsed_sec());
    }

    // initialise annealing schedule
    float w_min = terms[0].w, w_max = terms[0].w;
    int ij;
    for (ij=1; ij<n_terms; ij++) {
        if (terms[ij].w < w_min)
            w_min = terms[ij].w;
        if (terms[ij].w > w_max)
            w_max = terms[ij].w;
    }
    // note: Epsilon is different from MODE_KK and MODE_MAJOR as it is a minimum step size rather than energy threshold
    //       MaxIter is also different as it is a fixed number of iterations rather than a maximum
    float eta_max = 1 / w_min;
    float eta_min = Epsilon / w_max;
    float lambda = log(eta_max/eta_min) / (MaxIter-1);

    // initialise starting positions (from neatoprocs)
    initial_positions(G, n);
    // copy initial positions and state into temporary space for speed
    float *pos = N_NEW(2*n, float);
    bool *unfixed = N_NEW(n, bool);
    for (i=0; i<n; i++) {
        node_t *node = GD_neato_nlist(G)[i];
        pos[2*i] = ND_pos(node)[0];
        pos[2*i+1] = ND_pos(node)[1];
        unfixed[i] = !isFixed(node);
    }

    // perform optimisation
    if (Verbose) {
        fprintf(stderr, "solving model:");
        start_timer();
    }
    int t;
    rk_seed(0, &rstate); // TODO: get seed from graph
    for (t=0; t<MaxIter; t++) {
        fisheryates_shuffle(terms, n_terms);
        float eta = eta_max * exp(-lambda * t);
        for (ij=0; ij<n_terms; ij++) {
            // cap step size
            float mu = eta * terms[ij].w;
            if (mu > 1)
                mu = 1;

            float dx = pos[2*terms[ij].i] - pos[2*terms[ij].j];
            float dy = pos[2*terms[ij].i+1] - pos[2*terms[ij].j+1];
            float mag = sqrt(dx*dx + dy*dy);

            float r = (mu * (mag-terms[ij].d)) / (2*mag);
            float r_x = r * dx;
            float r_y = r * dy;

            if (unfixed[terms[ij].i]) {
                pos[2*terms[ij].i] -= r_x;
                pos[2*terms[ij].i+1] -= r_y;
            }
            if (unfixed[terms[ij].j]) {
                pos[2*terms[ij].j] += r_x;
                pos[2*terms[ij].j+1] += r_y;
            }
        }
        if (Verbose) {
            fprintf(stderr, " %.3f", calculate_stress(pos, terms, n_terms));
        }
    }
    if (Verbose) {
        fprintf(stderr, "\nfinished in %.2f sec\n", elapsed_sec());
    }
    free(terms);

    // copy temporary positions back into graph_t
    for (i=0; i<n; i++) {
        node_t *node = GD_neato_nlist(G)[i];
        ND_pos(node)[0] = pos[2*i];
        ND_pos(node)[1] = pos[2*i+1];
    }
    free(pos);
    free(unfixed);
}
