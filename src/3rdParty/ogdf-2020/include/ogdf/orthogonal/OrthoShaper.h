/** \file
 * \brief Computes the orthogonal representation of a planar
 *        representation of a UML graph using the simple flow
 *        approach.
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
#include <ogdf/uml/PlanRepUML.h>


namespace ogdf {

class OGDF_EXPORT OrthoShaper
{
public:
	//! Types of network nodes: nodes and faces
	enum class NetworkNodeType { low, high, inner, outer };

	OrthoShaper() {
		setDefaultSettings();
	}

	~OrthoShaper() { }

	// Given a planar representation for a UML graph and its planar
	// combinatorial embedding, call() produces an orthogonal
	// representation using Tamassias bend minimization algorithm
	// with a flow network where every flow unit defines 90 degree angle
	// in traditional mode.

	void call(PlanRepUML &PG,
		CombinatorialEmbedding &E,
		OrthoRep &OR,
		bool fourPlanar = true);

	void call(PlanRep &PG,
		CombinatorialEmbedding &E,
		OrthoRep &OR,
		bool fourPlanar = true);

	//sets the default settings used in the standard constructor
	void setDefaultSettings()
	{
		m_distributeEdges = true; // true;  //try to distribute edges to all node sides
		m_fourPlanar      = true;  //do not allow zero degree angles at high degree
		m_allowLowZero    = false; //do allow zero degree at low degree nodes
		m_multiAlign      = true;//true;  //start/end side of multi edges match
		m_traditional     = true;//true;  //prefer 3/1 flow at degree 2 (false: 2/2)
		m_deg4free        = false; //allow free angle assignment at degree four
		m_align           = false; //align nodes on same hierarchy level
		m_startBoundBendsPerEdge = 0; //don't use bound on bend number per edge
	}

	// returns option distributeEdges
	bool distributeEdges() { return m_distributeEdges; }
	// sets option distributeEdges to b
	void distributeEdges(bool b) { m_distributeEdges = b; }

	// returns option multiAlign
	bool multiAlign() { return m_multiAlign; }
	// sets option multiAlign to b
	void multiAlign(bool b) { m_multiAlign = b; }

	// returns option traditional
	bool traditional() { return m_traditional; }
	// sets option traditional to b
	void traditional(bool b) { m_traditional = b; }

	//returns option deg4free
	bool fixDegreeFourAngles() { return m_deg4free; }
	//sets option deg4free
	void fixDegreeFourAngles(bool b) { m_deg4free = b; }

	//alignment of brothers in hierarchies
	void align(bool al) {m_align = al;}
	bool align() {return m_align;}

	//! Set bound for number of bends per edge (none if set to 0). If shape
	//! flow computation is unsuccessful, the bound is increased iteratively.
	void setBendBound(int i){ OGDF_ASSERT(i >= 0); m_startBoundBendsPerEdge = i;}
	int getBendBound(){return m_startBoundBendsPerEdge;}

private:
	//! distribute edges among all sides if degree > 4
	bool m_distributeEdges;

	//! should the input graph be four planar
	//! (no zero degree)
	bool m_fourPlanar;

	//! allow low degree nodes zero degree
	//! (to low for zero...)
	bool m_allowLowZero;

	//! multi edges aligned on the same side
	bool m_multiAlign;

	//! allow degree four nodes free angle assignment
	bool m_deg4free;

	/**
	 * Do not prefer 180-degree angles.
	 * Traditional is not tamassia,
	 * traditional is a kandinsky-ILP-like network with node supply 4,
	 * not traditional interprets angle flow zero as 180 degree, "flow
	 * through the node"
	 */
	bool m_traditional;

	//! Try to achieve an alignment in hierarchy levels
	bool m_align;

	/**
	 * Bound on the number of bends per edge for flow.
	 * If == 0, no bound is used.
	 *
	 * A maximum number of bends per edge can be specified in
	 * m_startBoundBendsPerEdge. If the algorithm is not successful in
	 * producing a bend minimal representation subject to
	 * startBoundBendsPerEdge, it successively enhances the bound by
	 * one trying to compute an orthogonal representation.
	 *
	 * Using m_startBoundBendsPerEdge may not produce a bend minimal
	 * representation in general.
	 */
	int m_startBoundBendsPerEdge;

	//! Set angle boundary.
	//! Warning: sets upper AND lower bounds, therefore may interfere with existing bounds
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
