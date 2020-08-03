/** \file
 * \brief Implementation of ogdf::SpringEmbedderKK.
 *
 * \author Karsten Klein
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <ogdf/energybased/SpringEmbedderKK.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {
const double SpringEmbedderKK::startVal = std::numeric_limits<double>::max() - 1.0;
const double SpringEmbedderKK::minVal = DBL_MIN;
const double SpringEmbedderKK::desMinLength = 0.0001;
const int SpringEmbedderKK::maxVal = std::numeric_limits<int>::max();

void SpringEmbedderKK::initialize(
	GraphAttributes& GA,
	NodeArray<dpair>& partialDer,
	const EdgeArray<double>& eLength,
	NodeArray< NodeArray<double> >& oLength,
	NodeArray< NodeArray<double> >& sstrength,
	bool simpleBFS)
{
	double maxDist;
	const Graph &G = GA.constGraph();
	m_prevEnergy =  startVal;
	m_prevLEnergy =  startVal;

	// all edges straight-line
	GA.clearAllBends();
	if (!m_useLayout)
		shufflePositions(GA);

	//the shortest path lengths
	for(node v : G.nodes)
		oLength[v].init(G, std::numeric_limits<double>::max());

	//computes shortest path distances d_ij
	if (simpleBFS)
	{
		//we use simply BFS n times
		//TODO experimentally compare speed, also with bintree dijkstra
#if 0
#ifdef OGDF_DEBUG
		double timeUsed;
		usedTime(timeUsed);
#endif
#endif
		maxDist = allpairsspBFS(G, oLength);

#if 0
#ifdef OGDF_DEBUG
		timeUsed = usedTime(timeUsed);
		std::cout << "\n******APSP BFS runtime: \n";
#endif
#endif
	}
	else
	{
		EdgeArray<double> adaptedLength(G);
		adaptLengths(G, GA, eLength, adaptedLength);
		//we use simply the BFM n times or Floyd instead, leading to cubic runtime
		//TODO experimentally compare speed, also with bintree dijkstra
		maxDist = allpairssp(G, adaptedLength, oLength, std::numeric_limits<double>::max());
	}
	//computes original spring length l_ij

	//first we determine desirable edge length L
	//nodes sizes may be non-uniform, we approximate the display size (grid)
	//this part relies on the fact that node sizes are set != zero
	//TODO check later if this is a good choice
	double L = m_desLength; //desirable length
	if (L < desMinLength)
	{
		double swidth = 0.0, sheight = 0.0;

		// Do all nodes lie on the same point? Check by computing BB of centers
		// Then, perform simple shifting in layout
		node vFirst = G.firstNode();
		double minX = GA.x(vFirst), maxX = GA.x(vFirst),
				minY = GA.y(vFirst), maxY = GA.y(vFirst);
		// Two goals:
		//				add node sizes to estimate desirable length
		//				compute BB to check degeneracy
		for(node v : G.nodes)
		{
			swidth += GA.width(v);
			sheight += GA.height(v);

			if(GA.x(v) < minX) minX = GA.x(v);
			if(GA.x(v) > maxX) maxX = GA.x(v);
			if(GA.y(v) < minY) minY = GA.y(v);
			if(GA.y(v) > maxY) maxY = GA.y(v);
		}

		double sroot = maxDist;//sqrt(G.numberOfNodes());
		swidth = swidth / sroot;
		sheight = sheight / sroot;
		double Lzero = max(2.0*sroot, 2.0*(swidth + sheight));
		//test for multilevel
		Lzero = max(max(maxX-minX, maxY-minY), 2.0*Lzero);
#if 0
		std::cout << "Lzero: "<<Lzero<<"\n";
#endif


		L = Lzero / maxDist;

#if 0
#ifdef OGDF_DEBUG
		std::cout << "Desirable edge length computed: "<<L<<"\n";
#endif
#endif
	}
	// Having L we can compute the original lengths l_ij
	// Computes spring strengths k_ij
	double dij;
	for(node v : G.nodes)
	{
		sstrength[v].init(G);
		for(node w : G.nodes)
		{
			dij = oLength[v][w];
			if (dij == std::numeric_limits<double>::max())
			{
				sstrength[v][w] = minVal;
			}
			else
			{
				oLength[v][w] = L * dij;
				if (v==w) sstrength[v][w] = 1.0;
				else
				sstrength[v][w] = m_K / (dij * dij);
			}
		}
	}
}

void SpringEmbedderKK::mainStep(GraphAttributes& GA,
								NodeArray<dpair>& partialDer,
								NodeArray< NodeArray<double> >& oLength,
								NodeArray< NodeArray<double> >& sstrength)
{
	const Graph &G = GA.constGraph();

#ifdef OGDF_DEBUG
	NodeArray<int> nodeCounts(G, 0);
	int nodeCount = 0; //number of moved nodes
#endif
	// Now we compute delta_m, we search for the node with max value
	double delta_m = 0.0;
	node best_m = G.firstNode();

	// Compute the partial derivatives first
	for(node v : G.nodes)
	{
		dpair parder = computeParDers(v, GA, sstrength, oLength);
		partialDer[v] = parder;
		//delta_m is sqrt of squares of partial derivatives
		double delta_v = sqrt(parder.x1()*parder.x1() + parder.x2()*parder.x2());

		if (delta_v > delta_m)
		{
			best_m = v;
			delta_m = delta_v;
		}
	}

	int globalItCount, localItCount;
	if (m_computeMaxIt)
	{
		globalItCount = m_gItBaseVal+m_gItFactor*G.numberOfNodes();
		localItCount = 2*G.numberOfNodes();
	}
	else
	{
		globalItCount = m_maxGlobalIt;
		localItCount = m_maxLocalIt;
	}


	while (globalItCount-- > 0 && !finished(delta_m))
	{
#ifdef OGDF_DEBUG
		nodeCount++;
		nodeCounts[best_m]++;
#endif
		// The contribution best_m makes to the partial derivatives of
		// each vertex.
		NodeArray<dpair> p_partials(G);
		for(node v : G.nodes)
		{
			p_partials[v] = computeParDer(v, best_m, GA, sstrength, oLength);
		}

		localItCount = 0;
		do {
			// Compute the 4 elements of the Jacobian
			double dE_dx_dx = 0.0, dE_dx_dy = 0.0, dE_dy_dx = 0.0, dE_dy_dy = 0.0;
			for(node v : G.nodes)
			{
				if (v != best_m) {
					double x_diff = GA.x(best_m) - GA.x(v);
					double y_diff = GA.y(best_m) - GA.y(v);
					double dist = sqrt(x_diff * x_diff + y_diff * y_diff);
					double dist3 = dist * dist * dist;
					OGDF_ASSERT(dist3 != 0.0);
					double k_mi = sstrength[best_m][v];
					double l_mi = oLength[best_m][v];
					dE_dx_dx += k_mi * (1 - (l_mi * y_diff * y_diff)/dist3);
					dE_dx_dy += k_mi * l_mi * x_diff * y_diff / dist3;
					dE_dy_dx += k_mi * l_mi * x_diff * y_diff / dist3;
					dE_dy_dy += k_mi * (1 - (l_mi * x_diff * x_diff)/dist3);
				}
			}

			// Solve for delta_x and delta_y
			double dE_dx = partialDer[best_m].x1();
			double dE_dy = partialDer[best_m].x2();

			double delta_x =
				(dE_dx_dy * dE_dy - dE_dy_dy * dE_dx)
				/ (dE_dx_dx * dE_dy_dy - dE_dx_dy * dE_dy_dx);

			double delta_y =
				(dE_dx_dx * dE_dy - dE_dy_dx * dE_dx)
				/ (dE_dy_dx * dE_dx_dy - dE_dx_dx * dE_dy_dy);

			// Move p by (delta_x, delta_y)
			GA.x(best_m) += delta_x;
			GA.y(best_m) += delta_y;

			// Recompute partial derivatives and delta_p
			dpair deriv = computeParDers(best_m, GA, sstrength, oLength);
			partialDer[best_m] = deriv;

			delta_m =
				sqrt(deriv.x1()*deriv.x1() + deriv.x2()*deriv.x2());
		} while (localItCount-- > 0 && !finishedNode(delta_m));

		// Select new best_m by updating each partial derivative and delta
		node old_p = best_m;
		for(node v : G.nodes)
		{
			dpair old_deriv_p = p_partials[v];
			dpair old_p_partial =
				computeParDer(v, old_p, GA, sstrength, oLength);
			dpair deriv = partialDer[v];

			deriv.x1() += old_p_partial.x1() - old_deriv_p.x1();
			deriv.x2() += old_p_partial.x2() - old_deriv_p.x2();

			partialDer[v] = deriv;
			double delta = sqrt(deriv.x1()*deriv.x1() + deriv.x2()*deriv.x2());

			if (delta > delta_m) {
				best_m = v;
				delta_m = delta;
			}
		}
	}
}

void SpringEmbedderKK::doCall(GraphAttributes& GA, const EdgeArray<double>& eLength, bool simpleBFS)
{
	const Graph& G = GA.constGraph();
	NodeArray<dpair> partialDer(G); //stores the partial derivative per node
	NodeArray< NodeArray<double> > oLength(G);//first distance, then original length
	NodeArray< NodeArray<double> > sstrength(G);//the spring strength

	//only for debugging
	OGDF_ASSERT(isConnected(G));

	//compute relevant values
	initialize(GA, partialDer, eLength, oLength, sstrength, simpleBFS);

	//main loop with node movement
	mainStep(GA, partialDer, oLength, sstrength);

	if (simpleBFS) scale(GA);
}


void SpringEmbedderKK::call(GraphAttributes& GA)
{
	const Graph &G = GA.constGraph();
	if (!hasNonSelfLoopEdges(G)) {
		return;
	}

	EdgeArray<double> eLength(G);//, 1.0);is not used
	doCall(GA, eLength, true);
}

void SpringEmbedderKK::call(GraphAttributes& GA,  const EdgeArray<double>& eLength)
{
	const Graph &G = GA.constGraph();
	if (!hasNonSelfLoopEdges(G)) {
		return;
	}

	doCall(GA, eLength, false);
}

//changes given edge lengths (interpreted as weight factors)
//according to additional parameters like node size etc.
void SpringEmbedderKK::adaptLengths(
	const Graph& G,
	const GraphAttributes& GA,
	const EdgeArray<double>& eLengths,
	EdgeArray<double>& adaptedLengths)
{
	//we use the edge lengths as factor and try to respect
	//the node sizes such that each node has enough distance
	//adapt to node sizes
	for (edge e : G.edges)
	{
		double smax = max(GA.width(e->source()), GA.height(e->source()));
		double tmax = max(GA.width(e->target()), GA.height(e->target()));
		if (smax + tmax > 0.0)
			adaptedLengths[e] = (1 + eLengths[e])*(smax + tmax);///2.0);
		else adaptedLengths[e] = 5.0*eLengths[e];
	}
}

void SpringEmbedderKK::shufflePositions(GraphAttributes& GA)
{
	const Graph &G = GA.constGraph();
	int n = G.numberOfNodes();
	for (node v : G.nodes) {
		GA.x(v) = randomDouble(0.0, n);
		GA.y(v) = randomDouble(0.0, n);
	}
}

// Compute contribution of vertex u to the first partial
// derivatives (dE/dx_m, dE/dy_m) (for vertex m) (eq. 7 and 8 in paper)
SpringEmbedderKK::dpair SpringEmbedderKK::computeParDer(
	node m,
	node u,
	GraphAttributes& GA,
	NodeArray< NodeArray<double> >& ss,
	NodeArray< NodeArray<double> >& dist)
{
	dpair result(0.0, 0.0);
	if (m != u)
	{
		double x_diff = GA.x(m) - GA.x(u);
		double y_diff = GA.y(m) - GA.y(u);
		double distance = sqrt(x_diff * x_diff + y_diff * y_diff);
		result.x1() = (ss[m][u]) * (x_diff - (dist[m][u])*x_diff/distance);
		result.x2() = (ss[m][u]) * (y_diff - (dist[m][u])*y_diff/distance);
	}

	return result;
}


//compute partial derivative for v
SpringEmbedderKK::dpair SpringEmbedderKK::computeParDers(node v,
	GraphAttributes& GA,
	NodeArray< NodeArray<double> >& ss,
	NodeArray< NodeArray<double> >& dist)
{
	dpair result(0.0, 0.0);
	for(node u : GA.constGraph().nodes)
	{
		dpair deriv = computeParDer(v, u, GA, ss, dist);
		result.x1() += deriv.x1();
		result.x2() += deriv.x2();
	}

	return result;
}


/**
 * Initialise the original estimates from nodes and edges.
 */

