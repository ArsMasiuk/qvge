/** \file
 * \brief Abstract base class for structures on graphs, that need
 *        to be informed about graph changes (e.g. cluster graphs).
 *
 * Follows the observer pattern: graphs are observable
 * objects that can inform observers on changes made to their
 * structure.
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

#include <ogdf/basic/List.h>
#include <ogdf/basic/Graph.h>

namespace ogdf {

//
// in embedded graphs, adjacency lists are given in clockwise order.
//


//! Abstract Base class for graph observers.
/**
 * @ingroup graphs
 *
 * If a class needs to keep track of changes in a graph like addition or deletion
 * of nodes or edges, you can derive it from GraphObserver and override the
 * notification methods nodeDeleted, nodeAdded, edgeDeleted, edgeAdded.
 */
class OGDF_EXPORT GraphObserver {
	friend class Graph;

public:
	//! Constructs instance of GraphObserver class
	GraphObserver() : m_pGraph(nullptr) { }

	/**
	 *\brief Constructs instance of GraphObserver class
	 * \param G is the graph to be watched
	 */
	explicit GraphObserver(const Graph* G) : m_pGraph(G)
	{
		m_itGList = G->registerStructure(this);
	}

	//! Destroys the instance, unregisters it from watched graph
	virtual ~GraphObserver()
	{
		if (m_pGraph) m_pGraph->unregisterStructure(m_itGList);
	}

	//! Associates observer instance with graph \p G
	void reregister(const Graph *pG) {
		//small speedup: check if == m_pGraph
		if (m_pGraph) m_pGraph->unregisterStructure(m_itGList);
		if ((m_pGraph = pG) != nullptr) m_itGList = pG->registerStructure(this);
	}

	//! Called by watched graph when a node is deleted
	//! Has to be implemented by derived classes
	virtual void nodeDeleted(node v) = 0;

	//! Called by watched graph when a node is added
	//! Has to be implemented by derived classes
	virtual void nodeAdded(node v)   = 0;

	//! Called by watched graph when an edge is deleted
	//! Has to be implemented by derived classes
	virtual void edgeDeleted(edge e) = 0;

	//! Called by watched graph when an edge is added
	//! Has to be implemented by derived classes
	virtual void edgeAdded(edge e)   = 0;

	//! Called by watched graph when it is reinitialized
	//! Has to be implemented by derived classes
	virtual void reInit()            = 0;

	//! Called by watched graph when its clear function is called
	//! Has to be implemented by derived classes
	virtual void cleared()           = 0;

	const Graph*  getGraph() const { return m_pGraph; }

protected:
	const Graph* m_pGraph; //! watched graph
	ListIterator<GraphObserver*> m_itGList; //! List entry in graphs list of all registered graphobservers
};

}
