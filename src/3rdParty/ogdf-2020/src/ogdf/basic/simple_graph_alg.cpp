/** \file
 * \brief Implementation of simple graph algorithms
 *
 * \author Carsten Gutwenger, Sebastian Leipert
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


#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/tuples.h>
#include <ogdf/basic/Math.h>

namespace ogdf {

// Functions related to self-loops

void removeSelfLoops(Graph &graph, node v) {
	adjEntry adj = v->firstAdj();
	adjEntry adjPrev = nullptr;

	while (adj != nullptr) {
		edge e{adj->theEdge()};
		if (e->isSelfLoop()) {
			graph.delEdge(e);
		} else {
			adjPrev = adj;
		}

		adj = adjPrev == nullptr ? v->firstAdj() : adjPrev->succ();
	}
}

bool isLoopFree(const Graph &G)
{
	for(edge e : G.edges)
		if(e->isSelfLoop()) return false;

	return true;
}

void makeLoopFree(Graph &G)
{
	safeForEach(G.edges, [&](edge e) {
		if (e->isSelfLoop()) G.delEdge(e);
	});
}

bool hasNonSelfLoopEdges(const Graph &G) {
	for (edge e : G.edges) {
		if (!e->isSelfLoop()) {
			return true;
		}
	}
	return false;
}

// Functions related to directed parallel edges

void parallelFreeSort(const Graph &G, SListPure<edge> &edges)
{
	G.allEdges(edges);

	BucketSourceIndex bucketSrc;
	edges.bucketSort(0,G.maxNodeIndex(),bucketSrc);

	BucketTargetIndex bucketTgt;
	edges.bucketSort(0,G.maxNodeIndex(),bucketTgt);
}

bool isParallelFree(const Graph &G) {
	return numParallelEdges<true>(G) == 0;
}

// Functions related to undirected parallel edges

void parallelFreeSortUndirected(const Graph &G,
	SListPure<edge> &edges,
	EdgeArray<int> &minIndex,
	EdgeArray<int> &maxIndex)
{
	G.allEdges(edges);

	for(edge e : G.edges) {
		int srcIndex = e->source()->index(), tgtIndex = e->target()->index();
		if (srcIndex <= tgtIndex) {
			minIndex[e] = srcIndex; maxIndex[e] = tgtIndex;
		} else {
			minIndex[e] = tgtIndex; maxIndex[e] = srcIndex;
		}
	}

	BucketEdgeArray bucketMin(minIndex), bucketMax(maxIndex);
	edges.bucketSort(0,G.maxNodeIndex(),bucketMin);
	edges.bucketSort(0,G.maxNodeIndex(),bucketMax);
}

bool isParallelFreeUndirected(const Graph &G) {
	return numParallelEdgesUndirected<true>(G) == 0;
}

// Testing and establishing connectivity

bool isConnected(const Graph &G)
{
	node v = G.firstNode();
	if (v == nullptr) return true;

	int count = 0;
	NodeArray<bool> visited(G,false);
	ArrayBuffer<node> S(G.numberOfNodes());

	S.push(v);
	visited[v] = true;
	while(!S.empty()) {
		v = S.popRet();
		++count;

		for(adjEntry adj : v->adjEntries) {
			node w = adj->twinNode();
			if(!visited[w]) {
				visited[w] = true;
				S.push(w);
			}
		}
	}

	return count == G.numberOfNodes();
}


void makeConnected(Graph &G, List<edge> &added)
{
	added.clear();
	if (G.numberOfNodes() == 0) return;
	NodeArray<bool> visited(G,false);
	ArrayBuffer<node> S(G.numberOfNodes());

	node pred = nullptr;
	for(node u : G.nodes)
	{
		if (visited[u]) continue;

		node vMinDeg = u;
		int  minDeg  = u->degree();

		S.push(u);
		visited[u] = true;

		while(!S.empty())
		{
			node v = S.popRet();

			for(adjEntry adj : v->adjEntries) {
				node w = adj->twinNode();
				if(!visited[w]) {
					visited[w] = true;
					S.push(w);

					int wDeg = w->degree();
					if (wDeg < minDeg) {
						vMinDeg = w;
						minDeg  = wDeg;
					}
				}
			}
		}

		if (pred)
			added.pushBack(G.newEdge(pred,vMinDeg));
		pred = vMinDeg;
	}
}


int connectedComponents(const Graph &G,
		NodeArray<int> &component,
		List<node> *isolated)
{
	int nComponent = 0;
	component.fill(-1);

	ArrayBuffer<node> S;

	for(node v : G.nodes) {
		if (component[v] != -1) continue;

		if (isolated != nullptr && v->degree() == 0) {
			isolated->pushBack(v);
		}
		S.push(v);
		component[v] = nComponent;

		while(!S.empty()) {
			node w = S.popRet();
			for(adjEntry adj : w->adjEntries) {
				node x = adj->twinNode();
				if (component[x] == -1) {
					component[x] = nComponent;
					S.push(x);
				}
			}
		}

		++nComponent;
	}

	return nComponent;
}

// Testing and establishing biconnectivity

//! Build up a dfs-tree starting from the node root by assigning each reachable
//! node in the graph a discovery time (number) and a parent.
/**
 * @param root is the node which should be the root of the dfs-tree.
 * @param number is assigned the number (discovery time) for each node.
 *        The number of root is firstNr, the number of unvisited nodes is 0.
 * @param parent is assigned the parent in the dfs-tree for each node.
 * @param childNr is assigned the number of children for each node.
 * @param revS is assigned all visited nodes such that the top element of revS
 *        is the node that was visited last.
 * @param directed should be set to true if the directionality of edges should
 *        be respected.
 * @param firstNr is the index > 0 at which the numbering of the nodes starts.
 * @return the number of visited nodes, i.e., nodes in the dfs-tree.
 */
