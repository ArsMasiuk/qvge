/** \file
 * \brief A declaration of EdgeStandardRep class representing a graph
 *        representation of a hypergraph in the edge standard form.
 *
 * This class provides a kind of an intermediate repsenetation of a
 * hypergraph between Hypergraph and edge standard based layout classes.
 * It is derived from HypergraphObserver and provides some additional
 * functionality to handle edge standard representation of hypergraph.
 * It follows Observer design pattern.
 *
 * \author Ondrej Moris
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

#include <ogdf/basic/Graph_d.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/hypergraph/Hypergraph.h>
#include <ogdf/hypergraph/HypergraphArray.h>
#include <ogdf/hypergraph/HypergraphObserver.h>

namespace ogdf {

//! Enumeration class of possible edge standard representations.
enum class EdgeStandardType {
	//! no new dummy nodes are introduced, for every hyperedge
	//! \a e = (\a v_1, ..., \a v_l), we add a cliqie \a K_l connecting
	//! hypernodes incident with \a e
	clique = 0x0001,
	//! for every hyperedge \a e = {\a v_1, ..., \a v_l} a single new dummy
	//! node \a v_e is introduced, moreover, \a v_e becomes the center of
	//! a new star connecting all hypernodes incident with e (ie.
	//! {\a v_1, \a v_e}, ..., {\a v_l, \a v_e} are added)
	star   = 0x0002,
	//! for every hyperedge \a e a minimal subcubic tree connecting all
	//! hypernodes incident with e together is added with all its
	//! nodes and edges, leaves of tree are hypernodes, all non-leaf
	//! nodes are newly introduced dummy nodes.
	tree   = 0x0003
};

//! Edge standard representation of hypergraphs.
class OGDF_EXPORT EdgeStandardRep : public HypergraphObserver
{
private:

	//! The type of edge standard representation.
	EdgeStandardType m_type;

	//! The reference to the original hypergraph.
	const Hypergraph *m_hypergraph;

	//! Edge standard representation of the hypergraph.
	Graph m_graphRep;

	//! The map from representation nodes to hypernodes.
	NodeArray<hypernode> m_hypernodeMap;

	//! The map from representation hypernodes to nodes.
	HypernodeArray<node> m_nodeMap;

	//! The map from representation edge to hyperedges.
	EdgeArray<hyperedge> m_hyperedgeMap;

	//! The map from representation hyperedge to edges.
	HyperedgeArray<List<edge> > m_edgeMap;

	//! The list of all newly created nodes.
	List<node> m_dummyNodes;

public:

	//! Creates an edge standard representation.
	EdgeStandardRep();

	//! Creates an edge standard rep. of a given \p pType associated with \p pH.
	EdgeStandardRep(const Hypergraph &pH, EdgeStandardType pType);

	//! Destructor.
	virtual ~EdgeStandardRep();

	//! Clears all cluster data.
	void clear();

	//! Conversion to original hypergraph reference.
	const Hypergraph & hypergraph() const
	{
		return *m_hypergraph;
	}

	//! Returns a reference to the representation graph.
	const Graph & constGraph() const
	{
		return m_graphRep;
	}

	//! Returns the type of edge standard representation.
	EdgeStandardType type() const {
		return m_type;
	}

	//! Returns the node associated with the hypernode.
	node nodeMap(hypernode v)
	{
		return m_nodeMap[v];
	}

	//! Returns the hypernode associated with the node (if any).
	hypernode hypernodeMap(node v)
	{
		return m_hypernodeMap[v];
	}

	//! Returns the list of edges associated with the hyperedge.
	const List<edge> & edgeMap(hyperedge e)
	{
		return m_edgeMap[e];
	}

	//! Returns the hyperedge associated with the edge.
	hyperedge hyperedgeMap(edge e)
	{
		return m_hyperedgeMap[e];
	}

	//! Returns the list of dummy nodes.
	const List<node> & dummyNodes() const
	{
		return m_dummyNodes;
	}

protected:

	//! Hypernode removal reaction.
	virtual void hypernodeDeleted(hypernode v) override;

	//! Hypernode addition reaction.
	virtual void hypernodeAdded(hypernode v) override;

	//! Hyperedge removal reaction.
	virtual void hyperedgeDeleted(hyperedge e) override;

	//! Hyperedge addition reaction.
	virtual void hyperedgeAdded(hyperedge e) override;

	//! Hypergraph clean-up reaction.
	virtual void cleared() override;

private:

	void constructCliqueRep();

	void constructStarRep();

	void constructTreeRep();

	void hyperedgeToTree(hyperedge e, int degree);

	void hyperedgeToClique(hyperedge e);

	void cloneHypernodes();
};

}
