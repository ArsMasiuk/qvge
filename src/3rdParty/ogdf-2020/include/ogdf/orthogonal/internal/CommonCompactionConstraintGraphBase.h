/** \file
 * \brief Declares ogdf::CommonCompactionConstraintGraphBase
 *
 * \author Stephan Beyer, Carsten Gutwenger, Karsten Klein
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

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/planarity/PlanRep.h>

namespace ogdf {

//! Types of edges in the constraint graph
enum class ConstraintEdgeType {
	BasicArc,
	VertexSizeArc,
	VisibilityArc,
	FixToZeroArc, //!< can be compacted to zero length, can be fixed
	ReducibleArc, //!< can be compacted to zero length
	MedianArc //!< inserted to replace some reducible in fixzerolength
};

//! A common base class for ogdf::GridCompactionConstraintGraphBase
//! and ogdf::CompactionConstraintGraphBase
class CommonCompactionConstraintGraphBase : protected Graph {
protected:
	const OrthoRep *m_pOR;
	const PlanRep *m_pPR;

	NodeArray<SListPure<node>> m_path; //!< list of nodes contained in a segment
	NodeArray<node> m_pathNode; //!< segment containing a node in PG
	EdgeArray<edge> m_edgeToBasicArc; //!< basic arc representing an edge in PG

	EdgeArray<int> m_cost; //!< cost of an edge
	EdgeArray<ConstraintEdgeType> m_type; //!< constraint type for each edge

	EdgeArray<int> m_border; //!< only used for cage precompaction in flowcompaction computecoords

	//! Node does not represent drawing node as we dont have positions
	//! we save a drawing representant and an offset
	NodeArray<bool> m_extraNode; //!< true iff node does not represent drawing node
	NodeArray<node> m_extraRep; //!< existing representant of extranodes position anchor

	OrthoDir m_arcDir;
	OrthoDir m_oppArcDir;

	NodeArray<edge> m_originalEdge; //!< save edge for the basic arcs

	SList<node> m_sources;
	SList<node> m_sinks;

	virtual string getLengthString(edge e) const = 0;

	//! Build constraint graph with basic arcs
	CommonCompactionConstraintGraphBase(const OrthoRep &OR, const PlanRep &PG, OrthoDir arcDir, int costAssoc);

public:
	//! Returns underlying graph
	//! @{
	const Graph &getGraph() const { return (const Graph&)*this; }
	Graph &getGraph() { return (Graph&)*this; }
	//! @}
	//! @{

	//! Returns underlying OrthoRep
	const OrthoRep &getOrthoRep() const {
		return *m_pOR;
	}

	const PlanRep& getPlanRep() const {
		return *m_pPR;
	}

	//! Returns list of nodes contained in segment \p v
	//! @pre \p v is in the constraint graph
	const SListPure<node> &nodesIn(node v) const {
		return m_path[v];
	}

	//! Returns the segment (path node in constraint graph) containing \p v
	//! @pre \p v is a node in the associated planarized representation
	node pathNodeOf(node v) const {
		return m_pathNode[v];
	}

	//! Returns cost of edge \p e
	//! @pre \p e is an edge in the constraint graph
	int cost(edge e) const {
		return m_cost[e];
	}

	//! Returns extraNode existing anchor representant
	node extraRep(node v) const {
		return m_extraRep[v];
	}

	//! Returns true if edge lies on cage border
	bool onBorder(edge e) const {
		return m_border[e] > 0;
	}

	//! Returns true  if edge is subject to length fixation if length < sep
	bool fixOnBorder(edge e) const {
		return m_border[e] == 2;
	}

	//! @}

	//! Embeds constraint graph such that all sources and sinks lie in a common face
	void embed();

	//! Returns constraint arc representing input edge e in constraint graph
	edge basicArc(edge e) const {
		return m_edgeToBasicArc[e];
	}

	//! Computes topological numbering on the segments of the constraint graph
	void computeTopologicalSegmentNum(NodeArray<int> &topNum);

	//! Removes "arcs" from \p visibArcs which we already have in the
	//! constraint graph (as basic arcs)
	void removeRedundantVisibArcs(SListPure<Tuple2<node, node>> &visibArcs);

#ifdef OGDF_DEBUG

	/**
	 * Writes GML output (for debugging)
	 *
	 * Output in GML format with special edge colouring:
	 * Arcs with cost 0 are green, other arcs are red.
	 */
	//! @{
	void writeGML(const char *fileName) const;
	void writeGML(std::ostream &os) const;
	void writeGML(const char *fileName, const NodeArray<bool> &one) const;
	void writeGML(std::ostream &os, const NodeArray<bool> &one) const;
	//! @}

#endif

	/**
	 * Returns type of edge \p e
	 *
	 * \pre \p e is an edge in the constraint graph
	 */
	ConstraintEdgeType typeOf(edge e) const {
		return m_type[e];
	}

	//! Returns node status
	bool extraNode(node v) const {
		return m_extraNode[v];
	}
};

}
