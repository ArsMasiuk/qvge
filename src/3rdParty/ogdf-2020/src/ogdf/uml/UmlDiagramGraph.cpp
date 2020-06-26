/** \file
 * \brief Implementation of the class UmlDiagramGraph
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

#include <ogdf/uml/UmlDiagramGraph.h>

namespace ogdf {

UmlDiagramGraph::UmlDiagramGraph(const UmlModelGraph &umlModelGraph,
                                 UmlDiagramType diagramType,
                                 const string &diagramName):
	m_modelGraph(umlModelGraph),
	m_diagramName(diagramName),
	m_diagramType(diagramType)
{
}

UmlDiagramGraph::~UmlDiagramGraph()
{
	// Remove elements from lists
	m_containedNodes.clear();
	m_containedEdges.clear();
	m_x.clear();
	m_y.clear();
	m_w.clear();
	m_h.clear();
}

void UmlDiagramGraph::addNodeWithGeometry(
	node umlNode,
	double x, double y,
	double w, double h)
{
	// Append node to the end of the list
	m_containedNodes.pushBack(umlNode);

	// Dito with coordinates
	m_x.pushBack(x);
	m_y.pushBack(y);
	m_w.pushBack(w);
	m_h.pushBack(h);

}

void UmlDiagramGraph::addEdge(edge umlEdge)
{
	// Append edge to the end of the list
	m_containedEdges.pushBack(umlEdge);
}

const char *UmlDiagramGraph::getDiagramTypeString() const
{
	switch(m_diagramType){

	case (UmlDiagramType::classDiagram):
		return "Class diagram";
		break;
	case (UmlDiagramType::moduleDiagram):
		return "Module diagram";
		break;
	case (UmlDiagramType::sequenceDiagram):
		return "Sequence diagram";
		break;
	case (UmlDiagramType::collaborationDiagram):
		return "Collaboration diagram";
		break;
	case (UmlDiagramType::componentDiagram):
		return "Component diagram";
		break;
	case (UmlDiagramType::unknownDiagram):
		return "Unknown type diagram";
		break;
	default:
		return "";
	}
}

std::ostream &operator<<(std::ostream &os, const UmlDiagramGraph &diagramGraph)
{
	// Header with diagram name and type
	os << "\n--- " << diagramGraph.getDiagramTypeString()
		<< " \"" << diagramGraph.m_diagramName << "\" ---\n" << std::endl;

	// Nodes

	// Initialize iterators
	SListConstIterator<double> xIt = diagramGraph.m_x.begin();
	SListConstIterator<double> yIt = diagramGraph.m_y.begin();
	SListConstIterator<double> wIt = diagramGraph.m_w.begin();
	SListConstIterator<double> hIt = diagramGraph.m_h.begin();

	// Traverse lists
	for(node v : diagramGraph.m_containedNodes)
	{
		os << "Node " << diagramGraph.m_modelGraph.getNodeLabel(v)
			<< " with geometry ("
			<< *xIt << ", "
			<< *yIt << ", "
			<< *wIt << ", "
			<< *hIt << ")." << std::endl;

		++xIt;
		++yIt;
		++wIt;
		++hIt;
	}

	// Edges

	// Traverse lists
	for(edge e : diagramGraph.m_containedEdges)
	{
		os << "Edge between "
			<< diagramGraph.m_modelGraph.getNodeLabel(e->source())
			<< " and "
			<< diagramGraph.m_modelGraph.getNodeLabel(e->target())
			<< std::endl;
	}

	return os;
}

}
