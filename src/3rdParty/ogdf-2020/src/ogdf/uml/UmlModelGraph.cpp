/** \file
 * \brief Implementation of the class UmlModelGraph
 *
 * \author Dino Ahr
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

#include <ogdf/uml/UmlModelGraph.h>

namespace ogdf {

UmlModelGraph::UmlModelGraph(){

	// Initialize arrays
	m_nodeLabel.init(*this);
	m_eType.init(*this,Graph::EdgeType::association);
	m_vType.init(*this,Graph::NodeType::vertex);

}

UmlModelGraph::~UmlModelGraph(){

	// ??? Destroy arrays
}

std::ostream &operator<<(std::ostream &os, const UmlModelGraph &modelGraph)
{
	// Header
	os << "\n--- UmlModelGraph ---\n" << std::endl;

	// Traverse graph

	// Nodes
	os << "Classes/Interfaces:\n" << std::endl;
	for(node v : modelGraph.nodes) {
		os << "\t" << modelGraph.getNodeLabel(v) << std::endl;
	}

	// Edges
	os << "\nRelations:\n" << std::endl;
	for(edge e : modelGraph.edges) {
		os << "\t";

		if (modelGraph.type(e) == Graph::EdgeType::association){
			os << "Association between ";
		}
		if (modelGraph.type(e) == Graph::EdgeType::generalization){
			os << "Generalization between ";
		}
		if (modelGraph.type(e) == Graph::EdgeType::dependency){
			os << "Dependency between ";
		}

		os << modelGraph.getNodeLabel(e->source()) << " and "
			<< modelGraph.getNodeLabel(e->target()) << std::endl;
	}

	return os;
}

}