static int buildDfsTree(const node &root,
		NodeArray<int> &number,
		NodeArray<node> &parent,
		NodeArray<int> &childNr,
		ArrayBuffer<node> &revS,
		bool directed = false,
		int firstNr = 1)
{
	OGDF_ASSERT(firstNr > 0);

	ArrayBuffer<node> S;
	S.push(root);

	int numCount = firstNr;
	childNr.fill(0);

	// Build up search tree and note discovery time and parent for each node.
	while (!S.empty()) {
		node v = S.popRet();

		// Ignore nodes that were already visited.
		if (number[v] != 0) {
			continue;
		}

		revS.push(v);

		// Set correct discovery time for v.
		number[v] = numCount++;

		// For all adjacent nodes w of v:
		for (adjEntry adj : v->adjEntries) {
			if (directed && adj->theEdge()->source() != v) {
				continue;
			}

			node w = adj->twinNode();

			// If w has not been visited yet:
			// Push it on the stack, remember its parent and number of children.
			if (number[w] == 0) {
				S.push(w);

				// If a parent was determined previously, revert that.
				if (parent[w] != nullptr) {
					childNr[parent[w]]--;
				}

				parent[w] = v;
				childNr[v]++;
			}
		}
	}

	return numCount - firstNr;
}

//! Find cut vertices and potential edges that could be added to turn the cut
//! vertices into non-cut vertices.
/**
 * The algorithm is applied to the graph whose nodes were pushed to the
 * ArrayBuffer revS. number, parent and revS can be obtained with buildDfsTree.
 *
 * @param number contains the number (discovery time) for each node.
 *        The number of root is 1, the number of unvisited nodes is 0.
 * @param parent contains the parent in the dfs-tree for each node.
 * @param revS contains the nodes of a graph such that the node that was visited
 *        last during the dfs-traversal is its top element.
 * @param cutVertices is assigned the cut vertices of the graph.
 * @param addEdges is assigned the tuples of nodes which have to be connected in
 *        order to turn each cut vertex into a non-cut vertex.
 * @param only_one should be set to true if the search should stop after finding
 *        one cut vertex, to false if all cut vertices should be found.
 * @return true if the graph contains at least one cut vertex, false otherwise.
 */
