/** \file
 * \brief Implements the class BitonicOrdering...
 *
 * ... that computes a bitonic st ordering as described by Gronemann
 * in Bitonic st-orderings of biconnected planar graphs
 *
 * \author Martin Gronemann
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

#include <ogdf/planarlayout/BitonicOrdering.h>

using namespace ogdf;

BitonicOrdering::BitonicOrdering(Graph& G, adjEntry adj_st_edge)
  : m_graph(G)
  , m_currLabel(0)
  , m_orderIndex(G,-1)
  , m_indexToNode(G.numberOfNodes())
  , m_tree(G, adj_st_edge->theEdge(), true)
{
	// set all tree nodes to non flipped
	m_flipped.init(m_tree.tree(), false);

	// s in the graph
	node s_G = adj_st_edge->theNode();
	node t_G = adj_st_edge->twinNode();

	// we label s here manually: set the label
	m_orderIndex[s_G] = m_currLabel++;
	// and update the other map
	m_indexToNode[m_orderIndex[s_G]] = s_G;

	// label everything else except t
	handleCase(m_tree.rootNode());

	// we label s here manually: set the label
	m_orderIndex[t_G] = m_currLabel++;
	// and update the other map
	m_indexToNode[m_orderIndex[t_G]] = t_G;

	// finally embedd G
	m_tree.embed(m_graph);
}


// used to distinguish between the three cases below
void BitonicOrdering::handleCase(node v_T)
{
	// check if this skeleton has been flipped in some R-node
	// above temporarily
	if (isFlipped(v_T)) {
		// reverse embedding of the skeleton
		m_tree.reverse(v_T);
	}

	// the switch statement to check what type of node
	// v_T is
	switch (m_tree.typeOf(v_T)) {
		case ogdf::SPQRTree::NodeType::SNode:
			handleSerialCase(v_T);
			break;

		case ogdf::SPQRTree::NodeType::PNode:
			handleParallelCase(v_T);
			break;

		case ogdf::SPQRTree::NodeType::RNode:
			handleRigidCase(v_T);
			break;

		default:
			break;
	}

	// if we flipped it before, we now reverse the reversing
	if (isFlipped(v_T)) {
		m_tree.reverse(v_T);
	}

}

// helper function that finds the st-adjEntry in the skeleton of v_T
adjEntry BitonicOrdering::getAdjST(node v_T) const
{
	// the reference edge in G_skel
	edge e_ref = m_tree.skeleton(v_T).referenceEdge();

	// it is either e_ref's source or target
	// we try this one first
	adjEntry adj_st = e_ref->adjSource();

	// by our invariant s is labeled but t not
	// hence check if source is labeled
	if (getLabel(v_T, adj_st->theNode()) < 0) {
		// it is not, then it is the other one.
		adj_st = adj_st->twin();
	}

	// this is the element pointing from s to t
	return adj_st;
}

// the S-node case
void BitonicOrdering::handleSerialCase(node v_T)
{
	// the skeleton instance of the tree node
	const Skeleton& skel = m_tree.skeleton(v_T);

	// the adjElement pointing from s to t
	adjEntry adj_st = getAdjST(v_T);

	// the t of st
	node t = adj_st->twinNode();

	// now we traverse the cycle of skel counter clockwise from s to t
	for (adjEntry adj = adj_st->cyclicSucc();
			adj != adj_st->twin();
			adj = adj->twin()->cyclicSucc()) {
		// the current edge of the cycle
		edge e = adj->theEdge();

		// check if this is a virtual one, i.e. is there something we
		// have to label first?
		if (skel.isVirtual(e)) {
			// the corresponding child of v_T
			node w_T = skel.twinTreeNode(e);

			// mark the child as flipped iff this node is flipped
			setFlipped(w_T, isFlipped(v_T));

			// go and label those nodes first
			handleCase(w_T);
		}

		// the node which can be s
		node v = adj->twinNode();

		// assign v a label unless it is t.
		if (v != t) assignLabel(v_T, v);
	}
}

// the P-node case
void BitonicOrdering::handleParallelCase(node v_T)
{
	// the skeleton instance of the tree node
	const Skeleton& skel = m_tree.skeleton(v_T);

	// the adjElement pointing from s to t
	adjEntry adj_st = getAdjST(v_T);

	// a possible real st edge
	adjEntry adj_real = nullptr;

	// first we check if there is any real edge here
	for (adjEntry adj = adj_st->cyclicPred(); adj != adj_st; adj = adj->cyclicPred()) {
		// check if it is a real edge
		if (!skel.isVirtual(adj->theEdge())
		 && adj != adj_st->cyclicSucc()) {
			// we are only interested in a real edge that is not the cyclic succ of the reference adj
			adj_real = adj; // we found one, this is moved
		}
	}

	// we have to change the embedding now
	if (adj_real) {
		// swap it with the one right after the st edge
		m_tree.swap(v_T, adj_st->cyclicSucc(), adj_real);
	}

	// for all incident edges in reverse order
	for (adjEntry adj = adj_st->cyclicPred(); adj != adj_st; adj = adj->cyclicPred()) {
		// the edge
		edge e = adj->theEdge();

		// check if this is a virtual one, i.e. is there something we
		// have to label first?
		if (skel.isVirtual(e)) {
			// the corresponding child of v_T
			node w_T = skel.twinTreeNode(e);

			// mark the child as flipped iff this node is flipped
			setFlipped(w_T, isFlipped(v_T));

			// go and label those nodes first
			handleCase(w_T);
		}
	}
}

// transforms the result of the canonical ordering into two arrays,
// one holding the index in the temporary order for a node,
// the other is the reverse map. Function assumes proper init for indices and vertices
void BitonicOrdering::partitionToOrderIndices(const List<List<node> >& partitionlist,
                             NodeArray<int>& indices,
                             Array<node>& vertices) const
{
	// counter for the index of a node
	int currIndex = 0;

	// for all parititons
	for (List<List<node> >::const_iterator lit = partitionlist.begin(); lit.valid(); ++lit) {
		// a chain or singleton
		const List<node>& partition = *lit;

		// for all nodes in the chain or singleton
		for (List<node>::const_iterator it = partition.begin(); it.valid(); ++it) {
			// the node
			node v = *it;

			// assign it a number
			indices[v] = currIndex;

			// mark it in the map
			vertices[currIndex] = v;

			// increment the index
			currIndex++;
		}
	}
}

// the R-node case
void BitonicOrdering::handleRigidCase(node v_T)
{
	// the skeleton instance of the tree node
	const Skeleton& skel = m_tree.skeleton(v_T);

	// the adjElement pointing from s to t
	adjEntry adj_st = getAdjST(v_T);

	// s and t
	node s = adj_st->theNode();
	node t = adj_st->twinNode();

	// the skeleton graph of v_T
	const Graph& G_skel = skel.getGraph();

	// canonical ordeirng
	LeftistOrdering leftistOrder;

	// the result of the leftist order
	List<List<node> > temporaryPartition;

	// get the order
	leftistOrder.call(G_skel, adj_st, temporaryPartition);

	// index for every skeleton node
	NodeArray<int> vertexIndex(G_skel, -1);

	// the temporary order
	Array<node> order(G_skel.numberOfNodes());

	// partition to order indices
	partitionToOrderIndices(temporaryPartition, vertexIndex, order);

	// now traverse the order from 1 to V
	for (int i = 0; i < G_skel.numberOfNodes(); ++i) {
		// the ith node
		node w = order[i];

		// for all incident edges
		for (adjEntry adj = w->firstAdj(); adj; adj = adj->succ()) {
			// the neighbour
			node v = adj->twinNode();

			// check if this is an incoming edge in the temporary order
			if (vertexIndex[v] < vertexIndex[w] ) {
				// yes it is, w is the successor of v
				// time for some recursion
				edge e = adj->theEdge();

				// check if this is a virtual one, i.e. is there something we
				// have to label first? however, make sure not to recurse on the parent
				if (skel.isVirtual(e) && ( e != skel.referenceEdge())) {
					// the corresponding child of v_T
					node w_T = skel.twinTreeNode(e);

					// the successor of w in the embedding clockwise at v
					node w_next = adj->twin()->cyclicSucc()->twinNode();

					// we now check if the successor of w at v has a higher index then w
					// i.e. w is in the increasing partition at v and requires a flip in the embeddig
					// however, we have to be careful at s to not wrap around (the st edge is the cyclicSucc
					// of the v_1,v_2 edge. we just skip v = s = v_1 completly since it is
					// decreasing by corallary 1
					if ((vertexIndex[v] > 0) && (vertexIndex[w] < vertexIndex[w_next])) {
						// w is in the increasing partition
						// we mark the child as flipped if we are not flipped
						setFlipped(w_T, !isFlipped(v_T));
						// reverseRecursive(skel.twinTreeNode(e));
					} else {
						// mark the child as flipped iff this node is flipped
						setFlipped(w_T, isFlipped(v_T));
					}

					// go and label those nodes first
					handleCase(w_T);
				}
			}
		}

		//now we are ready to label v
		if ((w != t) && (w != s)) assignLabel(v_T, w);
	}
}

#ifdef OGDF_DEBUG
void BitonicOrdering::consistencyCheck(GraphAttributes& GA) const
{
	GA.init(m_graph, GraphAttributes::nodeLabel);
	bool allBitonic = true;

	for (node v : m_graph.nodes) {
		std::stringstream str;
		str << v << "(" << m_orderIndex[v] << ")";
		GA.label(v) = str.str();

		if (m_orderIndex[v] < 0) {
			Logger::slout() << "Node not assigned" << std::endl;
		}
	}

	// for all nodes we check now for bitonic indices
	for (node v : m_graph.nodes) {
		bool isNodeBitonic = true;
		// we skip t and s though
		if (m_orderIndex[v] != 0 && m_orderIndex[v] != m_graph.numberOfNodes() - 1) {
			adjEntry adj_first_succ = nullptr;
			adjEntry adj_last_succ = nullptr;


			for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
				// and its cyclic succ
				node w_prev = adj->cyclicPred()->twinNode();
				// the other node
				node w = adj->twinNode();
				// and its cyclic succ
				node w_next = adj->cyclicSucc()->twinNode();

				if ((m_orderIndex[v] > m_orderIndex[w_prev]) && (m_orderIndex[v] < m_orderIndex[w]))
					adj_first_succ = adj;

				if ((m_orderIndex[v] > m_orderIndex[w_next]) && (m_orderIndex[v] < m_orderIndex[w]))
					adj_last_succ = adj;

			}


			// we are going to look for bitonic succ lists
			for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
				// and its cyclic succ
				node w_prev = adj->cyclicPred()->twinNode();
				// the other node
				node w = adj->twinNode();
				// and its cyclic succ
				node w_next = adj->cyclicSucc()->twinNode();

				if (m_orderIndex[v] <= max(m_orderIndex[w_prev], max(m_orderIndex[w], m_orderIndex[w_next]))) {
					// all succs, lets check for bitonic indices

					if ((m_orderIndex[w_prev] >= m_orderIndex[w]) && (m_orderIndex[w_next] >= m_orderIndex[w]) &&
						(m_orderIndex[v] > 0)) {
						isNodeBitonic = false;
						Logger::slout()
								<< "[BitonicOrder:] " << "NOT BITONIC SUCC LIST " << v << "(" << m_orderIndex[v] << ")"
								<< std::endl
								<< "[BitonicOrder:] " << w_prev << "(" << m_orderIndex[w_prev] << ") "
								<< w << "(" << m_orderIndex[w] << ") "
								<< w_next << "(" << m_orderIndex[w_next] << ")" << std::endl
								<< std::endl;
					};
				}
			}

			if (!isNodeBitonic) {
				for (adjEntry adj = adj_first_succ; adj != adj_last_succ->cyclicSucc(); adj = adj->cyclicSucc()) {
					Logger::slout() << "(" << m_orderIndex[adj->twinNode()] << ") ";
				}
			}

			OGDF_ASSERT(allBitonic && isNodeBitonic);
		}
	}
}
#endif
