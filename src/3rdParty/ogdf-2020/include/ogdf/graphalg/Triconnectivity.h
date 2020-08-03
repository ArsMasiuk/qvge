/** \file
 * \brief Declares class Triconnectivity which realizes the Hopcroft/Tarjan
 * algorithm for finding the triconnected components of a biconnected
 * multi-graph.
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

#pragma once

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/Array.h>
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

//! realizes Hopcroft/Tarjan algorithm for finding the triconnected
//! components of a biconnected multi-graph
class OGDF_EXPORT Triconnectivity
{
public:
	/**
	 * Divides G into triconnected components.
	 * \param G graph
	 */
	explicit Triconnectivity(const Graph &G);

	/**
	 * Tests G for triconnectivity.
	 * \param G graph
	 * \param isTric true if G is triconnected, false otherwise.
	 * \param s1 first vertex of separation pair if G is biconnected, cut vertex of G if G is not biconnected, nullptr if G is not connected.
	 * \param s2 second vertex of separation pair if G is biconnected, nullptr otherwise.
	 */
	Triconnectivity(const Graph &G, bool &isTric, node &s1, node &s2);

	Triconnectivity(const Triconnectivity &) = delete;

	~Triconnectivity();

	//! type of split-components / triconnected components
	enum class CompType { bond, polygon, triconnected };

	//! representation of a component
	struct CompStruct {
		List<edge> m_edges;
		CompType m_type;

		CompStruct &operator<<(edge e) {
			m_edges.pushBack(e);
			return *this;
		}

		void finishTricOrPoly(edge e) {
			m_edges.pushBack(e);
			m_type = (m_edges.size() >= 4) ? CompType::triconnected : CompType::polygon;
		}
	};

	//! copy of G containing also virtual edges
	GraphCopySimple  *m_pGC;
	//! array of components
	Array<CompStruct> m_component;
	//! number of components
	int m_numComp;

	/**
	 * Checks if computed triconnected componets are correct.
	 * \pre checkComp() assumes that the graph is simple!
	 */
	bool checkComp();


private:
	bool checkSepPair(edge eVirt);

	//! splits bundles of multi-edges into bonds and creates
	//! a new virtual edge in GC.
	void splitMultiEdges();

	//! stack of triples
	int *m_TSTACK_h, *m_TSTACK_a, *m_TSTACK_b;
	int m_top;

	//! push a triple on TSTACK
	void TSTACK_push (int h, int a, int b) {
		m_TSTACK_h[++m_top] = h;
		m_TSTACK_a[m_top] = a;
		m_TSTACK_b[m_top] = b;
	}

	//! push end-of-stack marker on TSTACK
	void TSTACK_pushEOS() {
		m_TSTACK_a[++m_top] = -1;
	}

	//! returns true iff end-of-stack marker is not on top of TSTACK
	bool TSTACK_notEOS() {
		return m_TSTACK_a[m_top] != -1;
	}

	//! create a new empty component
	CompStruct &newComp() {
		return m_component[m_numComp++];
	}

	//! create a new empty component of type t
	CompStruct &newComp(CompType t) {
		CompStruct &C = m_component[m_numComp++];
		C.m_type = t;
		return C;
	}

	//! type of edges with respect to palm tree
	enum class EdgeType { unseen, tree, frond, removed };

	//! first dfs traversal
	void DFS1 (const Graph& G, node v, node u);
	//! special version for triconnectivity tes
	void DFS1 (const Graph& G, node v, node u, node &s1);

	//! constructs ordered adjaceny lists
	void buildAcceptableAdjStruct (const Graph& G);
	//! the second dfs traversal
	void DFS2 (const Graph& G);
	void pathFinder(const Graph& G, node v);

	//! finding of split components
	void pathSearch (const Graph& G, node v);

	bool pathSearch (const Graph &G, node v, node &s1, node &s2);

	//! merges split-components into triconnected components
	void assembleTriconnectedComponents();

	//! debugging stuff
	void printOs(edge e);
	void printStacks();

	//! returns high(v) value
	int high(node v) {
		return (m_HIGHPT[v].empty()) ? 0 : m_HIGHPT[v].front();
	}

	void delHigh(edge e) {
		ListIterator<int> it = m_IN_HIGH[e];
		if (it.valid()) {
			node v = e->target();
			m_HIGHPT[v].del(it);
		}
	}

	NodeArray<int>   m_NUMBER; //!< (first) dfs-number of v
	NodeArray<int>   m_LOWPT1;
	NodeArray<int>   m_LOWPT2;
	NodeArray<int>   m_ND;     //!< number of descendants in palm tree
	NodeArray<int>   m_DEGREE; //!< degree of v
	Array<node>      m_NODEAT; //!< node with number i
	NodeArray<node> m_FATHER;  //!< father of v in palm tree
	EdgeArray<EdgeType> m_TYPE; //!< type of edge e
	NodeArray<List<edge> > m_A; //!< adjacency list of v
	NodeArray<int> m_NEWNUM; //!< (second) dfs-number of v
	EdgeArray<bool> m_START; //!< edge starts a path
	NodeArray<edge> m_TREE_ARC; //!< tree arc entering v
	NodeArray<List<int> > m_HIGHPT; //!< list of fronds entering v in the order they are visited
	EdgeArray<ListIterator<edge>> m_IN_ADJ; //!< pointer to element in adjacency list containing e
	EdgeArray<ListIterator<int>> m_IN_HIGH; //!< pointer to element in HIGHPT list containing e
	ArrayBuffer<edge> m_ESTACK; //!< stack of currently active edges

	node m_start;     //!< start node of dfs traversal
	int  m_numCount;  //!< counter for dfs-traversal
	bool m_newPath;   //!< true iff we start a new path
};

}
