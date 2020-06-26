/** \file
 * \brief Implementation of UMLGraph class
 *
 * \author Carsten Gutwenger, Sebastian Leipert
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


#include <ogdf/uml/UMLGraph.h>
#include <ogdf/misclayout/CircularLayout.h>

namespace ogdf {


UMLGraph::UMLGraph(
	Graph &G,
	long initAttributes) :
	GraphAttributes(G, initAttributes | edgeType | nodeType | nodeGraphics | edgeGraphics), m_pG(&G), m_cliqueCenterSize(10)
{
	m_hiddenEdges = new Graph::HiddenEdgeSet(G);
	m_upwardEdge.init(G, false);
	m_hierarchyParent.init(G,nullptr);
	m_assClass.init(G, nullptr);
	m_associationClassModel.init(G, nullptr);
}


UMLGraph::~UMLGraph()
{
	delete m_hiddenEdges;

	SListIterator<AssociationClass*> it = m_assClassList.begin();
	while (it.valid())
	{
		delete *it;
		++it;
	}
}

void UMLGraph::insertGenMergers()
{
	if (m_pG->empty()) return;

	node v = m_pG->firstNode(), vLast = m_pG->lastNode(); //new nodes are pushed behind
	//otherwise, mergers would be considered

	for( ; ; )
	{
		SList<edge> inGens;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if (e->target() != v || type(e) != Graph::EdgeType::generalization)
				continue;

			inGens.pushBack(e);
		}

		doInsertMergers(v, inGens);

		if (v == vLast) break;
		v = v->succ();
	}

	adjustHierarchyParents();
}

void UMLGraph::adjustHierarchyParents()
{
	for(node v : m_pG->nodes)
	{
		if (!m_hierarchyParent[v]) continue;
		for(adjEntry ae : v->adjEntries)
		{
			if (ae->theNode() != v)
				continue;
			if (m_hierarchyParent[v] == m_hierarchyParent[ae->twinNode()]) //(half)brothers
				m_upwardEdge[ae] = true; //the same should be for twin

		}
	}
}

// inserts a merger node for generalizations hanging at v
node UMLGraph::doInsertMergers(node v, SList<edge> &inGens)
{
	node u = nullptr;
	if (m_pG->empty()) return u;
	if(inGens.size() >= 2)
	{

		// create a new node representing the merge point for the generalizations
		u = m_pG->newNode();
		type(u) = Graph::NodeType::generalizationMerger;

		// add the edge from v to the merge point
		// this edge is a generalization, but has no original edge
		edge eMerge = m_pG->newEdge(u,v);
		type(eMerge) = Graph::EdgeType::generalization;
		m_mergeEdges.pushBack(eMerge);

		// We move the target node of each ingoing generalization of v to u.
		// Note that, for each such edge e, the target node of the original
		// edge is then different from the original of the target node of e
		// (the latter is 0 because u is a new (dummy) node)
		SListConstIterator<edge> it;
		for(it = inGens.begin(); it.valid(); ++it)
		{
			// all edges in the list inGens must be ingoing generalizations of v
			OGDF_ASSERT((*it)->target() == v);
			OGDF_ASSERT(type(*it) == Graph::EdgeType::generalization);

			m_pG->moveTarget(*it,u);
			m_hierarchyParent[(*it)->source()] = u; //set to merger
			m_hierarchyParent[u] = v;
			m_upwardEdge[(*it)->adjSource()] = true;//set status at source node
		}
	} else {
		if (inGens.size() == 1)
		{   //they are not needed yet
#if 0
			m_hierarchyParent[inGens.front()->source()] = v;
			m_upwardEdge[inGens.front()->adjSource()] = true;//set status at source node
#endif
		}
	}
	return u;
}

void UMLGraph::undoGenMergers()
{
	SListConstIterator<edge> it;
	for(it = m_mergeEdges.begin(); it.valid(); ++it)
	{
		edge eMerge = *it;
		node u = eMerge->source();
		const DPolyline &common = bends(eMerge);

		adjEntry adj, adjSucc;
		for(adj = u->firstAdj(); adj != nullptr; adj = adjSucc) {
			adjSucc = adj->succ();

			edge e = adj->theEdge();
			if(e->target() != u) continue;

			DPolyline &dpl = bends(e);
			dpl.pushBack(DPoint(x(u),y(u)));

			ListConstIterator<DPoint> itDp;
			for(itDp = common.begin(); itDp.valid(); ++itDp)
				dpl.pushBack(*itDp);

			m_pG->moveTarget(e,eMerge->target());
		}

		m_pG->delNode(u);
	}

	m_mergeEdges.clear();
}

// not used?
#if 0
// sorts the edges around all nodes of AG corresponding to the
// layout given in AG
// there is no check of the embedding afterwards because this
// method could be used as a first step of a planarization
void UMLGraph::sortEdgesFromLayout()
{
	//we order the edges around each node corresponding to
	//the input embedding in the GraphAttributes layout
	NodeArray<SListPure<adjEntry> > adjList(*m_pG);

	EdgeComparer* ec = new EdgeComparer(*this);

	for(node v : m_pG->nodes)
	{
		for(adjEntry ae : v->adjEntries)
		{
			adjList[v].pushBack(ae);
		}

		//sort the entries
		adjList[v].quicksort(*ec);
		m_pG->sort(v, adjList[v]);
	}

	delete ec;
}
#endif

// replace each node set in cliques by a star connecting
// a new center node with all nodes in set, deletes all
// edges between nodes in set, lists need to be disjoint
// TODO: think about directly using the cliquenum array
// output of findcliques here
void UMLGraph::replaceByStar(List< List<node> > &cliques)
{
	m_cliqueCircleSize.init(*m_pG);
	m_cliqueCirclePos.init(*m_pG);
#if 0
	m_cliqueEdges.init(*m_pG);
#endif
	m_replacementEdge.init(*m_pG, false);

	if (cliques.empty()) return;
	//we save membership of nodes in each list
	NodeArray<int> cliqueNum(*m_pG, -1);
	ListIterator< List<node> > it = cliques.begin();

	int num = 0;
	while (it.valid())
	{
		ListIterator<node> itNode = (*it).begin();
		while (itNode.valid())
		{
			cliqueNum[*itNode] = num;
			++itNode;
		}

		num++;
		++it;
	}

	//now replace each list
	it = cliques.begin();
	while (it.valid())
	{
		node newCenter = replaceByStar((*it), cliqueNum);
		OGDF_ASSERT(newCenter);
		m_centerNodes.pushBack(newCenter);
		//now we compute a circular drawing of the replacement
		//and save its size and the node positions
		m_cliqueCircleSize[newCenter] = circularBound(newCenter);
		++it;
	}
}

//compute a drawing of the clique around node center and save its size
//the call to circular will later be replaced by an dedicated computation
DRect UMLGraph::circularBound(node center)
{

	//TODO: hier computecliqueposition(0,...) benutzen, rest weglassen
	DRect bb;
	CircularLayout cl;
	Graph G;
	GraphAttributes AG(G);
	NodeArray<node> umlOriginal(G);

	//TODO: we need to assure that the circular drawing
	//parameters fit the drawing parameters of the whole graph
	//umlgraph clique parameter members?

	OGDF_ASSERT(center->degree() > 0);
	node lastNode = nullptr;
	node firstNode = nullptr;

	adjEntry ae = center->firstAdj();
	do {
		node w = ae->twinNode();
		node v = G.newNode();
		umlOriginal[v] = w;

		if (!firstNode) firstNode = v;
		AG.width(v) = width(w);
		AG.height(v) = height(w);
		ae = ae->cyclicSucc();
		if (lastNode != nullptr) G.newEdge(lastNode, v);
		lastNode = v;
	} while (ae != center->firstAdj());
	G.newEdge(lastNode, firstNode);

	cl.call(AG);

	for(node v : G.nodes)
	{
		m_cliqueCirclePos[umlOriginal[v]] = AG.point(v);
	}
	bb = AG.boundingBox();

	return bb;
}

#if 0
//computes relative positions of all nodes around center on a circle
//keeping the topological ordering of the nodes, letting the opposite
//node (to center) of the edge at firstAdj having the position at
//three o'clock (TODO: unter umstaenden hier aus gegebenen coords in
//this den Punkt auswaehlen. Aber: Was, wenn keine coords=>optional?
void UMLGraph::computeCliquePosition(node center, double rectMin)
{
	List<node> adjNodes;
	adjEntry ae = center->firstAdj();
	do
	{
		adjNodes.pushBack(ae->twinNode());
		ae = ae->cyclicPred();
	} while (ae != center->firstAdj());
	computeCliquePosition(adjNodes, center, rectMin);
}
#endif

//computes relative positions of all nodes in List cList on a minimum size
//circle (needed to compute positions with different ordering than given in *this).
//Precondition: nodes in adjNodes are adjacent to center
//first node in adjNodes is positioned to the right
void UMLGraph::computeCliquePosition(List<node> &adjNodes, node center, double rectMin)//, const adjEntry &startAdj)
{
	DRect boundingBox;
	OGDF_ASSERT(center->degree() > 0);
	OGDF_ASSERT(center->degree() == adjNodes.size());

	node v;
	double radius = 0.0;
	//TODO: member, parameter
	//const
		double minDist = 1.0;
	//TODO: necessary?
	double minCCDist = 20.0;

	ListIterator<node> itNode = adjNodes.begin();

	//for the temporary solution (scale clique to fixed rect if possible instead
	//of guaranteeing the rect size in compaction) we check in advance if the sum
	//of diameters plus dists fits into the given rect by heuristic estimate (biggest
	//node size + radius)
	if (rectMin > 0.0)
	{
		double rectDist = m_cliqueCenterSize; //dist to rect border todo: parameter
		double rectBound = rectMin - 2.0*rectDist;
		double maxSize = 0.0;
		double pureSumDiameters = 0.0;
		while (itNode.valid())
		{
			node q  =(*itNode);
			double d = sqrt(
				width(q)*width(q) + height(q)*height(q));
			pureSumDiameters += d;

			if (d > maxSize) maxSize = d;

			++itNode;
		}
		double totalSum = pureSumDiameters+(center->degree()-1)*minDist;
		//TODO: scling, not just counting
		while (totalSum/Math::pi < rectBound*0.75)
		{
			minDist = minDist + 1.0;
			totalSum += (center->degree()-1.0);
		}
		if (minDist > 1.1) minDist -= 1.0;
		//do not use larger value than cliquecentersize (used with separation)
		//if (minDist > m_cliqueCenterSize) minDist = m_cliqueCenterSize;
		itNode = adjNodes.begin();
	}
	//temporary part ends

	//first, we compute the radius of the circle

	const int n = center->degree();
	//sum of all diameters around the nodes and the max diameter radius
	double sumDiameters = 0.0, maxR = 0;
	//list of angles for all nodes
	List<double> angles; //node at startAdj gets 0.0
	double lastDiameter = 0.0; //temporary storage of space needed for previous node
	bool first = true;

	while (itNode.valid())
	{
		v  =(*itNode);
		double d = sqrt(
			width(v)*width(v) + height(v)*height(v));

		sumDiameters += d;

		if (d/2.0 > maxR) maxR = d/2.0;

		//save current position relative to startadj
		//later on, compute angle out of these values
		if (first)
		{
			angles.pushBack(0.0);
			first = false;
		}
		else
		{
			angles.pushBack(lastDiameter+d/2.0+minDist+angles.back());
		}
		lastDiameter = d/2.0; //its only half diameter...

		++itNode;
	}

	OGDF_ASSERT(adjNodes.size() == angles.size());

	if(n == 1) {
		radius      = 0;
	} else if (n == 2) {
		radius      = 0.5*minDist + sumDiameters / 4;
	} else {
		double perimeter = (n*minDist + sumDiameters);
		radius      = perimeter / (2*Math::pi);

		ListIterator<double> it = angles.begin();
		itNode = adjNodes.begin();
		while (it.valid())
		{
			(*it) = (*it)*360.0/perimeter;
			node w = *itNode;
			double angle = Math::degreesToRadians(*it);
			m_cliqueCirclePos[w].m_x = radius*cos(angle);
			m_cliqueCirclePos[w].m_y = radius*sin(angle);
			++itNode;
			++it;
		}
	}

	//now we normalize the values (start with 0.0) and
	//derive the bounding box
	v = adjNodes.front();
	double minX = m_cliqueCirclePos[v].m_x,
		maxX = m_cliqueCirclePos[v].m_x;
	double minY = m_cliqueCirclePos[v].m_y,
		maxY = m_cliqueCirclePos[v].m_y;
	itNode = adjNodes.begin();
	while (itNode.valid())
	{
		node w = *itNode;
		double wx = m_cliqueCirclePos[w].m_x;
		double wy = m_cliqueCirclePos[w].m_y;
		if(wx-width (w)/2.0 < minX) minX = wx-width(w)/2.0;
		if(wx+width (w)/2.0 > maxX) maxX = wx+width(w)/2.0;
		if(wy-height(w)/2.0 < minY) minY = wy-height(w)/2.0;
		if(wy+height(w)/2.0 > maxY) maxY = wy+height(w)/2.0;
		++itNode;
	}
	//allow distance
	minX -= minCCDist;
	minY -= minCCDist;
	//normalize
#if 0
	std::cout << "\n";
#endif

	itNode = adjNodes.begin();
	while (itNode.valid())
	{
		node w = *itNode;
#if 0
		std::cout << "x1:" << m_cliqueCirclePos[w].m_x << ":y:" << m_cliqueCirclePos[w].m_y << "\n";
#endif
		m_cliqueCirclePos[w].m_x -= minX;
		m_cliqueCirclePos[w].m_y -= minY;
#if 0
		std::cout << "x:" << m_cliqueCirclePos[w].m_x << ":y:" << m_cliqueCirclePos[w].m_y << "\n";
#endif
		++itNode;
	}

	//reassign the size, this time it is the final value
	m_cliqueCircleSize[center] = DRect(0.0, 0.0, maxX-minX, maxY-minY);
}

node UMLGraph::replaceByStar(List<node> &clique, NodeArray<int> &cliqueNum)
{
	if (clique.empty()) return nullptr;
	//insert an additional center node

	node center = m_pG->newNode();
	width(center) = m_cliqueCenterSize;
	height(center) = m_cliqueCenterSize;
#ifdef OGDF_DEBUG
	//should ask for attributes
	if(has(nodeStyle))
		fillColor(center) = Color(0x55,0x55,0x55);
#endif
	//we delete all edges inzident to two clique nodes
	//store all of them first in delEdges
	List<edge> delEdges;
	//TODO: Store edge type for all deleted edges
	ListIterator<node> it = clique.begin();
	while (it.valid())
	{
		node v = (*it);

		int numIt = cliqueNum[v];

		for(adjEntry ad : v->adjEntries)
		{
			if (cliqueNum[ad->twinNode()] == numIt)
			{
				if (ad->theEdge()->source() == v)
				{
#if 0
					m_cliqueEdges[v].pushBack(new CliqueInfo(ad->theEdge()->target(), ad->theEdge()->index()));
#endif
					delEdges.pushBack(ad->theEdge());
				}
			}
		}

		//connect center node to clique node
		edge inserted = m_pG->newEdge(center, v);
		this->type(inserted) = Graph::EdgeType::association;
		m_replacementEdge[inserted] = true;

		++it;
	}

	//now delete all edges
	ListIterator<edge>	itEdge = delEdges.begin();
	while (itEdge.valid())
	{
#if 0
		m_pG->delEdge((*itEdge));
#endif
		m_hiddenEdges->hide(*itEdge);
		++itEdge;
	}

	return center;
}

void UMLGraph::undoStars()
{
	SListIterator<node> it = m_centerNodes.begin();
	while (it.valid())
	{
		undoStar(*it, false);
		++it;
	}

	m_hiddenEdges->restore();
	m_centerNodes.clear();
	m_replacementEdge.init();
}

//remove the center node and reinsert the deleted edges
void UMLGraph::undoStar(node center, bool restoreAllEdges)
{
	OGDF_ASSERT(center);

	if (restoreAllEdges) {
		m_hiddenEdges->restore();
	}

	//remove center node
	m_pG->delNode(center);
}

// Same as in GraphAttributes. Except: Writes red color to generalizations

void UMLGraph::writeGML(const char *fileName)
{
	std::ofstream os(fileName);
	writeGML(os);
}


void UMLGraph::writeGML(std::ostream &os)
{
	const Graph &G = constGraph();

	NodeArray<int> id(G);
	int nextId = 0;

	os.setf(std::ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::GraphAttributes::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	for(node v : G.nodes) {
		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";

		if (has(nodeLabel))
			os << "    label \"" << label(v) << "\"\n";

#if 0
		if (has(nodeGraphics)
#endif
		{
			os << "    graphics [\n";
			os << "      x " << x(v) << "\n";
			os << "      y " << y(v) << "\n";
			os << "      w " << width(v) << "\n";
			os << "      h " << height(v) << "\n";
			os << "      type \"rectangle\"\n";
			os << "      width 1.0\n";

			if (type(v) == Graph::NodeType::generalizationMerger)
				os << "      fill \"#0000A0\"\n";
			else if (type(v) == Graph::NodeType::generalizationExpander)
				os << "      fill \"#00FF00\"\n";
			else {
				if (has(nodeStyle))
				{
					os << "      fill \"" << fillColor(v) << "\"\n";
					os << "      line \"" << strokeColor(v) << "\"\n";
				} else {
					if (v->degree() > 4) {
						os << "      fill \"#FFFF00\"\n";
					}
				}
			}
			os << "    ]\n"; // graphics
		}

		os << "  ]\n"; // node
	}

	for(edge e : G.edges) {
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		if (has(edgeType))
			os << "    generalization " << type(e) << "\n";

		if (has(edgeGraphics)) {
			os << "    graphics [\n";
			os << "      type \"line\"\n";
			if (has(GraphAttributes::edgeType)) {
				if (type(e) == Graph::EdgeType::generalization)
				{
					os << "      arrow \"last\"\n";
					if (m_upwardEdge[e->adjSource()])
						os << "      fill \"#FF00FF\"\n";
					else os << "      fill \"#FF0000\"\n";
					os << "      width 2.0\n";
				}
				else
				{
					if (has(edgeStyle))
					{
						// color edges, if specific color in attribut is set
						os << "      fill \"" << m_edgeStroke[e].m_color << "\"\n";
					}
					else
						if (m_upwardEdge[e->adjSource()])
							os << "      fill \"#2Fff2F\"\n";
					os << "      arrow \"none\"\n";
					os << "      width 1.0\n";
				}
#if 0
				os << "      generalization " << type(e) << "\n";
#endif

			} else {
				os << "      arrow \"last\"\n";
			}

			const DPolyline &dpl = bends(e);
			if (!dpl.empty()) {
				os << "      Line [\n";
				os << "        point [ x " << x(e->source()) << " y " <<
					y(e->source()) << " ]\n";

				ListConstIterator<DPoint> it;
				for(it = dpl.begin(); it.valid(); ++it)
					os << "        point [ x " << (*it).m_x << " y " << (*it).m_y << " ]\n";

				os << "        point [ x " << x(e->target()) << " y " << y(e->target()) << " ]\n";

				os << "      ]\n"; // Line
			}

			os << "    ]\n"; // graphics
		}

		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}

}
