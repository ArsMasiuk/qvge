/** \file
 * \brief Tests for the basic graph class
 *
 * \author Tilo Wiedera
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

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/graph_generators.h>
#include <resources.h>

/**
 * Returns an arbitrary edge where both nodes have at least \c minDegree incident edges.
 * Requires the graph to contain at least one such edge.
 *
 * @param graph graph to investigate
 * @param minDegree minimal number of incident edges
 * @return the chosen edge
 */
edge chooseEdge(const Graph &graph, int minDegree) {
	return graph.chooseEdge([&](edge e) { return e->source()->degree() >= minDegree && e->target()->degree() >= minDegree; });
}

/**
 * Returns an arbitrary node with at least \c minDegree incident edges.
 * Requires the graph to contain at least one such node.
 *
 * @param graph graph to investigate
 * @param minDegree minimal number of incident edges
 * @return the chosen node
 */
node chooseNode(const Graph &graph, int minDegree) {
	return graph.chooseNode([&](node v) { return v->degree() >= minDegree; });
}

/**
 * Returns an arbitrary node which does not equal \c v.
 * Requires the graph to contain at least one such node.
 *
 * @param graph graph to investigate
 * @param v the node to exclude from selection
 * @return the chosen node
 */
node chooseNode(const Graph &graph, node v) {
	return graph.chooseNode([&](node w) { return w != v; });
}