static bool findCutVertices(NodeArray<int> &number,
		NodeArray<node> &parent,
		ArrayBuffer<node> &revS,
		ArrayBuffer<node> &cutVertices,
		ArrayBuffer<Tuple2<node,node>> &addEdges,
		bool only_one)
{
	NodeArray<int> lowpt(number);

	// Go backwards through the dfs-tree:
	// Calculate the lowpoint for each node and test for cut vertices.
	while (!revS.empty()) {
		node v = revS.popRet();
		node firstChild = nullptr;

		// For all adjacent nodes w of v:
		for (adjEntry adj : v->adjEntries) {
			node w = adj->twinNode();

			// Ignore self-loops and the parent of v.
			if (v == w || parent[v] == w) {
				continue;
			}

			// If v->w is a backedge in the dfs-tree, update v's lowpoint.
			if (number[v] > number[w] ) {
				if (lowpt[v] > number[w]) {
					lowpt[v] = number[w];
				}
			} else {
				// If w is v's child in the dfs-tree, update v's lowpoint.
				if (parent[w] == v) {
					if (lowpt[v] > lowpt[w]) {
						lowpt[v] = lowpt[w];
					}

					// See whether w is v's first son.
					if (firstChild == nullptr) {
						firstChild = w;
					}

					// Non-root v is a cut vertex if lowpt[w] >= number[v].
					if (parent[v] != nullptr && lowpt[w] >= number[v]) {
						// Suggest to add an edge between w and v's parent.
						cutVertices.push(v);
						addEdges.push(Tuple2<node,node>(w, parent[v]));

						if (only_one) {
							return true;
						}
					}

					// Root v is a cut vertex if v has two or more children.
					if  (parent[v] == nullptr && w != firstChild) {
						// Suggest to add an edge between those children.
						cutVertices.push(v);
						addEdges.push(Tuple2<node,node>(w, firstChild));

						if (only_one) {
							return true;
						}
					}
				}
			}
		}
	}

	return !cutVertices.empty();
}

bool isBiconnected(const Graph &G, node &cutVertex)
{
	cutVertex = nullptr;

	if (G.empty()) {
		return true;
	}

	NodeArray<int> number(G,0);        // discovery times
	NodeArray<node> parent(G,nullptr); // parents in the dfs tree
	ArrayBuffer<node> revS;            // nodes of the dfs tree in reverse order

	// Build the dfs-tree and get the number of visited nodes.
	NodeArray<int> childNr(G);
	int numCount = buildDfsTree(G.firstNode(), number, parent, childNr, revS);

	// If the graph is not connected, return false.
	if (numCount != G.numberOfNodes()) {
		return false;
	}

	// If there are cut vertices in the graph, return false, else true.
	ArrayBuffer<node> cutVertices;
	ArrayBuffer<Tuple2<node,node>> addEdges;
	if (findCutVertices(number, parent, revS, cutVertices, addEdges, true)) {
		cutVertex = cutVertices.top();
		return false;
	} else {
		return true;
	}
}

void makeBiconnected(Graph &G, List<edge> &added)
{
	if (G.empty()) {
		return;
	}

	makeConnected(G, added);

	NodeArray<int> number(G,0);        // discovery times
	NodeArray<node> parent(G,nullptr); // parents in the dfs tree
	ArrayBuffer<node> revS;            // nodes of the dfs tree in reverse order

	// Build the dfs-tree.
	NodeArray<int> childNr(G);
	buildDfsTree(G.firstNode(), number, parent, childNr, revS);

	// Find all cut vertices.
	ArrayBuffer<node> cutVertices;
	ArrayBuffer<Tuple2<node,node>> addEdges;
	findCutVertices(number, parent, revS, cutVertices, addEdges, false);

	// Add a new edge for each cut vertex to make the graph biconnected.
	for (Tuple2<node,node> nodes : addEdges) {
		added.pushBack(G.newEdge(nodes.x1(), nodes.x2()));
	}
}


// Biconnected components

/**
 * Return true iff all incident edges of the given node v are self-loops.
 * In particular, return true if v has no incident edges at all.
 *
 * @param v the node to be tested for isolation.
 * @return true iff v has no incident edges to other nodes.
 */
static bool isIsolated(const node v)
{
	bool isolated = true;
	for (adjEntry adj : v->adjEntries) {
		if (adj->twinNode() != v) {
			isolated = false;
			break;
		}
	}

	return isolated;
}

