#ifdef __cplusplus
extern "C" {
#endif

#ifndef SGD_H
#define SGD_H

typedef struct term_sgd {
    int i, j;
    float d, w;
} term_sgd;

typedef struct graph_sgd {
    int n; // number of nodes
    int *sources; // index of first edge in *targets for each node (length n+1)
    bool *pinneds; // whether a node is fixed or not

    int *targets; // index of targets for each node (length sources[n])
    float *weights; // weights of edges (length sources[n])
} graph_sgd;

extern void sgd(graph_t *, int);

#endif /* SGD_H */

#ifdef __cplusplus
}
#endif
