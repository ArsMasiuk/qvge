/** \file
 * \brief Base class for simultaneous drawing.
 *
 * \author Michael Schulz and Daniel Lueckerath
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

#include<ogdf/simultaneous/SimDraw.h>
#include <ogdf/fileformats/GraphIO.h>


namespace ogdf {

// default constructor
SimDraw::SimDraw()
{
	m_GA.init(m_G, GraphAttributes::edgeSubGraphs);
	m_compareBy = CompareBy::index;
	m_isDummy.init(m_G, false);
}

// calls GraphAttributes::readGML
void SimDraw::readGML(const char *fileName)
{
	GraphIO::read(m_GA, m_G, fileName, GraphIO::readGML);
}

// calls GraphAttributes::writeGML
void SimDraw::writeGML(const char *fileName) const
{
	GraphIO::write(m_GA, fileName, GraphIO::writeGML);
}


// checks whether node is a proper dummy node
// proper dummy means that node is marked as dummy and
// incident edges have at least one common input graph
bool SimDraw::isProperDummy(node v) const
{
	if(!isDummy(v))
		return false;
	int sgb = m_GA.subGraphBits(v->firstAdj()->theEdge());
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		sgb &= m_GA.subGraphBits(e);
	}

	return sgb != 0;
}

// returns number of dummies
int SimDraw::numberOfDummyNodes() const
{
	int counter = 0;
	for(node v : m_G.nodes)
		if(isDummy(v))
			counter++;
	return counter;
}

// returns number of phantom dummies
int SimDraw::numberOfPhantomDummyNodes() const
{
	int counter = 0;
	for(node v : m_G.nodes)
		if(isPhantomDummy(v))
			counter++;
	return counter;
}

// returns number of proper dummies
int SimDraw::numberOfProperDummyNodes() const
{
	int counter = 0;
	for(node v : m_G.nodes)
		if(isProperDummy(v))
			counter++;
	return counter;
}

#ifdef OGDF_DEBUG
void SimDraw::consistencyCheck() const
{
	OGDF_ASSERT(&m_G == &(m_GA.constGraph()));
	for(edge e : m_G.edges) {
		OGDF_ASSERT(m_GA.subGraphBits(e) != 0);
	}
}
#endif

// calculates maximum number of input graphs
//
int SimDraw::maxSubGraph() const
{
	int max = -1;
	for(edge e : m_G.edges)
	{
		for(int i = 31; i > max; i--)
			if(m_GA.inSubGraph(e, i))
				max = i;
	}
	return max;
}

//returns number of basic graphs
//
int SimDraw::numberOfBasicGraphs() const
{
	if(m_G.empty())
		return 0;
	return maxSubGraph()+1;
}

// returns Graph consisting of all edges and nodes from SubGraph i
//
const Graph SimDraw::getBasicGraph(int i) const
{
	//get a copy of m_G
	GraphCopy GC(m_G);

	//delete all edges that are not in SubGraph i
	List<edge> LE;
	GC.allEdges(LE);
	for(edge e : LE)
		if(!(m_GA.inSubGraph(GC.original(e),i)))
			GC.delEdge(e);

	//delete all Nodes where degree = 0
	List<node> LN;
	GC.allNodes(LN);
	for(node v : LN)
		if(v->degree() == 0)
			GC.delNode(v);

	return GC;
}

// returns GraphAttributes associated with basic graph i
//
void SimDraw::getBasicGraphAttributes(int i, GraphAttributes &GA, Graph &G)
{
	G = m_G;
	GA.init(G,m_GA.attributes());

	List<edge> LE;
	m_G.allEdges(LE);
	for(edge eLE : LE)
	{
		if(m_GA.inSubGraph(eLE,i))
		{
			for(node v : G.nodes)
			{
				if(compare(GA,v,m_GA,eLE->source()))
				{
					if(m_GA.has(GraphAttributes::nodeGraphics))
					{
						GA.x(v) = m_GA.x(eLE->source());
						GA.y(v) = m_GA.y(eLE->source());
						GA.height(v) = m_GA.height(eLE->source());
						GA.width(v) = m_GA.width(eLE->source());
					}

					if(m_GA.has(GraphAttributes::nodeId))
						GA.idNode(v) = m_GA.idNode(eLE->source());

					if(m_GA.has(GraphAttributes::nodeLabel))
						GA.label(v) = m_GA.label(eLE->source());
				}

				if(compare(GA,v,m_GA,eLE->target()))
				{
					if(m_GA.has(GraphAttributes::nodeGraphics))
					{
						GA.x(v) = m_GA.x(eLE->target());
						GA.y(v) = m_GA.y(eLE->target());
						GA.height(v) = m_GA.height(eLE->target());
						GA.width(v) = m_GA.width(eLE->target());
					}

					if(m_GA.has(GraphAttributes::nodeId))
						GA.idNode(v) = m_GA.idNode(eLE->target());

					if(m_GA.has(GraphAttributes::nodeLabel))
						GA.label(v) = m_GA.label(eLE->target());
				}
			}

			for(edge e : G.edges)
			{
				if(compare(GA,e->source(),m_GA,eLE->source())
					&& compare(GA,e->target(),m_GA,eLE->target()))
				{
					if(m_GA.has(GraphAttributes::edgeIntWeight))
						GA.intWeight(e) = m_GA.intWeight(eLE);

					if(m_GA.has(GraphAttributes::edgeLabel))
						GA.label(e) = m_GA.label(eLE);

					if(m_GA.has(GraphAttributes::edgeStyle))
						GA.strokeColor(e) = m_GA.strokeColor(eLE);

					if(m_GA.has(GraphAttributes::edgeGraphics))
						GA.bends(e) = m_GA.bends(eLE);
				}
			}
		}
		else
		{
			List<edge> LE2;
			G.allEdges(LE2);
			for(edge e2 : LE2)
			{
				if(compare(GA,e2->source(),m_GA,eLE->source())
					&& compare(GA,e2->target(),m_GA,eLE->target()))
				{
					G.delEdge(e2);
				}
			}
		}
	}

	//remove all Nodes with degree == 0
	//this can change the IDs of the nodes in G.
	List<node> LN;
	G.allNodes(LN);
	for (node v : LN)
		if (v->degree() == 0)
			G.delNode(v);
}

//adds new GraphAttributes to m_G if maxSubgraph() < 32
bool SimDraw::addGraphAttributes(const GraphAttributes & GA)
{
	if(maxSubGraph() >= 31)
		return false;

#if 0
	if(compareBy() == label)
#endif
	OGDF_ASSERT((compareBy() != CompareBy::label) || m_GA.has(GraphAttributes::edgeLabel));

	int max = numberOfBasicGraphs();
	bool foundEdge = false;
	Graph G = GA.constGraph();

	for(edge e : G.edges) {
		for(edge f : m_G.edges) {
			if (compare(m_GA, f->source(), GA, e->source())
			 && compare(m_GA, f->target(), GA, e->target())) {
				foundEdge = true;
				m_GA.addSubGraph(f,max);
			}
		}

		if (!foundEdge) {
			node s, t;
			bool srcFound = false;
			bool tgtFound = false;
			for(node v : m_G.nodes) {
				if (compare(m_GA, v, GA, e->source())) {
					s = v;
					srcFound = true;
				}
				if (compare(m_GA, v, GA, e->target())) {
					t = v;
					tgtFound = true;
				}
			}

			if (!srcFound)
				s = m_G.newNode(e->source()->index());

			if (!tgtFound)
				t = m_G.newNode(e->target()->index());

			edge d = m_G.newEdge(s, t);
			if(compareBy() == CompareBy::label)
				m_GA.label(d) = GA.label(e);

			m_GA.addSubGraph(d, max);
		}
	}
	return true;
}

//adds the new Graph G to the instance m_G if maxSubGraph < 32
//and CompareMode = index.
bool SimDraw::addGraph(const Graph & G)
{
	if(compareBy() == CompareBy::label)
		return false;
	else
	{
		GraphAttributes newGA(G);
		return addGraphAttributes(newGA);
	}
}

//compares two nodes depending on the mode in m_CompareBy
bool SimDraw::compare(const GraphAttributes & vGA, node v,
	const GraphAttributes & wGA, node w) const
{
	if(m_compareBy == CompareBy::index)
		return compareById(v,w);
	else if(m_compareBy == CompareBy::label)
		return compareByLabel(vGA, v, wGA, w);
	else
	{
		OGDF_ASSERT( false ); // m_compareBy is not set correctly
		return false;
	}
}

}