int biconnectedComponents(const Graph &G, EdgeArray<int> &component, int &nComponent)
{
	if (G.empty()) {
		return 0;
	}

	NodeArray<int> number(G,0);      // discovery times
	NodeArray<int> lowpt(G);         // lowpoints
	ArrayBuffer<node> called;        // already visited nodes

	int nNumber = 0;                 // number of nodes
	int nIsolated = 0;               // number of isolated nodes
	nComponent = 0;                  // number of biconnected components

	// For every unvisited node u:
	for(node u : G.nodes) {
		if (number[u] != 0) {
			continue;
		}

		if (isIsolated(u)) {
			++nIsolated;
		}

		// We have to simulate the call stack to turn the normally recursive
		// algorithm into an iterative one. A struct for our stack variables:
		struct StackElem {
			node v;
			node parent;
			ListPure<adjEntry> *adjEntries;

			StackElem() {}
			StackElem(node vertex, node father) : v(vertex), parent(father) {
				adjEntries = new ListPure<adjEntry>;
				vertex->allAdjEntries(*adjEntries);
			}
		} initElem = {u, nullptr};

		// Start a depth-first search at u.
		ArrayBuffer<StackElem> stack;
		stack.push(initElem);
		bool forwards = true;

		while (!stack.empty()) {
			bool restartLoop = false;

			// Get current node, parent and adjEntries from the stack.
			StackElem elem = stack.top();
			node v = elem.v;
			node parent = elem.parent;
			ListPure<adjEntry> *adjEntries = elem.adjEntries;

			// If we are continuing the dfs forwards and have a new node:
			// Note discovery time & initial lowpoint, remember v for later.
			if (forwards) {
				lowpt[v] = number[v] = ++nNumber;
				called.push(v);
			} else {
				// If backtracking, update v's lowpt using its visited child w.
				node w = adjEntries->popFrontRet()->twinNode();
				if (lowpt[w] < lowpt[v]) {
					lowpt[v] = lowpt[w];
				}
			}

			// For all adjacent nodes w of v:
			while (!adjEntries->empty() && !restartLoop) {
				node w = adjEntries->front()->twinNode();

				// If w is unvisited, continue the dfs with w & v as its parent.
				if (number[w] == 0) {
					stack.push(StackElem(w,v));
					forwards = true;
					restartLoop = true;
				} else {
					if (v == w) {
						// Put self-loops in their own biconnected components.
						if (adjEntries->front()->isSource()) {
							component[adjEntries->front()->theEdge()] = nComponent++;
						}
					} else {
						// Else update v's lowpoint.
						if (number[w] < lowpt[v]) {
							lowpt[v] = number[w];
						}
					}
					adjEntries->popFront();
				}
			}

			if (restartLoop) {
				continue;
			}

			// If the parent of v is a cut vertex:
			// The upwards-going edges of all the nodes below that parent in the
			// dfs-tree form one biconnected component.
			if (parent != nullptr && lowpt[v] == number[parent]) {
				node w;
				do {
					w = called.popRet();
					for (adjEntry adj : w->adjEntries) {
						if (number[w] > number[adj->twinNode()]) {
							component[adj->theEdge()] = nComponent;
						}
					}
				} while (w != v);
				++nComponent;
			}

			// v has no more children to visit. Backtrack.
			stack.pop();
			forwards = false;
			delete adjEntries;
		}
	}

	return nComponent + nIsolated;
}

/**
 * Helper function for ogdf::isTwoEdgeConnected
 * Fills up the output parameters \p dfsOrder, \p prev and \p backEdges, by doing a dfs on the \p graph.
 *
 * @param graph input graph
 * @param dfsOrder the nodes of the graph get pushed into this list
 * in the order in which they appear in the DFS
 * @param prev maps to the edge from which a node was entered in the DFS
 * @param backEdges list all backedges of a node
 * @return false, if the \p graph is not connected, true otherwise
 */
static bool dfsTwoEdgeConnected(const Graph &graph,
                                List<node> &dfsOrder,
                                NodeArray<edge> &prev,
                                NodeArray<ArrayBuffer<edge>> &backEdges)
{
	dfsOrder.clear();
	prev.init(graph, nullptr);
	backEdges.init(graph, ArrayBuffer<edge>());
	EdgeArray<bool> visited(graph, false);

	struct StackElement {
		node v;
		edge e; // edge to v
	};
	ArrayBuffer<StackElement> stack;
	int visitCounter = 0;

	auto push = [&](node vPush, edge ignoredEdge){
		visitCounter++;
		dfsOrder.pushBack(vPush);
		for(adjEntry adj : vPush->adjEntries) {
			if(adj->theEdge() != ignoredEdge && !visited[adj->theEdge()]) {
				stack.push(StackElement{adj->twinNode(), adj->theEdge()});
			}
		}
	};
	push(graph.firstNode(), nullptr);

	while(stack.size() != 0) {
		StackElement currentElem = stack.popRet();
		node current = currentElem.v;
		edge prevEdge = currentElem.e;
		if(visited[prevEdge]) {
			continue;
		}
		visited[prevEdge] = true;
		if(prev[current] != nullptr || current == graph.firstNode()) {
			backEdges[current].push(prevEdge);
		} else {
			prev[current] = prevEdge;
			push(current, prevEdge);
		}
	}

	return visitCounter == graph.numberOfNodes();
}

