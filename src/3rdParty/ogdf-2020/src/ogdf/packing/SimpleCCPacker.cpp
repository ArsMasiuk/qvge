/** \file
 * \brief Definition of ogdf::SimpleCCPacker.
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

#include <ogdf/packing/SimpleCCPacker.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/packing/TileToRowsCCPacker.h>

using namespace ogdf;

void SimpleCCPacker::computeBoundingBox(const GraphAttributes& graphAttributes, DPoint& min_coord, DPoint& max_coord )
{
	// make access easy
	const Graph& graph = graphAttributes.constGraph();

	// no nodes, no work, life is easy, we can go home
	if (!graph.numberOfNodes())
		return;

	// initialize with some values inside the bounding box (take the first node for that)
	max_coord.m_x = min_coord.m_x = graphAttributes.x( graph.firstNode() );
	max_coord.m_y = min_coord.m_y = graphAttributes.y( graph.firstNode() );

	// iterate over all nodes and update the min max coords
	for (node v = graph.firstNode(); v; v = v->succ()) {
		// left border of the node
		if (( graphAttributes.x(v) -  graphAttributes.width(v)*0.5 ) < min_coord.m_x )
			min_coord.m_x = ( graphAttributes.x(v) -  graphAttributes.width(v)*0.5 );

		// right border
		if (( graphAttributes.x(v) +  graphAttributes.width(v)*0.5 ) > max_coord.m_x )
			max_coord.m_x = ( graphAttributes.x(v) +  graphAttributes.width(v)*0.5 );

		// bottom border
		if (( graphAttributes.y(v) -  graphAttributes.height(v)*0.5 ) < min_coord.m_y )
			min_coord.m_y = ( graphAttributes.y(v) -  graphAttributes.height(v)*0.5 );

		// top border
		if (( graphAttributes.y(v) +  graphAttributes.height(v)*0.5 ) > max_coord.m_y )
			max_coord.m_y = ( graphAttributes.y(v) +  graphAttributes.height(v)*0.5 );
	};
}


void SimpleCCPacker::call(GraphAttributes& graphAttributes)
{
	// the graph to decompose
	const Graph& graph = graphAttributes.constGraph();

	// this is the most easy case...
	if ( !graph.numberOfNodes() )
		return;

	// connected component index for each node of
	// the original graph
	NodeArray<int> ccIndex(graph, -1);

	// number of connected components
	int numCCs = connectedComponents(graph, ccIndex);

	// special case no decomposition required
	// take a short cut to avoid the copy
	if (numCCs == 1) {
		// call the sub layout
		m_pSubLayoutModule->call(graphAttributes);

		// and return without wasting any more time/mem
		return;
	}

	// the corresponding node in the CC graph map
	NodeArray<node> node2CCNode(graph, nullptr);

	// array of all connected component graphs
	Graph** ccGraph = new Graph*[ numCCs ];

	// array of all connected component graphs
	GraphAttributes** ccGraphAttributes = new GraphAttributes*[ numCCs ];

	// allocate for each CC a Graph and GraphAttributes instance
	for (int i = 0; i < numCCs; i++) {
		ccGraph[i] = new Graph();
		// init with attribute flags (not whats the best idea here)
		// First option would be graphAttributes.attributes() which requires
		// more mem (especially when it comes to all the string based stuff)
		// Second option is only necessary stuff. However, the algorithm might require something
		// like edgeWeight or something like that.
		ccGraphAttributes[i] = new GraphAttributes( *ccGraph[i],
				graphAttributes.attributes() );
	}

	// create for each node a representative in the
	// corresponding cc graph
	for (node v = graph.firstNode(); v; v = v->succ()) {
		// the cc index of v
		int i = ccIndex[v];

		// create the node
		node cv = node2CCNode[v] = ccGraph[ i ]->newNode();

		// copy the attributes
		ccGraphAttributes[i]->x(cv) = graphAttributes.x(v);
		ccGraphAttributes[i]->y(cv) = graphAttributes.y(v);
		ccGraphAttributes[i]->width(cv) = graphAttributes.width(v);
		ccGraphAttributes[i]->height(cv) = graphAttributes.height(v);
	}

	// create for each edge an edge in the corresponding
	// cc graph (do we need a map here too? )
	for (edge e = graph.firstEdge(); e; e = e->succ()) {
		// create the edge
		ccGraph[ ccIndex[ e->target() ] ]->newEdge( node2CCNode[ e->source() ],
				node2CCNode[ e->target() ] );
	}

	// lower left corner of the current bounding box.
	// The current bounding box is required later when moving the nodes
	// to their new position
	Array<DPoint> boundingBoxOffset( numCCs );
	// size of the different bounding boxes
	Array<DPoint> boundingBoxSize( numCCs );
	// The new offset (lower left corner) calculated by the pack algorithm
	Array<DPoint> boundingBoxOffsetPacker( numCCs );

	// For each connected component:
	// Calculate a new layout using the sub layout module and the new bounding box.
	for (int i = 0; i < numCCs; i++) {
		// run the pSubLayoutModule
		// this might be unset because someone uses this class for packing only
		if (m_pSubLayoutModule)
			m_pSubLayoutModule->call( *ccGraphAttributes[i] );

		// min and max temp variables for this cc
		DPoint min_coord, max_coord;

		// calculate the min and max values
		computeBoundingBox( *ccGraphAttributes[i], min_coord, max_coord );

		// size is the difference
		boundingBoxSize[i] = max_coord - min_coord;

		// add the margins to the size
		boundingBoxSize[i].m_x += m_leftMargin + m_rightMargin;
		boundingBoxSize[i].m_y += m_bottomMargin + m_topMargin;

		// w00t is this for ?
		//boundingBoxSize[i].m_x -= m_leftMargin;
		//boundingBoxSize[i].m_y -= m_bottomMargin;

		// offset is the lower left coord which is the min coord
		boundingBoxOffset[i] = min_coord;
	}

	// call the packer to pack the boxes given by size
	// the result is stored in boundingBoxOffsetPacker
	TileToRowsCCPacker packer;
	packer.call( boundingBoxSize, boundingBoxOffsetPacker, 1.0 );

	// now we move the nodes and update the original GraphAttributes instance
	// in one passs
	for (node v = graph.firstNode(); v; v = v->succ()) {
		// the cc index of v
		int i = ccIndex[v];
		// get the corresponding node in the ccGraph
		node cv = node2CCNode[v];
		// Move the CC to the origin by using the old cc Offset and then to new packed position and put the result
		// in the original GraphAttributes
		graphAttributes.x(v) = ccGraphAttributes[i]->x(cv) - boundingBoxOffset[i].m_x + boundingBoxOffsetPacker[i].m_x;
		graphAttributes.y(v) = ccGraphAttributes[i]->y(cv) - boundingBoxOffset[i].m_y + boundingBoxOffsetPacker[i].m_y;
	}

	// free all ccGraph related memory
	for (int i = 0; i < numCCs; i++) {
		delete ccGraph[i];
		delete ccGraphAttributes[i];
	}

	// release the arrays
	delete[] ccGraph;
	delete[] ccGraphAttributes;
}
