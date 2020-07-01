/** \file
 * \brief Declaration of the class BoyerMyrvoldInit
 *
 * \author Jens Schmidt
 *
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

#include <random>
#include <limits>
#include <ogdf/planarity/boyer_myrvold/BoyerMyrvoldPlanar.h>
#include <ogdf/basic/List.h>

namespace ogdf {
namespace boyer_myrvold {

//! This class is used in the Boyer-Myrvold planarity test for preprocessing purposes.
/**
 * Among these is the computation of lowpoints, highestSubtreeDFIs,
 * separatedDFSChildList and of course building the DFS-tree.
 */
class BoyerMyrvoldInit {
public:
	//! Constructor, the parameter BoyerMyrvoldPlanar is needed
	explicit BoyerMyrvoldInit(BoyerMyrvoldPlanar* pBM);

	//! Destructor
	~BoyerMyrvoldInit() { }

	//! Creates the DFSTree
	void computeDFS();

	//! Computes lowpoint, highestSubtreeDFI and links virtual to nonvirtual vertices
	void computeLowPoints();

	//! Computes the list of separated DFS children for all nodes
	void computeDFSChildLists();

	// avoid automatic creation of assignment operator
	//! Assignment operator is undefined!
	BoyerMyrvoldInit &operator=(const BoyerMyrvoldInit &);

private:
	//! The input graph
	Graph& m_g;

	//! Some parameters... see BoyerMyrvold.h for further instructions
	const int& m_embeddingGrade;
	const double& m_randomness;
	const EdgeArray<int> *m_edgeCosts;
	std::minstd_rand m_rand;

	//! Link to non-virtual vertex of a virtual Vertex.
	/** A virtual vertex has negative DFI of the DFS-Child of related non-virtual Vertex
	 */
	NodeArray<node>& m_realVertex;

	//! The one and only DFI-Array
	NodeArray<int>& m_dfi;

	//! Returns appropriate node from given DFI
	Array<node>& m_nodeFromDFI;

	//! Links to opposite adjacency entries on external face in clockwise resp. ccw order
	/** m_link[0]=CCW, m_link[1]=CW
	 */
	NodeArray<adjEntry> (&m_link)[2];

	//! The adjEntry which goes from DFS-parent to current vertex
	NodeArray<adjEntry>& m_adjParent;

	//! The DFI of the least ancestor node over all backedges
	/** If no backedge exists, the least ancestor is the DFI of that node itself
	 */
	NodeArray<int>& m_leastAncestor;

	//! Contains the type of each edge
	EdgeArray<BoyerMyrvoldEdgeType>& m_edgeType;

	//! The lowpoint of each node
	NodeArray<int>& m_lowPoint;

	//! The highest DFI in a subtree with node as root
	NodeArray<int>& m_highestSubtreeDFI;

	//! A list to all separated DFS-children of node
	/** The list is sorted by lowpoint values (in linear time)
	*/
	NodeArray<ListPure<node> >& m_separatedDFSChildList;

	//! Pointer to node contained in the DFSChildList of his parent, if exists.
	/** If node isn't in list or list doesn't exist, the pointer is set to nullptr.
	*/
	NodeArray<ListIterator<node> >& m_pNodeInParent;

	//! Creates and links a virtual vertex of the node belonging to \p father
	void createVirtualVertex(const adjEntry father);
};

}
}