/**
 * Helper function for ogdf::isTwoEdgeConnected
 *
 * @copydetails ogdf::dfsTwoEdgeConnected
 * @param bridge same as in ogdf::isTwoEdgeConnected
 */
static bool chainsTwoEdgeConnected(const Graph &graph,
                                   edge &bridge,
                                   List<node> &dfsOrder,
                                   const NodeArray<edge> &prev,
                                   const NodeArray<ArrayBuffer<edge>> &backEdges)
{
	NodeArray<bool> visited(graph, false);
	EdgeArray<bool> inAChain(graph, false);

	while(!dfsOrder.empty()) {
		node current = dfsOrder.popFrontRet();
		for(edge e : backEdges[current]) {
			inAChain[e] = true;
			visited[current] = true;
			node v = e->opposite(current);
			while(!visited[v]) {
				visited[v] = true;
				edge prevEdge = prev[v];
				if(prevEdge != nullptr) {
					v = prevEdge->opposite(v);
					inAChain[prevEdge] = true;
				}
			}
		}
	}

	for(edge e : graph.edges) {
		if(!inAChain[e]){
			bridge = e;
			// bridge found
			return false;
		}
	}

	return true;
}

bool isTwoEdgeConnected(const Graph &graph, edge &bridge) {
	bridge = nullptr;
	NodeArray<edge> prev(graph, nullptr);
	NodeArray<ArrayBuffer<edge>> backEdges(graph, ArrayBuffer<edge>());
	List<node> dfsOrder;

	if(graph.numberOfNodes() <= 1) {
		// empty and single-node graphs are defined to be 2-edge-connected
		return true;
	}

	if(!dfsTwoEdgeConnected(graph, dfsOrder, prev, backEdges)) {
		// not connected
		return false;
	}

	return chainsTwoEdgeConnected(graph, bridge, dfsOrder, prev, backEdges);
}

// Testing triconnectivity

bool isTriconnectedPrimitive(const Graph &G, node &s1, node &s2)
{
	s1 = s2 = nullptr;

	if (!isConnected(G) || !isBiconnected(G, s1)) {
		return false;
	}

	if (G.numberOfNodes() <= 3)
		return true;

	// make a copy of G
	GraphCopySimple GC(G);

	// for each node v in G, we test if G \ v is biconnected
	for(node v : G.nodes)
	{
		node vC = GC.copy(v), wC;

		// store adjacent nodes
		SListPure<node> adjacentNodes;
		for(adjEntry adj : vC->adjEntries) {
			wC = adj->twinNode();
			// forget self-loops (vC would no longer be in GC!)
			if (wC != vC)
				adjacentNodes.pushBack(wC);
		}

		GC.delNode(vC);

		// test for biconnectivity
		if(!isBiconnected(GC, wC)) {
			s1 = v; s2 = GC.original(wC);
			return false;
		}

		// restore deleted node with adjacent edges
		vC = GC.newNode(v);
		for(node uC : adjacentNodes)
			GC.newEdge(vC,uC);
	}

	return true;
}


// Triangulations

void triangulate(Graph &G)
{
	OGDF_ASSERT(isSimple(G));

	CombinatorialEmbedding E(G);

#ifdef OGDF_DEBUG
	E.consistencyCheck();
#endif

	adjEntry succ, succ2, succ3;
	NodeArray<int> marked(E.getGraph(), 0);

	for(node v : E.getGraph().nodes) {
		marked.init(E.getGraph(), 0);

		for(adjEntry adj : v->adjEntries) {
			marked[adj->twinNode()] = 1;
		}

		// forall faces adj to v
		for(adjEntry adj : v->adjEntries) {
			succ = adj->faceCycleSucc();
			succ2 = succ->faceCycleSucc();

			if (succ->twinNode() != v && adj->twinNode() != v) {
				while (succ2->twinNode() != v) {
					if (marked[succ2->theNode()] == 1) {
						// edge e=(x2,x4)
						succ3 = succ2->faceCycleSucc();
						E.splitFace(succ, succ3);
					}
					else {
						// edge e=(v=x1,x3)
						edge e = E.splitFace(adj, succ2);
						marked[succ2->theNode()] = 1;

						// old adj is in wrong face
						adj = e->adjSource();
					}
					succ = adj->faceCycleSucc();
					succ2 = succ->faceCycleSucc();
				}
			}
		}
	}
}


