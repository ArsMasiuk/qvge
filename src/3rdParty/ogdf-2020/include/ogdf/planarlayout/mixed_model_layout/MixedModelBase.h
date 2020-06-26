/** \file
 * \brief Base functionality of Mixed-Model layout algorithm
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

#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/GridLayout.h>
#include <ogdf/augmentation/AugmentationModule.h>
#include <ogdf/planarlayout/ShellingOrderModule.h>
#include <ogdf/planarity/EmbedderModule.h>
#include <ogdf/planarlayout/mixed_model_layout/MMOrder.h>
#include <ogdf/planarlayout/mixed_model_layout/IOPoints.h>

namespace ogdf {

class MixedModelBase
{
public:
	MixedModelBase(PlanRep &PG, GridLayout &gridLayout) :
		m_PG(PG), m_gridLayout(gridLayout), m_iops(PG) { }

	virtual ~MixedModelBase() { }

	//! Computes the ordered partition (incl. #m_leftOp[k], em_rightOp[k]) and
	//! constructs the in- and outpoint lists.
	void computeOrder(
		AugmentationModule &augmenter,
		EmbedderModule *pEmbedder,
		adjEntry adjExternal,
		ShellingOrderModule &compOrder);

	//! Computes the relative coordinates of the in- and outpoints, incl. height(v), depth(v).
	void assignIopCoords();

	//! Implements the placement step. Computes x[v] and y[v].
	void placeNodes();

	//! Computes the absolute x-coordinates x[v] of all nodes in the ordering,
	//! furthermore dyla[k] and dyra[k] (used by compute_y_coordinates)
	void computeXCoords();

	//! Computes the absolute y-coordinates y[v] of all nodes in the ordering.
	void computeYCoords();

	//! Assigns polylines to edges of the original graph and computes the x-
	//! and y-coordinates of deg-1-nodes not in the ordering.
	void setBends();

	//! Tries to reduce the number of bends by changing the outpoints of nodes with indeg and outdeg 2.
	void postprocessing1();

	//! Tries to reduce the number of bends by moving degree-2 nodes on bend points.
	void postprocessing2();


	//! Functions for debugging output
	//! @{
	void printMMOrder(std::ostream &os);
	void printInOutPoints(std::ostream &os);
	void print(std::ostream &os, const InOutPoint &iop);
	void printNodeCoords(std::ostream &os);
	//! @}

	MixedModelBase &operator=(const MixedModelBase &) = delete;

private:
	PlanRep &m_PG;

	GridLayout &m_gridLayout;

	MMOrder  m_mmo;
	IOPoints m_iops;
	ArrayBuffer<PlanRep::Deg1RestoreInfo> m_deg1RestoreStack;

	Array<int> m_dyl, m_dyr;
	Array<ListConstIterator<InOutPoint> > m_leftOp, m_rightOp;
	NodeArray<ListConstIterator<InOutPoint> > m_nextLeft, m_nextRight;
	NodeArray<int> m_dxla, m_dxra;


	bool exists(adjEntry adj) {
		return !m_PG.isDummy(adj->theEdge());
	}

	//! Determine if the kth set in the ordered partition has a "real" left vertex
	bool hasLeft (int k) const;

	//! Determine if the kth set in the ordered partition has a "real" right vertex
	bool hasRight(int k) const;

	//! Removes degree-1 nodes and store informations for restoring them.
	void removeDeg1Nodes();

	void firstPoint(int &x, int &y, adjEntry adj);
	bool isRedundant(int x1, int y1, int x2, int y2, int x3, int y3);
};

}
