/** \file
 * \brief Computes the Orthogonal Representation of a Planar
 * Representation of a UML Graph using the simple flow
 * approach.
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

#pragma once

#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/cluster/ClusterPlanRep.h>


namespace ogdf {


//! Computes the orthogonal representation of a clustered graph.
/**
 * @ingroup gd-cluster
 */
class OGDF_EXPORT ClusterOrthoShaper
{

public:

	enum class BendCost { defaultCost, topDownCost, bottomUpCost };
	enum class n_type { low, high, inner, outer }; // types of network nodes: nodes and faces

	ClusterOrthoShaper() {
		m_distributeEdges = true;  //!< try to distribute edges to all node sides
		m_fourPlanar      = true;  //!< do not allow zero degree angles at high degree
		m_allowLowZero    = false; //!< do allow zero degree at low degree nodes
		m_multiAlign      = true;  //!< true;  //start/end side of multi edges match
		m_traditional     = true;  //!< true;  //if set to true, prefer 3/1 flow at degree 2 (false: 2/2)
		m_deg4free        = false; //!< if set to true, free angle assignment at degree four nodes allowed
		m_align           = false; //!< if set to true, nodes are aligned on same hierarchy level
		m_topToBottom     = BendCost::defaultCost;     //bend costs depend on edges cluster depth
	};

	~ClusterOrthoShaper() { }

	// Given a planar representation for a UML graph and its planar
	// combinatorial embedding, call() produces an orthogonal
	// representation using Tamassias bend minimization algorithm
	// with a flow network where every flow unit defines 90 degree angle
	// in traditional mode.
	// A maximum number of bends per edge can be specified in
	// startBoundBendsPerEdge. If the algorithm is not successful in
	// producing a bend minimal representation subject to
	// startBoundBendsPerEdge, it successively enhances the bound by
	// one trying to compute an orthogonal representation.
	//
	// Using startBoundBendsPerEdge may not produce a bend minimal
	// representation in general.
	void call(ClusterPlanRep &PG,
		CombinatorialEmbedding &E,
		OrthoRep &OR,
		int startBoundBendsPerEdge = 0,
		bool fourPlanar = true);


	/// returns option distributeEdges
	bool distributeEdges() { return m_distributeEdges; }
	/// sets option distributeEdges to b
	void distributeEdges(bool b) { m_distributeEdges = b; }

	/// returns option multiAlign
	bool multiAlign() { return m_multiAlign; }
	/// sets option multiAlign to b
	void multiAlign(bool b) { m_multiAlign = b; }

	/// returns option for traditional angle distribution
	bool traditional() { return m_traditional; }
	/// sets option traditional to b
	void traditional(bool b) { m_traditional = b; }

	/// returns option for free angle assignment at degree four nodes
	bool fixDegreeFourAngles() { return m_deg4free; }
	/// sets option for free angle assignment at degree four nodes
	void fixDegreeFourAngles(bool b) { m_deg4free = b; }

	//alignment of brothers in hierarchies
	void align(bool al) { m_align = al; }
	bool align() { return m_align; }

	void bendCostTopDown(BendCost i) { m_topToBottom = i; }

	//return cluster dependant bend cost for standard cost pbc
	int clusterProgBendCost(int clDepth, int treeDepth, int pbc)
	{
		int cost = 1;
		switch (m_topToBottom)
		{
		case BendCost::topDownCost:
			cost = pbc*(clDepth+1); //safeInt
			break;
		case BendCost::bottomUpCost:
			cost = pbc*(treeDepth - clDepth + 1); //safeInt
			break;
		default: //defaultCost
			cost = pbc;
			break;
		}

#if 0
		std::cout << "   Cost/pbc: " << cost << "/" << pbc << "\n";
#endif

		return cost;
	}

	//this is a try: I dont know why this was never implemented for traditional,
	//maybe because of the unit cost for traditional bends vs. the highbound
	//cost for progressive bends
	//return cluster dependant bend cost for standard cost pbc
	//preliminary same as progressive
	int clusterTradBendCost(int clDepth, int treeDepth, int pbc)
	{
		int cost = 1;
		switch (m_topToBottom)
		{
		case BendCost::topDownCost:
			cost = pbc*(clDepth+1); //safeInt
			break;
		case BendCost::bottomUpCost:
			cost = pbc*(treeDepth - clDepth + 1); //safeInt
			break;
		default: //defaultCost
			cost = pbc;
			break;
		}

		return cost;
	}

private:
	bool m_distributeEdges; // distribute edges among all sides if degree > 4
	bool m_fourPlanar;      // should the input graph be four planar (no zero degree)
	bool m_allowLowZero;    // allow low degree nodes zero degree (to low for zero...)
	bool m_multiAlign;      // multi edges aligned on the same side
	bool m_deg4free;        // allow degree four nodes free angle assignment
	bool m_traditional;     // do not prefer 180 degree angles, traditional is not tamassia,
	// traditional is a kandinsky - ILP - like network with node supply 4,
	// not traditional interprets angle flow zero as 180 degree, "flow
	// through the node"
	bool m_align;           //try to achieve an alignment in hierarchy levels

	BendCost m_topToBottom;      //change bend costs on cluster hierarchy levels

	//set angle boundary
	//warning: sets upper AND lower bounds, therefore may interfere with existing bounds
	void setAngleBound(
		edge netArc,
		int angle,
		EdgeArray<int>& lowB,
		EdgeArray<int>& upB,
		EdgeArray<edge>& aTwin,
		bool maxBound = true)
	{
		// preliminary
		OGDF_ASSERT(!m_traditional);

		const int angleId = angle / 90;
		const edge e2 = aTwin[netArc];

		OGDF_ASSERT(angleId >= 0);
		OGDF_ASSERT(angleId <= 2);

		if (maxBound) {
			lowB[netArc] = 2 - angleId;
			upB[netArc] = 2;

			if (e2) {
				upB[e2] = lowB[e2] = 0;
			}
		} else {
			upB[netArc] = 2 - angleId;
			lowB[netArc] = 0;

			if (e2) {
				upB[e2] = 2;
				lowB[e2] = 0;
			}
		}
	}
};

}