// Testing and establishing acyclicity

bool isAcyclic(const Graph &G, List<edge> &backedges)
{
	backedges.clear();

	NodeArray<int> number(G,0);        // discovery times
	NodeArray<node> parent(G,nullptr); // parents in the dfs tree
	NodeArray<int> childNr(G);         // number of children in the dfs tree
	ArrayBuffer<node> revS;

	ArrayBuffer<node> leaves;          // leaves of the dfs tree
	NodeArray<int> completion(G,0);    // completion times
	int complCount = 0;
	int numCount = 0;

	// For all unvisited nodes:
	for (node v : G.nodes) {
		if (number[v] == 0) {
			// Build the dfs-tree starting at v.
			numCount += buildDfsTree(v, number, parent, childNr, revS, true, numCount+1);

			// Get all leaves of the dfs-tree.
			while (!revS.empty()) {
				node w = revS.popRet();
				if (childNr[w] == 0) {
					leaves.push(w);
				}
			}

			node lastParent = parent[leaves.top()];

			// Go through leaves of the dfs-tree.
			while (!leaves.empty()) {
				node w = leaves.top();

				// If the new leaf is a child of the same parent as before,
				// assign it a completion time and pop it from the stack.
				if (parent[w] == lastParent) {
					completion[w] = complCount++;
					leaves.pop();

					// The last parent has now one child less. If it has no
					// children anymore, push it as a new leaf on the stack.
					if (lastParent != nullptr) {
						childNr[lastParent]--;
						if (childNr[lastParent] == 0) {
							leaves.push(lastParent);
							lastParent = parent[lastParent];
						}
					}
				} else {
					// Else just continue with the next leaves and their parent.
					lastParent = parent[w];
				}
			}
		}
	}

	// Remember backedges.
	for(edge e : G.edges) {
		node src = e->source();
		node tgt = e->target();

		if (number[src] >= number[tgt] && completion[src] <= completion[tgt]) {
			backedges.pushBack(e);
		}
	}

	return backedges.empty();
}


bool isAcyclicUndirected(const Graph &G, List<edge> &backedges)
{
	backedges.clear();

	NodeArray<int> number(G,0);        // discovery times
	NodeArray<node> parent(G,nullptr); // parents in the dfs tree
	ArrayBuffer<node> S;
	int numCount = 0;

	// For all unvisited nodes:
	for (node v : G.nodes) {
		if (number[v] == 0) {
			// Start depth first search at v.
			S.push(v);
			while (!S.empty()) {
				node w = S.popRet();

				// Ignore nodes that were already visited.
				if (number[w] != 0) {
					continue;
				}

				// Set correct discovery time for w.
				number[w] = ++numCount;
				bool parentSeen = false;

				// For all adjacent nodes u of w:
				for (adjEntry adj : w->adjEntries) {
					node u = adj->twinNode();

					// If u has not been visited yet,
					// push it on the stack and remember its parent.
					if (number[u] == 0) {
						S.push(u);
						parent[u] = w;
					} else if (parent[w] == u && !parentSeen) {
						// The first time you see w's parent, it is no backedge.
						parentSeen = true;
					} else if (w != u || adj->isSource()) {
						// Collect backedges (self-loops only in one direction).
						backedges.pushBack(adj->theEdge());
					}
				}
			}
		}
	}

	return backedges.empty();
}


void makeAcyclic(Graph &G)
{
	List<edge> backedges;
	isAcyclic(G,backedges);

	for(edge e : backedges)
		G.delEdge(e);
}


void makeAcyclicByReverse(Graph &G)
{
	List<edge> backedges;
	isAcyclic(G,backedges);

	for(edge e : backedges)
		if (!e->isSelfLoop()) G.reverseEdge(e);
}