//we could speed this up by not using nested NodeArrays and
//by not doing the fully symmetrical computation on undirected graphs
//All Pairs Shortest Path Floyd, initializes the whole matrix
//returns maximum distance. Does not detect negative cycles (lead to neg. values on diagonal)
//threshold is the value for the distance of non-adjacent nodes, distance has to be
//initialized with
double SpringEmbedderKK::allpairssp(const Graph& G, const EdgeArray<double>& eLengths, NodeArray< NodeArray<double> >& distance,
	const double threshold)
{
	double maxDist = -threshold;

	for(node v : G.nodes)
	{
		distance[v][v] = 0.0;
	}

	//TODO: Experimentally compare this against
	// all nodes and incident edges (memory access) on huge graphs
	for(edge e : G.edges)
	{
		distance[e->source()][e->target()] = eLengths[e];
		distance[e->target()][e->source()] = eLengths[e];
	}

///**
// * And run the main loop of the algorithm.
// */
	for(node v : G.nodes)
	{
		for(node u : G.nodes)
		{
			for(node w : G.nodes)
			{
				if ((distance[u][v] < threshold) && (distance[v][w] < threshold))
				{
					Math::updateMin(distance[u][w], distance[u][v] + distance[v][w]);
#if 0
					distance[w][u] = distance[u][w]; //is done anyway afterwards
#endif
				}
				if (distance[u][w] < threshold)
					Math::updateMax(maxDist, distance[u][w]);
			}
		}
	}
#if 0
#ifdef OGDF_DEBUG
	for(node v : G.nodes)
	{
		if (distance[v][v] < 0.0) std::cerr << "\n###Error in shortest path computation###\n\n";
	}
	std::cout << "Maxdist: "<<maxDist<<"\n";
	for(node u : G.nodes)
	{
	for(node w : G.nodes)
	{
#if 0
		std::cout << "Distance " << u->index() << " -> "<<w->index()<<" "<<distance[u][w]<<"\n";
#endif
	}
	}
#endif
#endif
	return maxDist;
}

