/** \file
 * \brief Declaration of a base class for planar representations
 *        of graphs and cluster graphs.
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/basic/GraphCopy.h>


namespace ogdf {


/**
 * \brief Upward planarized representations (of a connected component) of a graph.
 * The upward planarization representation is a single source single sink graph.
 * The single source is s_hat and the single sink is t_hat.
 * s_hat is connected with the sources of the original graph. This muss be done before
 * creating of a instance of UpwardPlanRep. The super sink t_hat is contructed in this class.
 * For technical reason we contruct a sink t and connect the sink of the original graph
 * with t. Then we connect t with t_hat. The edge (t,t_hat) is called the external face handle.
 * Because the right face of the adjEntry of this edge should be the external face.
 */
class OGDF_EXPORT UpwardPlanRep : public GraphCopy
{
public:

	friend class SubgraphUpwardPlanarizer;

#if 0
	//debug only
	friend class FixedEmbeddingUpwardEdgeInserter;
#endif

	/* @{
	 * \brief Creates a planarized representation with respect to \p Gamma.
	 * Gamma muss be an upward planar embedding with a fixed ext. face
	 * Precondition: the graph is a single source graph
	 */

	explicit UpwardPlanRep(const CombinatorialEmbedding &Gamma); //upward planar embedding with a fixed ext. face

	UpwardPlanRep(const GraphCopy &GC, // must be upward embedded and single source
		adjEntry adj_ext); // the right face of this adjEntry is the external face

	//! copy constructor
	UpwardPlanRep(const UpwardPlanRep &UPR);

	//! standart constructor
	UpwardPlanRep(): GraphCopy(), isAugmented(false), t_hat(nullptr), s_hat(nullptr), extFaceHandle(nullptr), crossings(0) // multisources(false)
	{
		m_Gamma.init(*this);
		m_isSinkArc.init(*this, false);
		m_isSourceArc.init(*this, false);
	}

	virtual ~UpwardPlanRep() {}

	//! same as insertEdgePath, but assumes that the graph is embedded
	void insertEdgePathEmbedded(
		edge eOrig,
		SList<adjEntry> crossedEdges,
		EdgeArray<int> &cost);

	//! convert to a single source single sink graph (result is not necessary a st-graph!).
	// pred. the graph muss be a sinlge source graph
	// We construct node t and connect the sink-switches with t. The new arcs are sSinkArc.
	// For simplicity we construct an additional edge (t,t_hat) (the extFaceArc), where t_hat is the super sink.
	void augment();

	//! return true if graph is augmented to a single source single sink graph
	bool augmented() const { return isAugmented; }

	//! return the upward planar embedding
	const CombinatorialEmbedding & getEmbedding() const {return m_Gamma;}

	CombinatorialEmbedding & getEmbedding() {return m_Gamma;}

	node getSuperSink() const {return t_hat;}

	node getSuperSource() const {return s_hat;}

	int numberOfCrossings() const {return crossings;}

	//! Assignment operator.
	UpwardPlanRep &operator=(const UpwardPlanRep &copy);

	bool isSinkArc(edge e) const {return m_isSinkArc[e];}

	bool isSourceArc(edge e) const {return m_isSourceArc[e];}

	//! 0 if node v is not a sink switch (not the top sink switch !!) of an internal face.
	//! else v is sink-switch of the right face of the adjEntry.
	adjEntry sinkSwitchOf(node v) {return m_sinkSwitchOf[v];}

	//! return the adjEntry of v which right face is f.
	adjEntry getAdjEntry(const CombinatorialEmbedding &Gamma, node v, face f) const;

	//return the left in edge of node v.
	adjEntry leftInEdge(node v) const
	{
		if (v->indeg() == 0)
			return nullptr;

		adjEntry adjFound = nullptr;
		for(adjEntry adj : v->adjEntries) {
			if (adj->theEdge()->target() == v && adj->cyclicSucc()->theEdge()->source() == v) {
				adjFound = adj;
				break;
			}
		}
		return adjFound;
	}

	// debug
	void outputFaces(const CombinatorialEmbedding &embedding) const {
		std::cout << std::endl << "Face UPR " << std::endl;
		for (face f : embedding.faces) {
			std::cout << "face " << f->index() << ": ";
			adjEntry adjNext = f->firstAdj();
			do {
				std::cout << adjNext->theEdge() << "; ";
				adjNext = adjNext->faceCycleSucc();
			} while (adjNext != f->firstAdj());
			std::cout << std::endl;
		}
		if (embedding.externalFace() != nullptr)
			std::cout << "ext. face of the graph is: " << embedding.externalFace()->index() << std::endl;
		else
			std::cout << "no ext. face set." << std::endl;
	}


protected:

	bool isAugmented; //!< the UpwardPlanRep is augmented to a single source and single sink graph

	CombinatorialEmbedding m_Gamma; //! < embedding og this UpwardPlanRep

	node t_hat; //!< the super sink

	node s_hat; //!< the super source

	// sinkArk are edges which are added to transform the original graph to single sink graph.
	// note: the extFaceHandle is a sink arc.
	EdgeArray<bool> m_isSinkArc;

	// source arc are edges which are added to transform the original graph to a single source graph
	EdgeArray<bool> m_isSourceArc;

	// 0 if node v is not a non-top-sink-switch of a internal face.
	// else v is (non-top) sink-switch of f (= right face of adjEntry).
	NodeArray<adjEntry> m_sinkSwitchOf;

	adjEntry extFaceHandle; // the right face of this adjEntry is always the ext. face

	int crossings;


private:
	void computeSinkSwitches();

	//! only for planarizer !!!
	void initMe();

	void copyMe(const UpwardPlanRep &UPR);

	void removeSinkArcs(SList<adjEntry> &crossedEdges);

	void constructSinkArcs(face f, node t);
};

}
