/** \file
 * \brief Implements Hopcroft/Tarjan algorithm for finding the
 * triconnected components of a biconnected multi-graph
 *
 * \author Carsten Gutwenger
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


#include <ogdf/graphalg/Triconnectivity.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/NodeSet.h>

//#define TRIC_COMP_OUTPUT


namespace ogdf {


Triconnectivity::~Triconnectivity()
{
	delete m_pGC;
}


// Divides G into triconnected components.
Triconnectivity::Triconnectivity (const Graph& G) :
	m_ESTACK(G.numberOfEdges())
{
	m_pGC = new GraphCopySimple(G);
	GraphCopySimple &GC = *m_pGC;

	const int n = GC.numberOfNodes();
	const int m = GC.numberOfEdges();

#ifdef TRIC_COMP_OUTPUT
	std::cout << "Dividing G into triconnected components.\n" << std::endl;
	std::cout << "n = " << n << ", m = " << m << std::endl << std::endl;
#endif

	m_component = Array<CompStruct>(3*m-6);
	m_numComp = 0;

	// special cases
	OGDF_ASSERT(n >= 2);
	OGDF_HEAVY_ASSERT(isBiconnected(G));

	if (n <= 2) {
		OGDF_ASSERT(m >= 3);
		CompStruct &C = newComp();
		for (edge e : GC.edges)
			C << e;
		C.m_type = CompType::bond;
		return;
	}

	m_TYPE.init(GC,EdgeType::unseen);
	splitMultiEdges();

	// initialize arrays
	m_NUMBER.init(GC,0); m_LOWPT1.init(GC);
	m_LOWPT2.init(GC);   m_FATHER.init(GC,nullptr);
	m_ND    .init(GC);   m_DEGREE.init(GC);
	m_TREE_ARC.init(GC,nullptr);
	m_NODEAT = Array<node>(1,n);

	m_numCount = 0;
	m_start = GC.firstNode();
	DFS1(GC,m_start,nullptr);

	for (edge e : GC.edges) {
		bool up = (m_NUMBER[e->target()] - m_NUMBER[e->source()] > 0);
		if ((up && m_TYPE[e] == EdgeType::frond) || (!up && m_TYPE[e] == EdgeType::tree))
			GC.reverseEdge(e);
	}

#ifdef TRIC_COMP_OUTPUT
	std::cout << "\nnode\tNUMBER\tFATHER\tLOWPT1\tLOWPT2\tND" << std::endl;
	for (node v : GC.nodes) {
		std::cout << GC.original(v) << ":  \t" << m_NUMBER[v] << "   \t";
		if (m_FATHER[v] == 0) std::cout << "nil \t";
		else std::cout << GC.original(m_FATHER[v]) << "   \t";
		std::cout << m_LOWPT1[v] << "   \t" << m_LOWPT2[v] << "   \t" << m_ND[v] << std::endl;
	}
#endif

	m_A.init(GC);
	m_IN_ADJ.init(GC,nullptr);
	buildAcceptableAdjStruct(GC);

#ifdef TRIC_COMP_OUTPUT
	std::cout << "\nadjaceny lists:" << std::endl;
	for (node v : GC.nodes) {
		std::cout << v << "\t";
		for(edge ei : m_A[v]) {
			printOs(ei);
		}
		std::cout << std::endl;
	}
#endif

	DFS2(GC);

#ifdef TRIC_COMP_OUTPUT
	std::cout << "\nnode\tNEWNUM\tLOWPT1\tLOWPT2\tHIGHPT" << std::endl;
	for (node v : GC.nodes) {
		std::cout << GC.original(v) << ":  \t" << m_NEWNUM[v] << "   \t";
		std::cout << m_LOWPT1[v] << "   \t" << m_LOWPT2[v] << "   \t";
		for(int i : m_HIGHPT[v])
			std::cout << i << " ";
		std::cout << std::endl;
	}

	std::cout << "\nedges starting a path:" << std::endl;
	for (edge e : GC.edges) {
		if (m_START[e]) {
			printOs(e);
		}
	}
#endif


	m_TSTACK_h = new int[2*m+1];
	m_TSTACK_a = new int[2*m+1];
	m_TSTACK_b = new int[2*m+1];
	m_TSTACK_a[m_top = 0] = -1; // start with EOS

	pathSearch(G,m_start);

	// last split component
	CompStruct &C = newComp();
	while(!m_ESTACK.empty()) {
		C << m_ESTACK.popRet();
	}
	C.m_type = (C.m_edges.size() > 4) ? CompType::triconnected : CompType::polygon;

#ifdef TRIC_COMP_OUTPUT
	printStacks();
#endif

	delete[] m_TSTACK_h;
	delete[] m_TSTACK_a;
	delete[] m_TSTACK_b;

	// free resources
	m_NUMBER.init(); m_LOWPT1.init();
	m_LOWPT2.init(); m_FATHER.init();
	m_ND    .init(); m_TYPE  .init();
	m_A     .init(); m_NEWNUM.init();
	m_HIGHPT.init(); m_START .init();
	m_DEGREE.init(); m_TREE_ARC.init();
	m_IN_ADJ.init(); m_IN_HIGH.init();
	m_NODEAT.init();
	m_ESTACK.clear();

	assembleTriconnectedComponents();

	// Caution: checkComp() assumes that the graph is simple!
#if 0
	OGDF_ASSERT(checkComp());
#endif

#ifdef TRIC_COMP_OUTPUT
	std::cout << "\n\nTriconnected components:\n";
	for (int i = 0; i < m_numComp; i++) {
		const List<edge> &L = m_component[i].m_edges;
		if (L.size() == 0) continue;
		std::cout << "[" << i << "] ";
		switch(m_component[i].m_type) {
			case CompType::bond: std::cout << "bond "; break;
			case CompType::polygon: std::cout << "polygon "; break;
			case CompType::triconnected: std::cout << "triconnected "; break;
		}

		for(edge ei : L)
			printOs(ei);
		std::cout << "\n";
	}
#endif
}


// Tests G for triconnectivity and returns a cut vertex in
// s1 or a separation pair in (s1,s2).
Triconnectivity::Triconnectivity(const Graph &G, bool &isTric, node &s1, node &s2)
{
	m_pGC = new GraphCopySimple(G);
	GraphCopySimple &GC = *m_pGC;

	const int n = GC.numberOfNodes();
	const int m = GC.numberOfEdges();

	s1 = s2 = nullptr;

	if (n < 2) {
		isTric = true;
		return;
	} else if (n == 2) {
		isTric = hasNonSelfLoopEdges(G);
		return;
	} else if (m == 0) {
		isTric = false;
		return;
	}

	makeLoopFree(GC);
	makeParallelFreeUndirected(GC);

	// initialize arrays
	m_TYPE.init(GC,EdgeType::unseen);
	m_NUMBER.init(GC,0); m_LOWPT1.init(GC);
	m_LOWPT2.init(GC);   m_FATHER.init(GC,nullptr);
	m_ND    .init(GC);   m_DEGREE.init(GC);
	m_NODEAT.init(1,n);

	m_TREE_ARC.init(GC,nullptr); // probably not required

	m_numCount = 0;
	m_start = GC.firstNode();
	DFS1(GC,m_start,nullptr,s1);

	// graph not even connected?
	if(m_numCount < n) {
		s1 = nullptr; isTric = false;
		return;
	}

	// graph no biconnected?
	if(s1 != nullptr) {
		s1 = GC.original(s1);
		isTric = false; // s1 is a cut vertex
		return;
	}

	for (edge e : GC.edges) {
		bool up = (m_NUMBER[e->target()] - m_NUMBER[e->source()] > 0);
		if ((up && m_TYPE[e] == EdgeType::frond) || (!up && m_TYPE[e] == EdgeType::tree))
			GC.reverseEdge(e);
	}

	m_A.init(GC);
	m_IN_ADJ.init(GC,nullptr);
	buildAcceptableAdjStruct(GC);

	DFS2(GC);

	m_TSTACK_h = new int[m];
	m_TSTACK_a = new int[m];
	m_TSTACK_b = new int[m];
	m_TSTACK_a[m_top = 0] = -1; // start with EOS

	isTric = pathSearch(G,m_start,s1,s2);
	if(s1) {
		s1 = GC.original(s1);
		s2 = GC.original(s2);
	}

	delete[] m_TSTACK_h;
	delete[] m_TSTACK_a;
	delete[] m_TSTACK_b;

	// free resources
	m_NUMBER.init(); m_LOWPT1.init();
	m_LOWPT2.init(); m_FATHER.init();
	m_ND    .init(); m_TYPE  .init();
	m_A     .init(); m_NEWNUM.init();
	m_HIGHPT.init(); m_START .init();
	m_DEGREE.init(); m_TREE_ARC.init();
	m_IN_ADJ.init(); m_IN_HIGH.init();
	m_NODEAT.init();
}


// Splits bundles of multi-edges into bonds and creates
// a new virtual edge in GC.
void Triconnectivity::splitMultiEdges()
{
	GraphCopySimple &GC = *m_pGC;

	SListPure<edge> edges;
	EdgeArray<int> minIndex(GC), maxIndex(GC);
	parallelFreeSortUndirected(GC,edges,minIndex,maxIndex);

	SListIterator<edge> it;
	for (it = edges.begin(); it.valid(); ) {
		edge e = *it;
		int minI = minIndex[e], maxI = maxIndex[e];
		++it;
		if (it.valid() && minI == minIndex[*it] && maxI == maxIndex[*it]) {
			CompStruct &C = newComp(CompType::bond);
			C << GC.newEdge(e->source(),e->target()) << e << *it;
			m_TYPE[e] = m_TYPE[*it] = EdgeType::removed;

			for (++it; it.valid() &&
				minI == minIndex[*it] && maxI == maxIndex[*it];	++it)
			{
				C << *it;
				m_TYPE[*it] = EdgeType::removed;
			}
		}
	}
}

// Checks if computed triconnected components are correct.
bool Triconnectivity::checkSepPair(edge eVirt)
{
	GraphCopySimple G(*m_pGC);

	G.delNode(G.copy(m_pGC->original(eVirt->source())));
	G.delNode(G.copy(m_pGC->original(eVirt->target())));

	return !isConnected(G);
}

bool Triconnectivity::checkComp()
{
	bool ok = true;

	GraphCopySimple &GC = *m_pGC;
	GraphCopySimple GTest(GC.original());

	if (!isLoopFree(GC)) {
		ok = false;
		std::cout << "GC contains loops!" << std::endl;
	}

	EdgeArray<int> count(GC,0);
	for (int i = 0; i < m_numComp; i++) {
		for(edge e : m_component[i].m_edges)
			count[e]++;
	}

	for (edge e : GC.edges) {
		if (GC.original(e) == nullptr) {
			if (count[e] != 2) {
				ok = false;
				std::cout << "virtual edge contained " << count[e];
				printOs(e); std::cout << std::endl;
			}
			if (!checkSepPair(e)) {
				ok = false;
				std::cout << "virtual edge"; printOs(e);
				std::cout << " does not correspond to a sep. pair." << std::endl;
			}

		} else {
			if (count[e] != 1) {
				ok = false;
				std::cout << "real edge contained " << count[e];
				printOs(e); std::cout << std::endl;
			}
		}
	}

	NodeSet<> S(GC);
	NodeArray<node> map(GC);

	for(int i = 0; i < m_numComp; i++) {
		CompStruct &C = m_component[i];
		const List<edge> &L = C.m_edges;
		if (L.size() == 0) continue;

		S.clear();

		for(edge e : L) {
			S.insert(e->source());
			S.insert(e->target());
		}

		const int n = S.size();

		switch(C.m_type) {
		case CompType::bond:
			if (n != 2) {
				ok = false;
				std::cout << "bond [" << i << "] with " << n << " nodes!" << std::endl;
			}
			break;

		case CompType::polygon:
			if (n < 3) {
				ok = false;
				std::cout << "polygon [" << i << "] with " << n << " nodes!" << std::endl;
			}

			if (L.size() != n) {
				ok = false;
				std::cout << "polygon [" << i << "] with " << n << " vertices and " << L.size() << " edges!" << std::endl;

			} else {
				Graph Gp;
				for(node v : S.nodes())
					map[v] = Gp.newNode();
				for(edge e : L)
					Gp.newEdge(map[e->source()],map[e->target()]);

				for (node v : Gp.nodes) {
					if (v->degree() != 2) {
						ok = false;
						std::cout << "polygon [" << i << "] contains node with degree " << v->degree() << std::endl;
					}
				}
				if (!isConnected(Gp)) {
					ok = false;
					std::cout << "polygon [" << i << "] not connected." << std::endl;
				}
			}
			break;

		case CompType::triconnected:
			if (n < 4) {
				ok = false;
				std::cout << "triconnected component [" << i << "] with " << n << " nodes!" << std::endl;
			}

			{
			Graph Gp;
			for(node v : S.nodes())
				map[v] = Gp.newNode();
			for(edge e : L)
				Gp.newEdge(map[e->source()],map[e->target()]);

			if (!isTriconnectedPrimitive(Gp)) {
				ok = false;
				std::cout << "component [" << i << "] not triconnected!" << std::endl;
			}
			if (!isSimple(Gp)) {
				ok = false;
				std::cout << "triconnected component [" << i << "] not simple!" << std::endl;
			}
			}
			break;

		default:
			ok = false;
			std::cout << "component [" << i << "] with undefined type!" << std::endl;
		}
	}

	return ok;
}


// joins bonds and polygons with common virtual edge in
// order to build the triconnected components.
void Triconnectivity::assembleTriconnectedComponents()
{
	GraphCopySimple &GC = *m_pGC;

	EdgeArray<int>       comp1(GC), comp2(GC);
	EdgeArray<ListIterator<edge> > item1(GC,ListIterator<edge>());
	EdgeArray<ListIterator<edge> > item2(GC);

	bool *visited = new bool[m_numComp];

	int i;
	for(i = 0; i < m_numComp; i++) {
		visited[i] = false;
		List<edge> &L = m_component[i].m_edges;

		ListIterator<edge> it;
		for(it = L.begin(); it.valid(); ++it) {
			edge e = *it;
			if (!item1[e].valid()) {
				comp1[e] = i; item1[e] = it;
			} else {
				comp2[e] = i; item2[e] = it;
			}
		}
	}

	for(i = 0; i < m_numComp; i++) {
		CompStruct &C1 = m_component[i];
		List<edge> &L1 = C1.m_edges;
		visited[i] = true;

		if (L1.size() == 0) continue;

		if (C1.m_type == CompType::polygon || C1.m_type == CompType::bond) {
			ListIterator<edge> it, itNext;
			for(it = L1.begin(); it.valid(); it = itNext) {
				itNext = it.succ();
				edge e  = *it;

				if (GC.original(e) != nullptr) continue;

				int j = comp1[e];
				ListIterator<edge> it2;
				if (visited[j]) {
					j = comp2[e];
					if (visited[j]) continue;
					it2 = item2[e];
				} else
					it2 = item1[e];

				CompStruct &C2 = m_component[j];

				if (C2.m_type != C1.m_type) continue;

				visited[j] = true;
				List<edge> &L2 = C2.m_edges;

				L2.del(it2);
				L1.conc(L2);
				if (!itNext.valid())
					itNext = it.succ();
				L1.del(it);

				GC.delEdge(e);
			}
		}
	}

	delete[] visited;
}



// The first dfs-search
//  computes NUMBER[v], FATHER[v], LOWPT1[v], LOWPT2[v],
//           ND[v], TYPE[e], DEGREE[v]
void Triconnectivity::DFS1 (const Graph& G, node v, node u)
{
	m_NUMBER[v] = ++m_numCount;
	m_FATHER[v] = u;
	m_DEGREE[v] = v->degree();

	m_LOWPT1[v] = m_LOWPT2[v] = m_NUMBER[v];
	m_ND[v] = 1;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();

		if (m_TYPE[e] != EdgeType::unseen)
			continue;

		node w = e->opposite(v);

		if (m_NUMBER[w] == 0) {
			m_TYPE[e] = EdgeType::tree;

			m_TREE_ARC[w] = e;

			DFS1(G,w,v);

			if (m_LOWPT1[w] < m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT1[v],m_LOWPT2[w]);
				m_LOWPT1[v] = m_LOWPT1[w];

			} else if (m_LOWPT1[w] == m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_LOWPT2[w]);

			} else {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_LOWPT1[w]);
			}

			m_ND[v] += m_ND[w];

		} else {

			m_TYPE[e] = EdgeType::frond;

			if (m_NUMBER[w] < m_LOWPT1[v]) {
				m_LOWPT2[v] = m_LOWPT1[v];
				m_LOWPT1[v] = m_NUMBER[w];

			} else if (m_NUMBER[w] > m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_NUMBER[w]);
			}
		}
	}
}

void Triconnectivity::DFS1 (const Graph& G, node v, node u, node &s1)
{
	node firstSon = nullptr;

	m_NUMBER[v] = ++m_numCount;
	m_FATHER[v] = u;
	m_DEGREE[v] = v->degree();

	m_LOWPT1[v] = m_LOWPT2[v] = m_NUMBER[v];
	m_ND[v] = 1;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();

		if (m_TYPE[e] != EdgeType::unseen)
			continue;

		node w = e->opposite(v);

		if (m_NUMBER[w] == 0) {
			m_TYPE[e] = EdgeType::tree;
			if(firstSon == nullptr) firstSon = w;

			m_TREE_ARC[w] = e;

			DFS1(G,w,v,s1);

			// check for cut vertex
			if(m_LOWPT1[w] >= m_NUMBER[v] && (w != firstSon || u != nullptr))
				s1 = v;

			if (m_LOWPT1[w] < m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT1[v],m_LOWPT2[w]);
				m_LOWPT1[v] = m_LOWPT1[w];

			} else if (m_LOWPT1[w] == m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_LOWPT2[w]);

			} else {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_LOWPT1[w]);
			}

			m_ND[v] += m_ND[w];

		} else {

			m_TYPE[e] = EdgeType::frond;

			if (m_NUMBER[w] < m_LOWPT1[v]) {
				m_LOWPT2[v] = m_LOWPT1[v];
				m_LOWPT1[v] = m_NUMBER[w];

			} else if (m_NUMBER[w] > m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_NUMBER[w]);
			}
		}
	}
}


// Construction of ordered adjaceny lists
void Triconnectivity::buildAcceptableAdjStruct(const Graph& G)
{
	int max = 3*G.numberOfNodes()+2;
	Array<List<edge> > BUCKET(1,max);

	for (edge e : G.edges) {
		EdgeType t = m_TYPE[e];
		if (t == EdgeType::removed) continue;

		node w = e->target();
		int phi = (t == EdgeType::frond) ? 3*m_NUMBER[w]+1 : (
			(m_LOWPT2[w] < m_NUMBER[e->source()]) ? 3*m_LOWPT1[w] :
			3*m_LOWPT1[w]+2);
		BUCKET[phi].pushBack(e);
	}

	for (int i = 1; i <= max; i++) {
		for (edge e : BUCKET[i])
			m_IN_ADJ[e] = m_A[e->source()].pushBack(e);
	}
}


// The second dfs-search
void Triconnectivity::pathFinder(const Graph& G, node v)
{
	m_NEWNUM[v] = m_numCount - m_ND[v] + 1;

	for(edge e : m_A[v]) {
		node w = e->opposite(v);

		if (m_newPath) {
			m_newPath = false;
			m_START[e] = true;
		}

		if (m_TYPE[e] == EdgeType::tree) {
			pathFinder(G,w);
			m_numCount--;

		} else {
			m_IN_HIGH[e] = m_HIGHPT[w].pushBack(m_NEWNUM[v]);
			m_newPath = true;
		}
	}
}

void Triconnectivity::DFS2 (const Graph& G)
{
	m_NEWNUM .init(G,0);
	m_HIGHPT .init(G);
	m_IN_HIGH.init(G,ListIterator<int>());
	m_START  .init(G,false);

	m_numCount = G.numberOfNodes();
	m_newPath = true;

	pathFinder(G,m_start);

	Array<int> old2new(1,G.numberOfNodes());

	for (node v : G.nodes)
		old2new[m_NUMBER[v]] = m_NEWNUM[v];

	for (node v : G.nodes) {
		m_NODEAT[m_NEWNUM[v]] = v;
		m_LOWPT1[v] = old2new[m_LOWPT1[v]];
		m_LOWPT2[v] = old2new[m_LOWPT2[v]];
	}
}


// recognition of split components
void Triconnectivity::pathSearch (const Graph& G, node v)
{
	edge e;
	int y, vnum = m_NEWNUM[v];
	int a, b;

	List<edge> &Adj = m_A[v];
	int outv = Adj.size();

	ListIterator<edge> it, itNext;
	for(it = Adj.begin(); it.valid(); it=itNext)
	{
		itNext = it.succ();
		e = *it;
		node w = e->target();
		int wnum = m_NEWNUM[w];

		if (m_TYPE[e] == EdgeType::tree) {

			if (m_START[e]) {
				y = 0;
				if (m_TSTACK_a[m_top] > m_LOWPT1[w]) {
					do {
						y = max(y,m_TSTACK_h[m_top]);
						b = m_TSTACK_b[m_top--];
					} while (m_TSTACK_a[m_top] > m_LOWPT1[w]);
					TSTACK_push(y,m_LOWPT1[w],b);
				} else {
					TSTACK_push(wnum+m_ND[w]-1,m_LOWPT1[w],vnum);
				}
				TSTACK_pushEOS();
			}

			pathSearch(G,w);

			m_ESTACK.push(m_TREE_ARC[w]);  // add (v,w) to ESTACK (can differ from e!)

			node x;

			while (vnum != 1 && ((m_TSTACK_a[m_top] == vnum) ||
				(m_DEGREE[w] == 2 && m_NEWNUM[m_A[w].front()->target()] > wnum)))
			{
				a = m_TSTACK_a[m_top];
				b = m_TSTACK_b[m_top];

				edge eVirt;

				if (a == vnum && m_FATHER[m_NODEAT[b]] == m_NODEAT[a]) {
					m_top--;
				}

				else {
					edge e_ab = nullptr;

					if (m_DEGREE[w] == 2 && m_NEWNUM[m_A[w].front()->target()] > wnum) {
#ifdef TRIC_COMP_OUTPUT
						std::cout << std::endl << "\nfound type-2 separation pair " <<
							m_pGC->original(v) << ", " <<
							m_pGC->original(m_A[w].front()->target());
#endif

						edge e1 = m_ESTACK.popRet();
						edge e2 = m_ESTACK.popRet();
						m_A[w].del(m_IN_ADJ[e2]);

						x = e2->target();

						eVirt = m_pGC->newEdge(v,x);
						m_DEGREE[x]--; m_DEGREE[v]--;

						OGDF_ASSERT(e2->source() == w);
						CompStruct &C = newComp(CompType::polygon);
						C << e1 << e2 << eVirt;

						if (!m_ESTACK.empty()) {
							e1 = m_ESTACK.top();
							if (e1->source() == x && e1->target() == v) {
								e_ab = m_ESTACK.popRet();
								m_A[x].del(m_IN_ADJ[e_ab]);
								delHigh(e_ab);
							}
						}

					} else {
#ifdef TRIC_COMP_OUTPUT
						std::cout << "\nfound type-2 separation pair " <<
							m_pGC->original(m_NODEAT[a]) << ", " <<
							m_pGC->original(m_NODEAT[b]);
#endif

						int h = m_TSTACK_h[m_top--];

						CompStruct &C = newComp();
						while(true) {
							edge xy = m_ESTACK.top();
							x = xy->source();
							node xyTarget = xy->target();
							if (!(a <= m_NEWNUM[x] && m_NEWNUM[x] <= h &&
								a <= m_NEWNUM[xyTarget] && m_NEWNUM[xyTarget] <= h)) break;

							if ((m_NEWNUM[x] == a && m_NEWNUM[xyTarget] == b) ||
								(m_NEWNUM[xyTarget] == a && m_NEWNUM[x] == b))
							{
								e_ab = m_ESTACK.popRet();
								m_A[e_ab->source()].del(m_IN_ADJ[e_ab]);
								delHigh(e_ab);

							} else {
								edge eh = m_ESTACK.popRet();
								if (it != m_IN_ADJ[eh]) {
									m_A[eh->source()].del(m_IN_ADJ[eh]);
									delHigh(eh);
								}
								C << eh;
								m_DEGREE[x]--; m_DEGREE[xyTarget]--;
							}
						}

						eVirt = m_pGC->newEdge(m_NODEAT[a],m_NODEAT[b]);
						C.finishTricOrPoly(eVirt);
						x = m_NODEAT[b];
					}

					if (e_ab != nullptr) {
						CompStruct &C = newComp(CompType::bond);
						C << e_ab << eVirt;

						eVirt = m_pGC->newEdge(v,x);
						C << eVirt;

						m_DEGREE[x]--; m_DEGREE[v]--;
					}

					m_ESTACK.push(eVirt);
					*it = eVirt;
					m_IN_ADJ[eVirt] = it;

					m_DEGREE[x]++; m_DEGREE[v]++;
					m_FATHER[x] = v;
					m_TREE_ARC[x] = eVirt;
					m_TYPE[eVirt] = EdgeType::tree;

					w = x; wnum = m_NEWNUM[w];
				}
			}

			if (m_LOWPT2[w] >= vnum && m_LOWPT1[w] < vnum && (m_FATHER[v] != m_start || outv >= 2))
			{
#ifdef TRIC_COMP_OUTPUT
				std::cout << "\nfound type-1 separation pair " <<
					m_pGC->original(m_NODEAT[m_LOWPT1[w]]) << ", " <<
					m_pGC->original(v);
#endif

				CompStruct &C = newComp();
				int xx;
				OGDF_ASSERT(!m_ESTACK.empty()); // otherwise undefined behavior since x is not initialized
				while (!m_ESTACK.empty()) {
					edge xy = m_ESTACK.top();
					xx = m_NEWNUM[xy->source()];
					y = m_NEWNUM[xy->target()];

					if (!((wnum <= xx && xx < wnum+m_ND[w]) || (wnum <= y && y < wnum+m_ND[w])))
						break;

					C << m_ESTACK.popRet();
					delHigh(xy);
					m_DEGREE[m_NODEAT[xx]]--; m_DEGREE[m_NODEAT[y]]--;
				}

				edge eVirt = m_pGC->newEdge(v,m_NODEAT[m_LOWPT1[w]]);
				C.finishTricOrPoly(eVirt);

				if ((xx == vnum && y == m_LOWPT1[w]) || (y == vnum && xx == m_LOWPT1[w])) {
					CompStruct &compBond = newComp(CompType::bond);
					edge eh = m_ESTACK.popRet();
					if (m_IN_ADJ[eh] != it) {
						m_A[eh->source()].del(m_IN_ADJ[eh]);
					}
					compBond << eh << eVirt;
					eVirt = m_pGC->newEdge(v,m_NODEAT[m_LOWPT1[w]]);
					compBond << eVirt;
					m_IN_HIGH[eVirt] = m_IN_HIGH[eh];
					m_DEGREE[v]--;
					m_DEGREE[m_NODEAT[m_LOWPT1[w]]]--;
				}

				if (m_NODEAT[m_LOWPT1[w]] != m_FATHER[v]) {
					m_ESTACK.push(eVirt);
					*it = eVirt;
					m_IN_ADJ[eVirt] = it;
					if (!m_IN_HIGH[eVirt].valid() && high(m_NODEAT[m_LOWPT1[w]]) < vnum)
						m_IN_HIGH[eVirt] = m_HIGHPT[m_NODEAT[m_LOWPT1[w]]].pushFront(vnum);

					m_DEGREE[v]++;
					m_DEGREE[m_NODEAT[m_LOWPT1[w]]]++;

				} else {
					Adj.del(it);

					CompStruct &compBond = newComp(CompType::bond);
					compBond << eVirt;
					eVirt = m_pGC->newEdge(m_NODEAT[m_LOWPT1[w]],v);
					compBond << eVirt;

					edge eh = m_TREE_ARC[v];

					compBond << m_TREE_ARC[v];

					m_TREE_ARC[v] = eVirt;
					m_TYPE[eVirt] = EdgeType::tree;

					m_IN_ADJ[eVirt] = m_IN_ADJ[eh];
					*m_IN_ADJ[eh] = eVirt;
				}
			}

			if (m_START[e]) {
				while (TSTACK_notEOS()) {
					m_top--;
				}
				m_top--;
			}

			while (TSTACK_notEOS() &&
				m_TSTACK_b[m_top] != vnum && high(v) > m_TSTACK_h[m_top]) {
				m_top--;
			}

			outv--;

		} else { // frond arc
			if (m_START[e]) {
				y = 0;
				if (m_TSTACK_a[m_top] > wnum) {
					do {
						y = max(y,m_TSTACK_h[m_top]);
						b = m_TSTACK_b[m_top--];
					} while (m_TSTACK_a[m_top] > wnum);
					TSTACK_push(y,wnum,b);
				} else {
					TSTACK_push(vnum,wnum,vnum);
				}
			}

			m_ESTACK.push(e);  // add (v,w) to ESTACK
		}
	}
}

// simplified path search for triconnectivity test
bool Triconnectivity::pathSearch (const Graph &G, node v, node &s1, node &s2)
{
	edge e;
	int y, vnum = m_NEWNUM[v];
	int a, b;

	List<edge> &Adj = m_A[v];
	int outv = Adj.size();

	ListIterator<edge> it, itNext;
	for(it = Adj.begin(); it.valid(); it=itNext)
	{
		itNext = it.succ();
		e = *it;
		node w = e->target();
		int wnum = m_NEWNUM[w];

		if (m_TYPE[e] == EdgeType::tree) {

			if (m_START[e]) {
				y = 0;
				if (m_TSTACK_a[m_top] > m_LOWPT1[w]) {
					do {
						y = max(y,m_TSTACK_h[m_top]);
						b = m_TSTACK_b[m_top--];
					} while (m_TSTACK_a[m_top] > m_LOWPT1[w]);
					TSTACK_push(y,m_LOWPT1[w],b);
				} else {
					TSTACK_push(wnum+m_ND[w]-1,m_LOWPT1[w],vnum);
				}
				TSTACK_pushEOS();
			}

			if(!pathSearch(G,w,s1,s2))
				return false;

			while (vnum != 1 && ((m_TSTACK_a[m_top] == vnum) ||
				(m_DEGREE[w] == 2 && m_NEWNUM[m_A[w].front()->target()] > wnum)))
			{
				a = m_TSTACK_a[m_top];
				b = m_TSTACK_b[m_top];

				if (a == vnum && m_FATHER[m_NODEAT[b]] == m_NODEAT[a]) {
					m_top--;

				} else if (m_DEGREE[w] == 2 && m_NEWNUM[m_A[w].front()->target()] > wnum)
				{
					s1 = v;
					s2 = m_A[w].front()->target();
					return false;

				} else {
					s1 = m_NODEAT[a];
					s2 = m_NODEAT[b];
					return false;
				}
			}

			if (m_LOWPT2[w] >= vnum && m_LOWPT1[w] < vnum && (m_FATHER[v] != m_start || outv >= 2))
			{
				s1 = m_NODEAT[m_LOWPT1[w]];
				s2 = v;
				return false;
			}

			if (m_START[e]) {
				while (TSTACK_notEOS()) {
					m_top--;
				}
				m_top--;
			}

			while (TSTACK_notEOS() &&
				m_TSTACK_b[m_top] != vnum && high(v) > m_TSTACK_h[m_top]) {
				m_top--;
			}

			outv--;

		} else { // frond arc
			if (m_START[e]) {
				y = 0;
				if (m_TSTACK_a[m_top] > wnum) {
					do {
						y = max(y,m_TSTACK_h[m_top]);
						b = m_TSTACK_b[m_top--];
					} while (m_TSTACK_a[m_top] > wnum);
					TSTACK_push(y,wnum,b);
				} else {
					TSTACK_push(vnum,wnum,vnum);
				}
			}
		}
	}

	return true;
}


// triconnectivity test
bool isTriconnected(const Graph &G, node &s1, node &s2)
{
	bool isTric;
	Triconnectivity tric2(G,isTric,s1,s2);

	return isTric;
}


// debugging stuff
void Triconnectivity::printOs(edge e)
{
#ifdef TRIC_COMP_OUTPUT
	std::cout << " (" << m_pGC->original(e->source()) << "," <<
		m_pGC->original(e->target()) << "," << e->index() << ")";
	if (m_pGC->original(e) == 0) std::cout << "v";
#endif
}

void Triconnectivity::printStacks()
{
#ifdef TRIC_COMP_OUTPUT
	std::cout << "\n\nTSTACK:" << std::endl;

	for (int i = m_top; i >= 0; i--)
		std::cout << "(" << m_TSTACK_h[i] << "," << m_TSTACK_a[i] << "," << m_TSTACK_b[i] << ")\n";

	std::cout << "\nESTACK\n";
	while(!m_ESTACK.empty()) {
		printOs(m_ESTACK.popRet());
		std::cout << std::endl;
	}
#endif
}

}