//the same without weights, i.e. all pairs shortest paths with BFS
//Runs in time |V|²
//for compatibility, distances are double
double SpringEmbedderKK::allpairsspBFS(const Graph& G, NodeArray< NodeArray<double> >& distance)
{
	double maxDist = 0;

	for(node v : G.nodes)
	{
		distance[v][v] = 0.0;
	}

	//start in each node once
	for(node v: G.nodes)
	{
		//do a bfs
		NodeArray<bool> mark(G, true);
		SListPure<node> bfs;
		bfs.pushBack(v);
		mark[v] = false;

		while (!bfs.empty())
		{
			node w = bfs.popFrontRet();
			double d = distance[v][w]+1.0f;


			for(adjEntry adj : w->adjEntries) {
				node u = adj->twinNode();
				if (mark[u])
				{
					mark[u] = false;
					bfs.pushBack(u);
					distance[v][u] = d;
					Math::updateMax(maxDist, d);
				}
			}
		}
	}
	//check for negative cycles
	for(node v : G.nodes)
	{
		if (distance[v][v] < 0.0)
			std::cerr << "\n###Error in shortest path computation###\n\n";
	}

#if 0
#ifdef OGDF_DEBUG
	std::cout << "Maxdist: "<<maxDist<<"\n";
	for(node u : G.nodes)
	{
	for(node w : G.nodes)
	{
		std::cout << "Distance " << u->index() << " -> "<<w->index()<<" "<<distance[u][w]<<"\n";
	}
	}
#endif
#endif
	return maxDist;
}