// Testing sources and sinks

bool hasSingleSource(const Graph& G, node &s)
{
	s = nullptr;

	for(node v : G.nodes) {
		if (v->indeg() == 0) {
			if (s != nullptr) {
				s = nullptr;
				return false;
			} else s = v;
		}
	}
	return G.empty() || s != nullptr;
}


bool hasSingleSink(const Graph& G, node &t)
{
	t = nullptr;

	for(node v : G.nodes) {
		if (v->outdeg() == 0) {
			if (t != nullptr) {
				t = nullptr;
				return false;
			} else t = v;
		}
	}
	return G.empty() || t != nullptr;
}


// isStGraph()
// true <=> G is st-graph, i.e., is acyclic, contains exactly one source s
//   and one sink t, and the edge (s,t); returns single source s and single
//   sink t if contained (otherwise they are set to 0), and edge st if
//   contained (otherwise 0)
bool isStGraph(const Graph &G, node &s, node &t, edge &st)
{
	st = nullptr;

	hasSingleSource(G,s);
	hasSingleSink  (G,t);

	if (s == nullptr || t == nullptr || !isAcyclic(G)) {
		s = t = nullptr;
		return false;
	}

	for(adjEntry adj : s->adjEntries) {
		edge e = adj->theEdge();

		if (e->target() == t) {
			st = e;
			break;
		}
	}

	return st != nullptr;
}


// Topological numbering in acyclic graphs

void topologicalNumbering(const Graph &G, NodeArray<int> &num)
{
	ArrayBuffer<node> S(G.numberOfNodes());
	NodeArray<int> indeg(G);

	for(node v : G.nodes)
		if((indeg[v] = v->indeg()) == 0)
			S.push(v);

	int count = 0;
	while(!S.empty()) {
		node v = S.popRet();
		num[v] = count++;

		for(adjEntry adj : v->adjEntries) {
			node u = adj->theEdge()->target();
			if (u != v && --indeg[u] == 0) {
				S.push(u);
			}
		}
	}
}

int strongComponents(const Graph &graph, NodeArray<int> &components)
{
	int nNodes = graph.numberOfNodes();

	if (nNodes == 0) {
		return 0;
	}

	// A node v is on the stack set iff index[v] > -1 and lowLinks[v] < nNodes.
	NodeArray<int> lowLinks(graph, -1);
	NodeArray<int> index(graph, -1);
	ArrayBuffer<node> set(nNodes);
	int nextIndex = 0;
	int result = 0;

	// For every unvisited node u:
	for (node u : graph.nodes) {
		if (index[u] == -1) {
			// We have to simulate the call stack to turn the normally recursive
			// algorithm into an iterative one. A struct for our stack variables:
			struct StackElem {
				node v;
				ListPure<edge> *outEdges;

				StackElem() = default;
				explicit StackElem(node vertex) : v(vertex) {
					outEdges = new ListPure<edge>;
					vertex->outEdges(*outEdges);
				}
			} initElem = StackElem(u);

			// Start a depth-first search at u.
			ArrayBuffer<StackElem> stack;
			stack.push(initElem);
			bool forwards = true;

			while (!stack.empty()) {
				bool restartLoop = false;

				// Get current node and outEdges from the stack.
				StackElem elem = stack.top();
				node v = elem.v;
				ListPure<edge> *outEdges = elem.outEdges;

				// If we are continuing the dfs forwards and have a new node v:
				// Note v's index & initial lowlink, and remember it for later.
				if (forwards) {
					index[v] = lowLinks[v] = nextIndex++;
					set.push(v);
				} else {
					// If backtracking, update v's lowlink using its child w.
					node w = outEdges->popFrontRet()->target();
					Math::updateMin(lowLinks[v], lowLinks[w]);
				}

				// For all direct successors w of v:
				while (!outEdges->empty() && !restartLoop) {
					node w = outEdges->front()->target();

					// If w is unvisited, continue the dfs with w.
					if (index[w] == -1) {
						stack.push(StackElem(w));
						forwards = true;
						restartLoop = true;
					} else {
						// Else update v's lowlink.
						Math::updateMin(lowLinks[v], lowLinks[w]);
						outEdges->popFront();
					}
				}

				if (restartLoop) {
					continue;
				}

				// The nodes collected so far form one component.
				if (lowLinks[v] == index[v]) {
					node w;
					do {
						w = set.popRet();
						components[w] = result;
						lowLinks[w] = nNodes;
					} while (w != v);
					result++;
				}

				// v has no more children to visit. Backtrack.
				stack.pop();
				forwards = false;
				delete outEdges;
			}
		}
	}

	return result;
}

