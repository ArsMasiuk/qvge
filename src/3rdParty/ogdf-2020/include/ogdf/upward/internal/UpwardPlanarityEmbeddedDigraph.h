/** \file
 * \brief Declaration of class UpwardPlanarityEmbeddedDigraph, which implements
 *        the upward-planarity testing algorithm for
 *        digraphs with a fixed embedding by Bertolazzi et al.
 *
 * \author Robert Zeranski
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
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/CombinatorialEmbedding.h>

namespace ogdf {

class UpwardPlanarityEmbeddedDigraph {
public:
	//constructor
	explicit UpwardPlanarityEmbeddedDigraph(const Graph &H);

private:
	//copy of the Input-Graph
	const Graph &m_G;
	//super-source and super-sink of the flow-network
	node m_s, m_t;
	//flow-network
	Graph m_B;
	//combinatorial embedding of G
	const ConstCombinatorialEmbedding m_combEmb;
	//flow-values of the faces
	FaceArray<int> m_A;
	//annotations for the construction of the flow-network
	FaceArray< List<node> > m_assignedSourcesAndSinks;
	NodeArray<node> 	  	m_correspondingSourceOrSink;
	NodeArray<face> 	  	m_correspondingFace;
	FaceArray<node> 	  	m_correspondingFaceNode;
	NodeArray<edge> 	  	m_correspondingEdge;

public:
	//tests whether the embedded Digraph is upward planar by using the private class methods
	//returns true iff G is upward planar observing the fixed embedding
	bool isUpwardPlanarEmbedded();
	//get the set of feasible external faces (represented by the first AdjEntry) if G is upward planar observing the fixed embedding
	bool isUpwardPlanarEmbedded(List<adjEntry> &possibleExternalFaces);

private:
	//tests whether the embedded Digraph is upward planar
	//val = true forces a break if the first feasible external face was found
	void isUpwardPlanarEmbedded(const bool val, List<adjEntry> &possibleExternalFaces);
	//constructs flow-network of the corresponding Graph G
	void constructNetwork(EdgeArray<int> &capacity, EdgeArray<int> &flow);
	//tests whether a flow of power r is possible in the flow-network by executing augmentation steps
	bool isFlow(EdgeArray<int> &capacity, EdgeArray<int> &flow, const int r);
	//returns a feasible augmentation path
	void getPath(ArrayBuffer<node> &st, EdgeArray<int> &capacity, EdgeArray<int> &flow);
	//returns the value for one augmentation step
	int getMin(ArrayBuffer<node> stack, EdgeArray<int> &capacity, EdgeArray<int> &flow);
};

}