go_bandit([](){
describe("Graph Class", [](){
	std::vector<std::string> files = {"rome/grafo3703.45.lgr.gml.pun", "rome/grafo5745.50.lgr.gml.pun", "north/g.41.26.gml", "north/g.61.11.gml", "north/g.73.8.gml"};

	it("is initialized correctly", [](){
		Graph graph;

		AssertThat(graph.empty(), IsTrue());
		AssertThat(graph.numberOfNodes(), Equals(0));
		AssertThat(graph.numberOfEdges(), Equals(0));
		AssertThat(graph.maxNodeIndex(), IsLessThan(0));
		AssertThat(graph.maxEdgeIndex(), IsLessThan(0));
		AssertThat(graph.maxAdjEntryIndex(), IsLessThan(0));
		AssertThat(graph.nodeArrayTableSize(), IsGreaterThan(0));
		AssertThat(graph.edgeArrayTableSize(), IsGreaterThan(0));
		AssertThat(graph.adjEntryArrayTableSize(), IsGreaterThan(0));
		AssertThat(graph.firstNode(), IsNull());
		AssertThat(graph.lastNode(), IsNull());
		AssertThat(graph.firstEdge(), IsNull());
		AssertThat(graph.lastEdge(), IsNull());
	});

	for_each_graph_it("finds an existing edge", files, [](Graph &graph){
		edge e = graph.chooseEdge();
		AssertThat(graph.searchEdge(e->source(), e->target(), true), Equals(e));
		AssertThat(graph.searchEdge(e->source(), e->target(), false), Equals(e));
	});

	for_each_graph_it("returns the adjacency entries of an edge", files, [](Graph &graph){
		edge e = graph.chooseEdge();
		adjEntry adjSrc = e->adjSource();
		adjEntry adjTgt = e->adjTarget();

		AssertThat(adjSrc == adjTgt, IsFalse());
		AssertThat(adjSrc->isSource(), IsTrue());
		AssertThat(adjTgt->isSource(), IsFalse());
	});

	for_each_graph_it("finds a reverse edge", files, [](Graph &graph){
		edge e = graph.chooseEdge();
		AssertThat(graph.searchEdge(e->target(), e->source()), Equals(e));
	});

	for_each_graph_it("does not find non-existent edges", files, [](Graph &graph){
		edge e = graph.chooseEdge();
		node s = e->source();
		node t = e->target();
		graph.delEdge(e);
		AssertThat(graph.searchEdge(s, t), IsNull());

		edge reverseE = graph.newEdge(t, s);
		AssertThat(graph.searchEdge(s, t), Equals(reverseE));
		AssertThat(graph.searchEdge(s, t, true), IsNull());
	});

	for_each_graph_it("can be assigned", files, [](Graph &graph){
		int m = graph.numberOfEdges();

		int *degreeCounter = new int[m];

		for(int i = 0; i < m; i++) {
			degreeCounter[i] = 0;
		}

		for(node v : graph.nodes) {
			degreeCounter[v->degree()]++;
		}

		Graph copy = graph;

		AssertThat(copy.numberOfNodes(), Equals(graph.numberOfNodes()));
		AssertThat(copy.numberOfEdges(), Equals(m));

		for(node v : copy.nodes) {
			degreeCounter[v->degree()]--;
		}

		for(node v : graph.nodes) {
			AssertThat(degreeCounter[v->degree()], Equals(0));
		}

		delete[] degreeCounter;
	});

	it("maintains the adjacency order at nodes with self-loops", [] {
		Graph graph;
		node v = graph.newNode();
		List<adjEntry> entries;

		for(int i = 0; i < 2; i++) {
			edge e = graph.newEdge(v, v);
			entries.pushBack(e->adjTarget());
			entries.pushBack(e->adjSource());
		}

		graph.sort(v, entries);
		Graph copy(graph);

		for (adjEntry adj : copy.firstNode()->adjEntries) {
			edge e = adj->theEdge();
			adjEntry succ = adj->cyclicSucc();
			edge eSucc = succ->theEdge();

			bool isSourceAdj = adj == e->adjSource();

			AssertThat(e != eSucc, Equals(isSourceAdj));

			if (isSourceAdj) {
				AssertThat(succ == eSucc->adjTarget(), IsTrue());
			} else {
				AssertThat(succ == e->adjSource(), IsTrue());
			}
		}
	});

	it("adds nodes", [](){
		Graph graph;
		const int numberOfNodes = 100;
		emptyGraph(graph,numberOfNodes);

		AssertThat(graph.empty(), IsFalse());
		AssertThat(graph.numberOfNodes(), Equals(numberOfNodes));
		AssertThat(graph.numberOfEdges(), Equals(0));
		AssertThat(graph.maxNodeIndex(), IsGreaterThan(numberOfNodes - 2));
		AssertThat(graph.firstNode(), !IsNull());
		AssertThat(graph.lastNode(), !IsNull());

		int maxIndex = graph.maxNodeIndex();
		bool *visited = new bool[maxIndex + 1];

		for(int i = 0; i <= maxIndex; i++) {
			visited[i] = false;
		}

		int count = 0;

		for(node v : graph.nodes) {
			int index = v->index();
			AssertThat(index, IsGreaterThan(-1));
			AssertThat(index, IsLessThan(maxIndex + 1));
			AssertThat(visited[index], IsFalse());
			visited[index] = true;
			count++;
		}

		AssertThat(count, Equals(numberOfNodes));

		delete[] visited;
	});

	it("adds edges", [](){
		Graph graph;
		emptyGraph(graph, 100);

		int count = 0;

		for(node v : graph.nodes) {
			for(node w : graph.nodes) {
				if((v->index() + w->index()) % 3 == 0) {
					graph.newEdge(v, w);
					count++;
				}
			}
		}

		AssertThat(graph.numberOfEdges(), Equals(count));
		AssertThat(graph.maxEdgeIndex(), IsGreaterThan(count - 2));
		AssertThat(graph.maxAdjEntryIndex(), IsGreaterThan(count - 2));
		AssertThat(graph.firstEdge(), !IsNull());
		AssertThat(graph.lastEdge(), !IsNull());

		int maxIndex = graph.maxEdgeIndex();
		bool *visited = new bool[maxIndex + 1];

		for(int i = 0; i <= maxIndex; i++) {
			visited[i] = false;
		}

		int iterCount = 0;

		for(edge e : graph.edges) {
			int index = e->index();
			AssertThat(index, IsGreaterThan(-1));
			AssertThat(index, IsLessThan(maxIndex + 1));
			AssertThat(visited[index], IsFalse());
			visited[index] = true;
			iterCount++;
		}

		AssertThat(iterCount, Equals(count));

		delete[] visited;
	});

	it("doesn't duplicate self-loops", [](){
		Graph graph;

		node v = graph.newNode();
		graph.newEdge(v, v);

		List<edge> edges;
		v->adjEdges(edges);
		AssertThat(edges.size(), Equals(2));
		v->inEdges(edges);
		AssertThat(edges.size(), Equals(1));
		v->outEdges(edges);
		AssertThat(edges.size(), Equals(1));
	});

	for_each_graph_it("removes a node", files, [](Graph &graph){
		int n = graph.numberOfNodes();
		int m = graph.numberOfEdges();

		node v = graph.chooseNode();
		int deg = v->degree();

		graph.delNode(v);

		AssertThat(graph.numberOfNodes(), Equals(n - 1));
		AssertThat(graph.numberOfEdges(), Equals(m - deg));
	});

	for_each_graph_it("removes an edge", files, [](Graph &graph){
		int n = graph.numberOfNodes();
		int m = graph.numberOfEdges();

		edge e = graph.chooseEdge();
		node s = e->source();
		node t = e->target();

		graph.delEdge(e);

		AssertThat(graph.searchEdge(s, t), IsNull());
		AssertThat(graph.numberOfNodes(), Equals(n));
		AssertThat(graph.numberOfEdges(), Equals(m - 1));
	});

	for_each_graph_it("can be cleared", files, [](Graph &graph){
		graph.clear();

		AssertThat(graph.empty(), IsTrue());
		AssertThat(graph.numberOfNodes(), Equals(0));
		AssertThat(graph.numberOfEdges(), Equals(0));
	});

	for_each_graph_it("hides an edge and restores it", files, [](Graph &graph){
		int n = graph.numberOfNodes();
		int m = graph.numberOfEdges();

		edge e = graph.chooseEdge();
		Graph::HiddenEdgeSet set(graph);
		set.hide(e);

		AssertThat(set.size(), Equals(1));
		AssertThat(graph.numberOfNodes(), Equals(n));
		AssertThat(graph.numberOfEdges(), Equals(m - 1));
		AssertThat(graph.searchEdge(e->source(), e->target()), IsNull());

		set.restore(e);

		AssertThat(set.size(), Equals(0));
		AssertThat(graph.numberOfEdges(), Equals(m));
		AssertThat(graph.searchEdge(e->source(), e->target()), Equals(e));
	});

	for_each_graph_it("restores all hidden edges", files, [](Graph &graph){
		int m = graph.numberOfEdges();
		Graph::HiddenEdgeSet set(graph);

		// this should not change anything
		set.restore();

		for(int i = 0; i < m / 2; i++) {
			set.hide(graph.chooseEdge());
		}

		AssertThat(set.size(), Equals(m / 2));
		AssertThat(graph.numberOfEdges(), Equals(m - m / 2));
		set.restore();
		AssertThat(set.size(), Equals(0));
		AssertThat(graph.numberOfEdges(), Equals(m));
	});

	for_each_graph_it("hides all edges across 10 sets", files, [](Graph &graph){
		int m = graph.numberOfEdges();
		int maxIndex = graph.maxNodeIndex();

		int *inDeg = new int[maxIndex + 1];
		int *outDeg = new int[maxIndex + 1];

		for(node v : graph.nodes) {
			inDeg[v->index()] = v->indeg();
			outDeg[v->index()] = v->outdeg();
		}

		List<Graph::HiddenEdgeSet*> sets;

		for(int i = 0; i < 10; i++) {
			sets.pushFront(new Graph::HiddenEdgeSet(graph));

			for(int k = 0; k < m / 10; k++) {
				sets.front()->hide(graph.chooseEdge());
			}
		}

		sets.permute();

		while(graph.numberOfEdges() > 0) {
			sets.front()->hide(graph.chooseEdge());
		}

		for(node v : graph.nodes) {
			AssertThat(v->indeg(), Equals(0));
			AssertThat(v->outdeg(), Equals(0));
		}

		for(Graph::HiddenEdgeSet *set : sets) {
			// restore edges by deleting the set
			delete set;
		}

		AssertThat(graph.numberOfEdges(), Equals(m));

		for(node v : graph.nodes) {
			AssertThat(v->indeg(), Equals(inDeg[v->index()]));
			AssertThat(v->outdeg(), Equals(outDeg[v->index()]));
		}

		delete[] inDeg;
		delete[] outDeg;
	});

	for_each_graph_it("restores edges upon graph destruction", files, [](Graph &graph) {
		GraphCopy *copy = new GraphCopy(graph);
		Graph::HiddenEdgeSet set(*copy);
		set.hide(copy->chooseEdge());
		delete copy;
		AssertThat(set.size(), Equals(0));
	});

#ifdef OGDF_USE_ASSERT_EXCEPTIONS
	for_each_graph_it("doesn't hide edges of other graphs", files, [](Graph &graph) {
		GraphCopy copy(graph);
		Graph::HiddenEdgeSet set(copy);
		AssertThrows(AssertionFailed, set.hide(graph.chooseEdge()));

	});

	for_each_graph_it("doesn't restore a non-hidden edge", files, [](Graph &graph) {
		Graph::HiddenEdgeSet set(graph);
		AssertThrows(AssertionFailed, set.restore(graph.chooseEdge()));
	});

	for_each_graph_it("doesn't hide an edge twice", files, [](Graph &graph) {
		Graph::HiddenEdgeSet set(graph);
		edge e = graph.chooseEdge();
		set.hide(e);
		AssertThrows(AssertionFailed, set.hide(e));
	});

	for_each_graph_it("doesn't restore an edge twice", files, [](Graph &graph) {
		Graph::HiddenEdgeSet set(graph);
		edge e = graph.chooseEdge();
		set.hide(e);
		set.restore(e);
		AssertThrows(AssertionFailed, set.restore(e));
	});
#endif

	for_each_graph_it("reverses an edge", files, [](Graph &graph){
		edge e = chooseEdge(graph, 5);
		node s = e->source();
		node t = e->target();

		int inT = t->indeg();
		int outT = t->outdeg();

		int inS = s->indeg();
		int outS = s->outdeg();

		graph.reverseEdge(e);

		AssertThat(e->source(), Equals(t));
		AssertThat(e->target(), Equals(s));
		AssertThat(e->source()->degree(), Equals(inT + outT));
		AssertThat(e->target()->degree(), Equals(inS + outS));
		AssertThat(e->source()->indeg(), Equals(inT - 1));
		AssertThat(e->source()->outdeg(), Equals(outT + 1));
	});

	for_each_graph_it("reverses all edges", files, [](Graph &graph){
		int maxIndex = graph.maxEdgeIndex();
		node *sources = new node[maxIndex + 1];
		node *targets = new node[maxIndex + 1];

		for(int i = 0; i <= maxIndex; i++) {
			sources[i] = targets[i] = nullptr;
		}

		for(edge e : graph.edges) {
			sources[e->index()] = e->source();
			targets[e->index()] = e->target();
		}

		graph.reverseAllEdges();

		for(edge e : graph.edges) {
			AssertThat(e->source(), Equals(targets[e->index()]));
			AssertThat(e->target(), Equals(sources[e->index()]));
		}

		delete[] sources;
		delete[] targets;
	});

	for_each_graph_it("moves an adjacency entry", files, [](Graph &graph){
		adjEntry adj = chooseEdge(graph, 5)->adjSource();
		adjEntry adjSucc = adj->cyclicSucc();

		graph.moveAdj(adj, Direction::after, adjSucc);

		AssertThat(adjSucc->cyclicSucc(), Equals(adj));
		AssertThat(adj->cyclicSucc(), Is().Not().EqualTo(adjSucc));

		graph.moveAdj(adj, Direction::before, adjSucc);

		AssertThat(adj->cyclicSucc(), Equals(adjSucc));
		AssertThat(adjSucc->cyclicSucc(), Is().Not().EqualTo(adj));
	});

	for_each_graph_it("swaps the target of an edge", files, [](Graph &graph){
		edge e = graph.chooseEdge();
		node s = e->source();
		node t = e->target();

		node v = chooseNode(graph, t);

		graph.moveTarget(e, v);

		AssertThat(e->source(), Equals(s));
		AssertThat(e->target(), Equals(v));
	});

	for_each_graph_it("swaps the source of an edge", files, [](Graph &graph){
		edge e = graph.chooseEdge();
		node s = e->source();
		node t = e->target();

		node v = chooseNode(graph, s);

		graph.moveSource(e, v);

		AssertThat(e->source(), Equals(v));
		AssertThat(e->target(), Equals(t));
	});

	for_each_graph_it("splits an edge", files, [](Graph &graph){
		int n = graph.numberOfNodes();
		int m = graph.numberOfEdges();

		edge e = graph.chooseEdge();
		node v = e->target();

		edge f = graph.split(e);

		AssertThat(f->source(), Equals(e->target()));
		AssertThat(f->target(), Equals(v));
		AssertThat(f->source()->degree(), Equals(2));
		AssertThat(graph.numberOfNodes(), Equals(n + 1));
		AssertThat(graph.numberOfEdges(), Equals(m + 1));
	});

	for_each_graph_it("un-splits an edge by dummy-node", files, [](Graph &graph){
		int n = graph.numberOfNodes();
		int m = graph.numberOfEdges();

		edge e = graph.chooseEdge();
		node s = e->source();
		node t = e->target();

		graph.split(e);

		node v = e->target();

		graph.unsplit(v);

		AssertThat(graph.numberOfNodes(), Equals(n));
		AssertThat(graph.numberOfEdges(), Equals(m));
		AssertThat(e->source(), Equals(s));
		AssertThat(e->target(), Equals(t));
		AssertThat(graph.searchEdge(s, t), Equals(e));
	});

	for_each_graph_it("un-splits an edge by dummy-edge", files, [](Graph &graph){
		int n = graph.numberOfNodes();
		int m = graph.numberOfEdges();

		edge e = graph.chooseEdge();
		node s = e->source();
		node t = e->target();

		edge f = graph.split(e);
		graph.unsplit(e, f);

		AssertThat(graph.numberOfNodes(), Equals(n));
		AssertThat(graph.numberOfEdges(), Equals(m));
		AssertThat(e->source(), Equals(s));
		AssertThat(e->target(), Equals(t));
		AssertThat(graph.searchEdge(s, t), Equals(e));
	});

	for_each_graph_it("splits nodes", files, [](Graph &graph){
		node vLeft = chooseNode(graph, 6);

		int degree = vLeft->degree();
		List<adjEntry> entries;
		vLeft->allAdjEntries(entries);
		adjEntry adjFirstRight = *entries.get(degree / 2);
		node vRight = graph.splitNode(vLeft->firstAdj(), adjFirstRight);
		int count = 0;

		for(adjEntry adj = vLeft->firstAdj()->succ(); adj != nullptr; adj = adj->succ()) {
			AssertThat(adj, Equals(*entries.get(count++)));
		}

		for(adjEntry adj = vRight->firstAdj()->succ(); adj != nullptr; adj = adj->succ()) {
			AssertThat(adj, Equals(*entries.get(count++)));
		}

		AssertThat(count, Equals(degree));
		AssertThat(vLeft->degree() + vRight->degree(), Equals(degree + 2));
	});

	for_each_graph_it("contracts an edge", files, [](Graph &graph){
		edge e = chooseEdge(graph, 5);
		node s = e->source();
		node t = e->target();

		// create the list of expected adjacency order
		List<node> nodes;
		List<edge> edges;
		s->adjEdges(edges);

		for(edge f : edges) {
			nodes.pushBack(f->opposite(s));
		}

		// to prevent ambiguity, delete would-be multi-edges
		List<edge> deleteMe;
		t->adjEdges(edges);
		ListIterator<node> it = nodes.search(t);

		while(edges.front() != e) {
			edges.moveToBack(edges.begin());
		}

		edges.del(edges.begin());

		for(edge f : edges) {
			if(nodes.search(f->opposite(t)).valid()) {
				deleteMe.pushBack(f);
			} else {
				nodes.insertBefore(f->opposite(t), it);
			}
		}

		nodes.del(it);

		for(edge f : deleteMe) {
			graph.delEdge(f);
		}

		node v = graph.contract(e);
		edge f = graph.searchEdge(v, nodes.front());

		AssertThat(v == t || v == s, IsTrue());
		AssertThat(v->degree(), Equals(nodes.size()));
		AssertThat(f, !IsNull());

		adjEntry adj = f->source() == v ? f->adjSource() : f->adjTarget();
		for(node w : nodes) {
			AssertThat(adj->twinNode(), Equals(w));
			adj = adj->cyclicSucc();
		}

	});

	for_each_graph_it("collapses half of all nodes", files, [](Graph &graph){
		int m = graph.numberOfEdges();

		List<node> nodes;
		int maxIndex = graph.maxNodeIndex();
		bool *adjacent = new bool[maxIndex + 1];

		for(int i = 0; i <= maxIndex; i++) {
			adjacent[i] = false;
		}

		for(node v : graph.nodes) {
			if(v->index() % 2) {
				nodes.pushBack(v);
			}
		}

		int minRemoved = 0;
		for(edge e : graph.edges) {
			int target = e->target()->index();
			int source = e->source()->index();

			if(source % 2 && target % 2 == 0) {
				adjacent[target] = true;
			}

			if(source % 2 == 0 && target % 2) {
				adjacent[source] = true;
			}

			minRemoved += source % 2 && target % 2;
		}

		node v = nodes.front();
		graph.collapse(nodes);

		AssertThat(nodes.empty(), IsTrue());
		AssertThat(graph.numberOfEdges(), IsLessThan(1 + m - minRemoved));

		for(adjEntry adj = v->firstAdj(); adj != nullptr; adj = adj->succ()) {
			adjacent[adj->twinNode()->index()] = false;
		}

		for(int i = 0; i <= maxIndex; i++) {
			AssertThat(adjacent[i], IsFalse());
		}

		delete[] adjacent;
	});

	for_each_graph_it("sorts adjacency lists", files, [](Graph &graph){
		node v = chooseNode(graph, 6);

		List<adjEntry> entries;
		v->allAdjEntries(entries);

		entries.permute();

		graph.sort(v, entries);

		AssertThat(v->firstAdj(), Equals(entries.front()));
		AssertThat(v->lastAdj(), Equals(entries.back()));

		adjEntry adjBefore = nullptr;
		for(adjEntry adj : entries) {
			if(adjBefore != nullptr) {
				AssertThat(adjBefore->succ(), Equals(adj));
				AssertThat(adj->pred(), Equals(adjBefore));
			}

			adjBefore = adj;
		}
	});

	for_each_graph_it("reverses the order of all edges adjacent to a given node", files, [](Graph &graph){
		node v = chooseNode(graph, 6);
		List<edge> edges;
		v->adjEdges(edges);

		graph.reverseAdjEdges(v);
		edges.reverse();

		adjEntry adj = v->firstAdj();
		for(edge e : edges) {
			AssertThat(adj, !IsNull());
			AssertThat(adj->theEdge(), Equals(e));

			adj = adj->succ();
		}
	});

	for_each_graph_it("swaps adjacency entries", files, [](Graph &graph){
		edge e = chooseEdge(graph, 5);
		adjEntry adj = e->adjSource()->cyclicSucc()->cyclicSucc();

		graph.swapAdjEdges(e->adjSource(), adj);

		AssertThat(adj->cyclicSucc()->cyclicSucc(), Equals(e->adjSource()));
		AssertThat(e->adjSource()->cyclicSucc()->cyclicSucc(), Is().Not().EqualTo(adj));
	});

	for_each_graph_it("does not return a negative genus", files, [](Graph &graph){
		AssertThat(graph.genus(), IsGreaterThan(-1));
	});

	for_each_graph_it("detects a combinatorial embedding", files, [](Graph &graph){
		AssertThat(graph.representsCombEmbedding(), Equals(graph.genus() == 0));
	});

	for_each_graph_it("returns wether an adjacency entry lies between two others", files, [](Graph &graph) {
		node v = graph.newNode();

		while(graph.numberOfNodes() < 12) {
			graph.newNode();
		}

		int n = graph.numberOfNodes();
		int count = 0;
		adjEntry adjs[3];

		// Add new edge for every third node.
		// Pick 3 adjacency entries from the first, second, and last third.
		for(node w : graph.nodes) {
			if (count % 3 == 0) {
				adjs[(count*3)/n] = graph.newEdge(v, w)->adjSource();
			}

			count++;
		}

		AssertThat(adjs[0]->isBetween(adjs[2], adjs[1]), IsTrue());
		AssertThat(adjs[0]->isBetween(adjs[1], adjs[2]), IsFalse());

		AssertThat(adjs[1]->isBetween(adjs[0], adjs[2]), IsTrue());
		AssertThat(adjs[1]->isBetween(adjs[2], adjs[0]), IsFalse());

		AssertThat(adjs[2]->isBetween(adjs[1], adjs[0]), IsTrue());
		AssertThat(adjs[2]->isBetween(adjs[0], adjs[1]), IsFalse());
	});

	for_each_graph_it("returns the adjacency entry of an edge", files, [](Graph &graph) {
		node v = graph.chooseNode();

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();

			adjEntry adj2 = e->getAdj(v);

			AssertThat(adj2->theNode(), Equals(v));
			AssertThat(adj2->theEdge(), Equals(e));

			if(!e->isSelfLoop()) {
				AssertThat(adj2, Equals(adj));
			}
		}
	});
});

describe("EdgeElement", [] {
	Graph graph;
	node u{graph.newNode()};
	node v{graph.newNode()};
	node w{graph.newNode()};
	edge eSelfLoop1{graph.newEdge(u, u)};
	edge eSelfLoop2{graph.newEdge(u, u)};
	edge eParallelBase{graph.newEdge(v, w)};
	edge eParallelDirected{graph.newEdge(v, w)};
	edge eParallelInverted{graph.newEdge(w, v)};
	edge eUnrelated{graph.newEdge(v, u)};

	describe("nodes()", [&] {
		it("returns the same node twice on self-loops", [&] {
			for (edge e : {eSelfLoop1, eSelfLoop2}) {
				for (node x : e->nodes()) {
					AssertThat(x, Equals(u));
				}
			}
		});

		it("returns source and target on non-self-loops", [&] {
			for (edge e : {eParallelBase, eParallelDirected, eParallelInverted, eUnrelated}) {
				NodeArray<int> mark{graph, 0};
				for (node x : e->nodes()) {
					++mark[x];
				}
				for (node x : graph.nodes) {
					if (e->isIncident(x)) {
						AssertThat(mark[x], Equals(1));
					} else {
						AssertThat(mark[x], Equals(0));
					}
				}
			}
		});
	});

	describe("opposite()", [&] {
		it("returns the same node on self-loops", [&] {
			AssertThat(eSelfLoop1->opposite(u), Equals(u));
			AssertThat(eSelfLoop2->opposite(u), Equals(u));
		});

		it("returns the opposite node on non-self-loops", [&] {
			AssertThat(eParallelBase->opposite(w), Equals(v));
			AssertThat(eParallelDirected->opposite(v), Equals(w));
			AssertThat(eParallelInverted->opposite(v), Equals(w));
			AssertThat(eUnrelated->opposite(v), Equals(u));
		});
	});

	describe("isSelfLoop()", [&] {
		it("recognizes self-loops as self-loops", [&] {
			AssertThat(eSelfLoop1->isSelfLoop(), IsTrue());
			AssertThat(eSelfLoop2->isSelfLoop(), IsTrue());
		});

		it("recognizes non-self-loops as non-self-loops", [&] {
			for (edge e : {eParallelBase, eParallelDirected, eParallelInverted, eUnrelated}) {
				AssertThat(e->isSelfLoop(), IsFalse());
			}
		});
	});

	describe("isInvertedDirected()", [&] {
		it("recognizes self-loops as inverted edges", [&] {
			AssertThat(eSelfLoop1->isInvertedDirected(eSelfLoop1), IsTrue());
			AssertThat(eSelfLoop1->isInvertedDirected(eSelfLoop2), IsTrue());
			AssertThat(eSelfLoop2->isInvertedDirected(eSelfLoop2), IsTrue());
			AssertThat(eSelfLoop2->isInvertedDirected(eSelfLoop1), IsTrue());
		});

		it("recognizes inverted non-self-loop edges", [&] {
			AssertThat(eParallelBase->isInvertedDirected(eParallelInverted), IsTrue());
			AssertThat(eParallelInverted->isInvertedDirected(eParallelBase), IsTrue());
		});

		it("recognizes non-inverted edges", [&] {
			AssertThat(eSelfLoop1->isInvertedDirected(eUnrelated), IsFalse());
			AssertThat(eParallelBase->isInvertedDirected(eParallelDirected), IsFalse());
			AssertThat(eUnrelated->isInvertedDirected(eParallelBase), IsFalse());
		});

		it("recognizes itself as non-inverted (except self-loops)", [&] {
			for (edge e : graph.edges) {
				if (!e->isSelfLoop()) {
					AssertThat(e->isInvertedDirected(e), IsFalse());
				}
			}
		});
	});

	describe("isParallelDirected()", [&] {
		it("recognizes self-loops as parallel edges", [&] {
			AssertThat(eSelfLoop1->isParallelDirected(eSelfLoop1), IsTrue());
			AssertThat(eSelfLoop1->isParallelDirected(eSelfLoop2), IsTrue());
			AssertThat(eSelfLoop2->isParallelDirected(eSelfLoop2), IsTrue());
			AssertThat(eSelfLoop2->isParallelDirected(eSelfLoop1), IsTrue());
		});

		it("recognizes parallel non-self-loop edges", [&] {
			AssertThat(eParallelBase->isParallelDirected(eParallelDirected), IsTrue());
			AssertThat(eParallelDirected->isParallelDirected(eParallelBase), IsTrue());
		});

		it("recognizes non-parallel edges", [&] {
			AssertThat(eSelfLoop1->isParallelDirected(eUnrelated), IsFalse());
			AssertThat(eParallelBase->isParallelDirected(eParallelInverted), IsFalse());
			AssertThat(eUnrelated->isParallelDirected(eParallelBase), IsFalse());
		});

		it("recognizes itself as parallel", [&] {
			for (edge e : graph.edges) {
				AssertThat(e->isParallelDirected(e), IsTrue());
			}
		});
	});

	describe("isParallelUndirected()", [&] {
		it("recognizes self-loops as parallel edges", [&] {
			AssertThat(eSelfLoop1->isParallelUndirected(eSelfLoop1), IsTrue());
			AssertThat(eSelfLoop1->isParallelUndirected(eSelfLoop2), IsTrue());
			AssertThat(eSelfLoop2->isParallelUndirected(eSelfLoop2), IsTrue());
			AssertThat(eSelfLoop2->isParallelUndirected(eSelfLoop1), IsTrue());
		});

		it("recognizes parallel non-self-loop edges", [&] {
			AssertThat(eParallelBase->isParallelUndirected(eParallelDirected), IsTrue());
			AssertThat(eParallelBase->isParallelUndirected(eParallelInverted), IsTrue());
			AssertThat(eParallelDirected->isParallelUndirected(eParallelBase), IsTrue());
			AssertThat(eParallelInverted->isParallelUndirected(eParallelDirected), IsTrue());
		});

		it("recognizes non-parallel edges", [&] {
			AssertThat(eSelfLoop1->isParallelUndirected(eUnrelated), IsFalse());
			AssertThat(eUnrelated->isParallelUndirected(eParallelBase), IsFalse());
			AssertThat(eParallelInverted->isParallelUndirected(eUnrelated), IsFalse());
		});

		it("recognizes itself as parallel", [&] {
			for (edge e : graph.edges) {
				AssertThat(e->isParallelUndirected(e), IsTrue());
			}
		});
	});

	describe("isIncident()", [&] {
		it("recognizes incident nodes as incident", [&] {
			for (node x : graph.nodes) {
				for (adjEntry adj : x->adjEntries) {
					AssertThat(adj->theEdge()->isIncident(x), IsTrue());
				}
			}
		});

		it("recognizes non-incident nodes as non-incident", [&] {
			AssertThat(eSelfLoop1->isIncident(v), IsFalse());
			AssertThat(eSelfLoop1->isIncident(w), IsFalse());
			AssertThat(eParallelBase->isIncident(u), IsFalse());
			AssertThat(eUnrelated->isIncident(w), IsFalse());
		});
	});

	describe("isAdjacent()", [&] {
		it("recognizes itself as adjacent", [&] {
			for (edge e : graph.edges) {
				AssertThat(e->isAdjacent(e), IsTrue());
			}
		});

		it("recognizes parallel edges as adjacent", [&] {
			AssertThat(eSelfLoop1->isAdjacent(eSelfLoop2), IsTrue());
			AssertThat(eParallelBase->isAdjacent(eParallelDirected), IsTrue());
			AssertThat(eParallelInverted->isAdjacent(eParallelDirected), IsTrue());
		});

		it("recognizes adjacent edges as adjacent", [&] {
			AssertThat(eSelfLoop1->isAdjacent(eUnrelated), IsTrue());
			AssertThat(eUnrelated->isAdjacent(eParallelInverted), IsTrue());
			AssertThat(eUnrelated->isAdjacent(eParallelBase), IsTrue());
		});

		it("recognizes non-adjacent edges as non-adjacent", [&] {
			AssertThat(eSelfLoop1->isAdjacent(eParallelBase), IsFalse());
			AssertThat(eParallelInverted->isAdjacent(eSelfLoop2), IsFalse());
		});
	});

	describe("commonNode()", [&] {
		it("returns any common node of parallel edges", [&] {
			AssertThat(eSelfLoop1->commonNode(eSelfLoop2), Equals(u));
			AssertThat(eParallelBase->commonNode(eParallelDirected), Equals(v) || Equals(w));
			AssertThat(eParallelDirected->commonNode(eParallelInverted), Equals(v) || Equals(w));
		});

		it("returns the common node of adjacent non-parallel edges", [&] {
			AssertThat(eSelfLoop1->commonNode(eUnrelated), Equals(u));
			AssertThat(eUnrelated->commonNode(eSelfLoop2), Equals(u));
			AssertThat(eParallelBase->commonNode(eUnrelated), Equals(v));
		});

		it("returns nullptr if edges are non-adjacent", [&] {
			AssertThat(eSelfLoop1->commonNode(eParallelBase), IsNull());
			AssertThat(eParallelInverted->commonNode(eSelfLoop2), IsNull());
		});
	});
});
});