// makes the DiGraph bimodal such that all embeddings of the
// graph are bimodal embeddings!
void makeBimodal(Graph &G, List<edge> &newEdge)
{
	List<node> nodes;
	G.allNodes(nodes);

	ListIterator<node> it_n = nodes.begin();
	while (it_n.valid()) {
		node v = *it_n;
		if (v->indeg() < 2 || v->outdeg() < 2) {
			++it_n; continue;
		}
		List<adjEntry> newOrder;
		for (adjEntry adj : v->adjEntries) {
			if (adj->theEdge()->target() == v)
				newOrder.pushFront(adj);
			else
				newOrder.pushBack(adj);
		}
		G.sort(v, newOrder);

		ListIterator<adjEntry> it = newOrder.begin();
		while ((*it)->theEdge()->target() == v)
			++it;
		node newNode = G.splitNode(newOrder.front(), *it);
		for (adjEntry adj : newNode->adjEntries) {
			if (adj->theEdge()->target() == newNode) {
				newEdge.pushBack(adj->theEdge());
				break;
			}
		}
		++it_n;
	}
}

// Forest and arborescence testing

bool isArborescenceForest(const Graph& G, List<node> &roots)
{
	roots.clear();
	if (G.empty()) {
		return true;
	}

	if (G.numberOfNodes() <= G.numberOfEdges()) {
		return false;
	}

	ArrayBuffer<node> stack;
	int nodeCount = 0;

	// Push every source node to roots and start a dfs from it.
	for (node u : G.nodes) {
		if (u->indeg() == 0) {
			roots.pushBack(u);
			stack.push(u);

			while (!stack.empty()) {
				// Get node v from the stack and increase the node counter.
				node v = stack.popRet();
				nodeCount++;

				// Push all successors of v to the stack.
				// If one of them has indegree > 1, return false.
				for (adjEntry adj : v->adjEntries) {
					if (adj->isSource()) {
						node w = adj->twinNode();
						if (w->indeg() > 1) {
							return false;
						}
						stack.push(w);
					}
				}
			}
		}
	}

	// If there are still unvisited nodes, return false, else true.
	return nodeCount == G.numberOfNodes();
}


bool isArborescence (const Graph& G, node &root)
{
	List<node> roots;

	if (isArborescenceForest(G,roots) && roots.size() == 1) {
		root = roots.front(); return true;
	}
	return false;
}

bool isRegular(const Graph& G) {
	if (G.numberOfEdges() == 0) {
		return true;
	}
	return isRegular(G, G.firstNode()->degree());
}

bool isRegular(const Graph& G, int d) {
	for (auto n: G.nodes) {
		if (n->degree() != d) {
			return false;
		}
	}
	return true;
}

bool isBipartite(const Graph &G, NodeArray<bool> &color) {
	ArrayBuffer<node> stack;
	NodeArray<bool> visited(G, false);

	// Start a depth-first search from every unvisited node.
	for (node root : G.nodes) {
		if (!visited[root]) {
			stack.push(root);
			color[root] = true;
			visited[root] = true;

			while (!stack.empty()) {
				node v = stack.popRet();

				// For all adjacent nodes w of v:
				for (adjEntry adj : v->adjEntries) {
					node w = adj->twinNode();

					// If w is already visited/on the stack, check its color.
					if (visited[w]) {
						if (color[w] == color[v]) {
							return false;
						}
					} else {
						// Else color it and push it on the stack.
						visited[w] = true;
						color[w] = !color[v];
						stack.push(w);
					}
				}
			}
		}
	}

	return true;
}

void nodeDistribution(const Graph& G, Array<int> &dist, std::function<int(node)> func) {
	int maxval = 0;
	int minval = std::numeric_limits<int>::max();

	if (G.empty()) {
		dist.init();
		return;
	}

	for (node v : G.nodes) {
		Math::updateMax(maxval, func(v));
		Math::updateMin(minval, func(v));
	}

	dist.init(minval, maxval, 0);
	for (node v : G.nodes) {
		++dist[func(v)];
	}
}

}