void SpringEmbedderKK::scale(GraphAttributes& GA)
{
	//Simple version: Just scale to max needed
	//We run over all edges, find the largest distance needed and scale
	//the edges uniformly
	double maxFac = 0.0;
	bool scale = true;
	for (edge e : GA.constGraph().edges)
	{
		double w1 = sqrt(GA.width(e->source())*GA.width(e->source()) +
			GA.height(e->source())*GA.height(e->source()));
		double w2 = sqrt(GA.width(e->target())*GA.width(e->target()) +
			GA.height(e->target())*GA.height(e->target()));
		w2 = (w1 + w2) / 2.0; //half length of both diagonals
		double xs = GA.x(e->source());
		double xt = GA.x(e->target());
		double ys = GA.y(e->source());
		double yt = GA.y(e->target());
		double xdist = xs - xt;
		double ydist = ys - yt;
		if ((fabs(xs) > (std::numeric_limits<double>::max() / 2.0) - 1) ||
		    (fabs(xt) > (std::numeric_limits<double>::max() / 2.0) - 1) ||
		    (fabs(ys) > (std::numeric_limits<double>::max() / 2.0) - 1) ||
		    (fabs(yt) > (std::numeric_limits<double>::max() / 2.0) - 1)) {
			scale = false; //never scale with huge numbers
		}
		//(even though the drawing may be small and could be shifted to origin)
		double elength = sqrt(xdist*xdist + ydist*ydist);

		//Avoid a max factor of inf!!
		if (OGDF_GEOM_ET.greater(elength, 0.0001))
		{
			w2 = m_distFactor * w2 / elength;//relative to edge length

			if (w2 > maxFac)
				maxFac = w2;
		}
	}


	if (maxFac > 1.0 && (maxFac < (std::numeric_limits<double>::max() / 2.0) - 1) && scale) //only scale to increase distance
	{
		//if maxFac is large, we scale in steps until we reach a threshold
		if (maxFac > 2048)
		{
			double scaleF = maxFac + 0.00001;
			double base = 2.0;
			maxFac = base;

			while (scale && maxFac<scaleF)
			{
				for (node v : GA.constGraph().nodes)
				{
					GA.x(v) = GA.x(v)*base;
					GA.y(v) = GA.y(v)*base;
					if (GA.x(v) >(std::numeric_limits<double>::max() / base) - 1 || GA.y(v) > (std::numeric_limits<double>::max() / base) - 1)
						scale = false;
				}
				maxFac *= base;
			}
		}
		else
		{
			for (node v : GA.constGraph().nodes)
			{
				GA.x(v) = GA.x(v)*maxFac;
				GA.y(v) = GA.y(v)*maxFac;
			}
		}
#if 0
#ifdef OGDF_DEBUG
		std::cout << "Scaled by factor "<<maxFac<<"\n";
#endif
#endif
	}
}

}
