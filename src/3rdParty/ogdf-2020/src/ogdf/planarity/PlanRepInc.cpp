/** \file
 * \brief implementation of PlanRepUML class
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

//Debug
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>

//zwei Moeglichkeiten: Elemente verstecken mit hide/activate
//oder Elemente, die nicht akiv sind, loeschen

#include <ogdf/planarity/PlanRepInc.h>
#include <ogdf/planarity/TopologyModule.h>

namespace ogdf {

PlanRepInc::PlanRepInc(const UMLGraph &UG) : PlanRepUML(UG)
{
	initMembers(UG);
}

PlanRepInc::PlanRepInc(const UMLGraph &UG, const NodeArray<bool> &fixed)
: PlanRepUML(UG)
{
	initMembers(UG);

	const Graph &G = UG.constGraph();
	//we start the node activation status with the fixed input values
	for(node v : G.nodes)
	{
		m_activeNodes[v] = fixed[v];
	}

}

void PlanRepInc::initMembers(const UMLGraph &UG)
{
	m_activeNodes.init(UG.constGraph(), true);
	//braucht man vielleicht gar nicht mehr (Kreuzungen?)
	//zumindest spaeter durch type in typefields ersetzen
	m_treeEdge.init(*this, false);
	m_treeInit = false;
}


//activate a cc with at least one node, even if all nodes are
//excluded
node PlanRepInc::initMinActiveCC(int i)
{
	node v = initActiveCCGen(i, true);
	return v;
}

void PlanRepInc::initActiveCC(int i)
{
	initActiveCCGen(i, false);
}

//activates a cc, if minnode==true, at least one node is inserted
node PlanRepInc::initActiveCCGen(int i, bool minNode)
{
	//node to be returned
	node minActive = nullptr;
	//list to be filled wih activated nodes
	List<node> activeOrigCCNodes;
	// a) delete copy / chain fields for originals of nodes in current cc,
	// since we change the CC number...
	// (since we remove all these copies in initByNodes(...)
	// b) create list of currently active original nodes

	for(int j = m_ccInfo.startNode(i); j < m_ccInfo.stopNode(i); ++j)
	{
		node vG = m_ccInfo.v(j);

		if (m_activeNodes[vG])
			activeOrigCCNodes.pushBack(vG);

		if (m_currentCC >= 0)
		{
			m_vCopy[vG] = nullptr;

			for(adjEntry adj : vG->adjEntries)
			{
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				m_eCopy[eG].clear();
			}
		}
	}
	//}

	// now we check if we have to activate a single node
	if (minNode && activeOrigCCNodes.empty()) {
		//Simple strategy: take the first node
		minActive = m_ccInfo.v(m_ccInfo.startNode(i));
		if (minActive != nullptr) {
			m_activeNodes[minActive] = true;
			activeOrigCCNodes.pushFront(minActive);
		}
	}

	m_currentCC = i;

	//double feature: liste und nodearray, besser
	GraphCopy::initByActiveNodes(activeOrigCCNodes, m_activeNodes, m_eAuxCopy);

	// set type of edges (gen. or assoc.) in the current CC
	if (m_pGraphAttributes->has(GraphAttributes::edgeType))
		for(edge e : edges)
		{
			m_eType[e] = m_pGraphAttributes->type(original(e));
			if (original(e))
			{
				switch (m_pGraphAttributes->type(original(e)))
				{
					case Graph::EdgeType::generalization: setGeneralization(e); break;
					case Graph::EdgeType::association: setAssociation(e); break;
					default: OGDF_ASSERT(false);
				}
			}
		}

	if (m_pGraphAttributes->has(GraphAttributes::nodeType))
		for(node v : nodes)
			m_vType[v] = m_pGraphAttributes->type(original(v));
	//TODO:check only in CCs or global?
	m_treeInit = false;
	return minActive;
}

//node activation automatically activates all
//adjacent edges
//CCs can be joined by this method
void PlanRepInc::activateNode(node v)
{
	if (m_activeNodes[v]) return;

	m_activeNodes[v] = true;
}

//Connect parts of partial active current CC.
//Note that this only makes sense when the CC parts are
//already correctly embedded. Crossings are already replaced
//by nodes
bool PlanRepInc::makeTreeConnected(adjEntry /* adjExternal */)
{

	//we compute node numbers for the partial CCs in order to
	//identify the treeConnect Edges that can be deleted later
	m_component.init(*this, -1);

	//if there is only one CC, we don't need to connect
	if (isConnected(*this)) return false;
	//We have to insert edges connnecting nodes lying on
	//the "external" face of the active parts

	//First, we activate the CC's parts one by one and compute
	//the 'external' face from the layout information
	//If the PlanRepInc is not embedded corresponding to
	//this layout, we may introduce edges that are non-planar
	//in the drawing, leading to problems when we compute paths
	//in the dual graph

	List<node> isolatedNodes;
	const int numPartialCC = connectedComponents(*this, m_component, &isolatedNodes);

	//CombinatorialEmbedding can cope with unconnected graphs
	//but does not provide faces for isolated nodes
	CombinatorialEmbedding E(*this);
	TopologyModule tm;
	List<adjEntry> extAdjs;
	//we run through all faces searching for all outer faces
	for(face f : E.faces)
	{
		//TODO: check if we should select special adjEntry instead of first
		if (tm.faceSum(*this, *m_pGraphAttributes, f) < 0)
			extAdjs.pushBack(f->firstAdj());

#ifdef OGDF_DEBUG
		std::cout << "FaceSum in Face " << f->index() << " Groesse " << f->size()
			<< " ist: " << tm.faceSum(*this, *m_pGraphAttributes, f) <<"\n" << std::flush;
#endif
	}

	//now we have faces for all partial CCs that are not isolated nodes

	OGDF_ASSERT(extAdjs.size() + isolatedNodes.size() == numPartialCC);
#if 0
	OGDF_ASSERT(extAdjs.size() > 1) //eigentlich: = #partial CCs
	OGDF_ASSERT(extAdjs.size() == numPartialCC);
#endif

	const int n1 = numPartialCC-1;
	m_eTreeArray.init(0, n1, 0, n1, nullptr);
	m_treeInit = true;

	//Three cases: only CCs, only isolated nodes, and both (where
	//only one CC + isolated nodes is possible

	//now we connect all partial CCs by inserting edges at the adjEntries
	//in extAdjs and adding all isolated nodes
	adjEntry lastAdj = nullptr;
	ListIterator<adjEntry> it = extAdjs.begin();
	while(it.valid())
	{
		//for the case: one external face, multiple isolated nodes
		lastAdj = (*it);
		adjEntry adj = (*it);
		ListIterator<adjEntry> it2 = it.succ();
		if (it2.valid())
		{
			adjEntry adj2 = (*it2);
			edge eTree = newEdge(adj, adj2);
			m_treeEdge[eTree] = true;
			lastAdj = eTree->adjTarget();
			//save the connection edge by CC number index
			//this is used when deleting obsolete edges later
			//after edge reinsertion
			m_eTreeArray(componentNumber(adj->theNode()), componentNumber(adj2->theNode())) =
				m_eTreeArray(m_component[adj2->theNode()], m_component[adj->theNode()])
				= eTree;
		}

		++it;
	}
	while (!isolatedNodes.empty())
	{
		node uvw = isolatedNodes.popFrontRet();
		if (lastAdj)
		{
			//same block as above
			edge eTree = newEdge(uvw, lastAdj);
			m_treeEdge[eTree] = true;
			//save the connection edge by CC number index
			//this is used when deleting obsolete edges later
			//after edge reinsertion
			m_eTreeArray(componentNumber(lastAdj->theNode()), componentNumber(uvw)) =
				m_eTreeArray(m_component[uvw], m_component[lastAdj->theNode()])
				= eTree;
			lastAdj = eTree->adjSource();
		}
		else //connect the first two isolated nodes / only iso nodes exist
		{
			//MUST BE #isonodes>1, else we returned already because CC connected
			OGDF_ASSERT(!isolatedNodes.empty());
			node secv = isolatedNodes.popFrontRet();
			//same block as above
			edge eTree = newEdge(uvw, secv);
			m_treeEdge[eTree] = true;
			//save the connection edge by CC number index
			//this is used when deleting obsolete edges later
			//after edge reinsertion
			m_eTreeArray(componentNumber(secv), componentNumber(uvw)) =
				m_eTreeArray(m_component[uvw], m_component[secv])
				= eTree;
			lastAdj = eTree->adjSource();
		}
	}

	OGDF_ASSERT(isConnected(*this));

#if 0
	List<adjEntry> extAdjs;
	getExtAdjs(extAdjs);
#endif

	return true;
}

//is only called when CC not connected => m_eTreeArray is initialized
void PlanRepInc::deleteTreeConnection(int i, int j)
{
	edge e = m_eTreeArray(i, j);
	if (e == nullptr) return;
	edge nexte = nullptr;
	OGDF_ASSERT(e);
	OGDF_ASSERT(m_treeEdge[e]);
	//we have to take care of treeConnection edges that
	//are already crossed
	while ((e->target()->degree() == 4) &&
		m_treeEdge[e->adjTarget()->cyclicSucc()->cyclicSucc()->theEdge()])
	{
		nexte = e->adjTarget()->cyclicSucc()->cyclicSucc()->theEdge();
		OGDF_ASSERT(original(nexte) == nullptr);
		delEdge(e);
		e = nexte;
	}
	delEdge(e);
	m_eTreeArray(i, j) = nullptr;
	m_eTreeArray(j, i) = nullptr;

	OGDF_ASSERT(isConnected(*this));
}

//is only called when CC not connected => m_eTreeArray is initialized
void PlanRepInc::deleteTreeConnection(int i, int j, CombinatorialEmbedding &E)
{

	edge e = m_eTreeArray(i, j);
	if (e == nullptr) return;
	edge nexte = nullptr;
	OGDF_ASSERT(e);
	OGDF_ASSERT(m_treeEdge[e]);
	//we have to take care of treeConnection edges that
	//are already crossed
	while ((e->target()->degree() == 4) &&
		m_treeEdge[e->adjTarget()->cyclicSucc()->cyclicSucc()->theEdge()])
	{
		nexte = e->adjTarget()->cyclicSucc()->cyclicSucc()->theEdge();
		OGDF_ASSERT(original(nexte) == nullptr);
		E.joinFaces(e);
		e = nexte;
	}
	E.joinFaces(e);
	m_eTreeArray(i, j) = nullptr;
	m_eTreeArray(j, i) = nullptr;

	OGDF_ASSERT(isConnected(*this));
}

//use the layout information in the umlgraph to find nodes in
//unconnected active parts of a CC that can be connected without
//crossings in the given embedding
void PlanRepInc::getExtAdjs(List<adjEntry> & /* extAdjs */)
{
	//in order not to change the current CC initialization,
	//we construct a copy of the active parts (one by one)
	//and use the layout information to compute a external
	//face for that part. An (original) adjEntry on this face
	//is then inserted into the extAdjs list.

	//derive the unconnected parts by a run through the current
	//copy
	//compute connected component of current CC

	NodeArray<int> component(*this);
	int numPartialCC = connectedComponents(*this, component);
	EdgeArray<edge> copyEdge;//copy edges in partial CC copy
	//now we compute a copy for every CC
	//initialize an array of lists of nodes contained in a CC
	Array<List<node> >  nodesInPartialCC;
	nodesInPartialCC.init(numPartialCC);

	for(node v : nodes)
		nodesInPartialCC[component[v]].pushBack(v);

	int i = 0;
	for (i = 0; i < numPartialCC; i++)
	{
		List<node> &theNodes = nodesInPartialCC[i];
		GraphCopy GC;
		GC.createEmpty(*this);
		GC.initByNodes(theNodes, copyEdge);
		//now we derive an outer face of GC by using the
		//layout information on it's original

#if 0
		//TODO: Insert the bend points into the copy

		CombinatorialEmbedding E(GC);

		//run through the faces and compute angles to
		//derive outer face
		//we dont care about the original structure of
		//the graph, i.e., if crossings are inserted aso
		//we only take the given partial CC and its layout
		adjEntry extAdj = getExtAdj(GC, E);

		for(node v : GC.nodes)
		{
		}
#endif
	}
}

//return one adjEntry on the outer face of GC where GC
//is a partial copy of this PlanRepInc
adjEntry PlanRepInc::getExtAdj(GraphCopy & /* GC */, CombinatorialEmbedding & /* E */)
{
	return adjEntry();
}

//structure updates of underlying graph
//signaled by graph structure
void PlanRepInc::nodeDeleted(node /* v */)
{
}
void PlanRepInc::nodeAdded(node /* v */)   {}
void PlanRepInc::edgeDeleted(edge /* e */) {}
void PlanRepInc::edgeAdded(edge /* e */)   {}
void PlanRepInc::reInit()            {}
void PlanRepInc::cleared()           {}


//DEBUGSTUFF
#ifdef OGDF_DEBUG
int PlanRepInc::genusLayout(Layout &drawing) const
{
	Graph testGraph;
	GraphAttributes AG(testGraph, GraphAttributes::nodeGraphics |
		GraphAttributes::edgeGraphics |
		GraphAttributes::nodeStyle |
		GraphAttributes::edgeStyle
		);
	Layout xy;
	NodeArray<node> tcopy(*this, nullptr);
	EdgeArray<bool> finished(*this, false);
	EdgeArray<edge> eOrig(testGraph, nullptr);
	Color invalid(0,0,0,0);
	EdgeArray<Color> eCol(*this, invalid);

	if (numberOfNodes() == 0) return 0;

	int nIsolated = 0;
	for(node v : nodes)
		if (v->degree() == 0) ++nIsolated;

	NodeArray<int> component(*this);
	int nCC = connectedComponents(*this,component);

	AdjEntryArray<bool> visited(*this,false);
	int nFaceCycles = 0;

	int colBase = 3;
	int colBase2 = 250;
	for(node v : nodes)
	{
		Color col =Color(colBase,colBase2,colBase);
		colBase = (colBase*3) % 233;
		colBase2 = (colBase*2) % 233;

		if (tcopy[v] == nullptr)
		{
			node u = testGraph.newNode();
			tcopy[v] = u;
			AG.x(u) = drawing.x(v);
			AG.y(u) = drawing.y(v);
			AG.fillColor(u) = col;
			AG.strokeColor(u) = Color::Name::Red;
			AG.strokeWidth(u) = 8;
		}

		for(adjEntry adj1 : v->adjEntries) {
			bool handled = visited[adj1];
			adjEntry adj = adj1;

			do {
				node z = adj->theNode();
				if (tcopy[z] == nullptr)
				{
					node u1 = testGraph.newNode();
					tcopy[z] = u1;
					AG.x(u1) = drawing.x(z);
					AG.y(u1) = drawing.y(z);
					AG.fillColor(u1) = col;
				}
				if (!finished[adj->theEdge()])
				{
					node w = adj->theEdge()->opposite(z);
					if (tcopy[w] != nullptr)
					{
						edge e;
						if (w == adj->theEdge()->source())
							e = testGraph.newEdge(tcopy[w], tcopy[z]);
						else e = testGraph.newEdge(tcopy[z], tcopy[w]);
						eOrig[e] = adj->theEdge();
						if (eCol[adj->theEdge()] == invalid)
							AG.strokeColor(e) = eCol[adj->theEdge()];
						else
						{
							eCol[adj->theEdge()] = col;
							AG.strokeColor(e) = col;
						}
						finished[adj->theEdge()] = true;
					}
#if 0
					else {
						eCol[adj->theEdge()] = col;
					}
#endif
				}
				visited[adj] = true;
				adj = adj->faceCycleSucc();
			} while (adj != adj1);

			if (handled) continue;

			++nFaceCycles;
		}
	}
#if 0
	//insert the current embedding order by setting bends
	for(node v : testGraph.nodes)
	{
		adjEntry ad1 = v->firstAdj();
	}
#endif

	int genus = (numberOfEdges() - numberOfNodes() - nIsolated - nFaceCycles + 2*nCC) / 2;
#if 0
	if (genus != 0)
#endif
	{
		GraphIO::write(AG, "GenusErrorLayout.gml", GraphIO::writeGML);
	}
	return genus;
}
//#endif

void PlanRepInc::writeGML(std::ostream &os, const GraphAttributes &AG)
{
	OGDF_ASSERT(m_pGraphAttributes == &(AG));
	const Graph &G = *this;

	NodeArray<int> id(*this);
	int nextId = 0;

	os.setf(std::ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::PlanRepInc::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	for(node v : G.nodes) {
		if (!original(v)) continue;
		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";

		os << "    graphics [\n";
		os << "      x " << AG.x(original(v)) << "\n";
		os << "      y " << AG.y(original(v)) << "\n";
		os << "      w " << 10.0 << "\n";
		os << "      h " << 10.0 << "\n";
		os << "      type \"rectangle\"\n";
		os << "      width 1.0\n";
		if (typeOf(v) == Graph::NodeType::generalizationMerger) {
			os << "      type \"oval\"\n";
			os << "      fill \"#0000A0\"\n";
		}
		else if (typeOf(v) == Graph::NodeType::generalizationExpander) {
			os << "      type \"oval\"\n";
			os << "      fill \"#00FF00\"\n";
		}
		else if (typeOf(v) == Graph::NodeType::highDegreeExpander ||
			typeOf(v) == Graph::NodeType::lowDegreeExpander)
			os << "      fill \"#FFFF00\"\n";
		else if (typeOf(v) == Graph::NodeType::dummy)
			{
				if (isCrossingType(v))
				{
					os << "      fill \"#FF0000\"\n";
				}
				else
					os << "      fill \"#FFFFFF\"\n";
				os << "      type \"oval\"\n";
			}

		else if (v->degree() > 4)
			os << "      fill \"#FFFF00\"\n";

		else
			os << "    fill \"#000000\"\n";

		os << "  ]\n"; // graphics

		os << "]\n"; // node
	}

	for(edge e : G.edges) {
		if (!(original(e->source()) && original(e->target()))) continue;
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		os << "    generalization " << typeOf(e) << "\n";

		os << "    graphics [\n";

		os << "      type \"line\"\n";

		if (typeOf(e) == Graph::EdgeType::generalization)
		{
			os << "      arrow \"last\"\n";
			if (m_alignUpward[e->adjSource()])
				os << "      fill \"#0000FF\"\n";
			else
				os << "      fill \"#FF0000\"\n";
			os << "      width 3.0\n";
		}
		else
		{
			if (typeOf(e->source()) == Graph::NodeType::generalizationExpander ||
				typeOf(e->source()) == Graph::NodeType::generalizationMerger ||
				typeOf(e->target()) == Graph::NodeType::generalizationExpander ||
				typeOf(e->target()) == Graph::NodeType::generalizationMerger)
			{
				os << "      arrow \"none\"\n";
				if (isBrother(e))
					os << "      fill \"#F0F000\"\n"; //gelb
				else if (isHalfBrother(e))
					os << "      fill \"#FF00AF\"\n";
				else
					os << "      fill \"#FF0000\"\n";
			}
			else
				os << "      arrow \"none\"\n";
			if (isBrother(e))
				os << "      fill \"#F0F000\"\n"; //gelb
			else if (isHalfBrother(e))
				os << "      fill \"#FF00AF\"\n";
			else
				os << "      fill \"#00000F\"\n";
			os << "      width 1.0\n";
		}

		if (original(e) != nullptr)
		{
			const DPolyline &dpl = AG.bends(original(e));
			if (!dpl.empty()) {
				os << "      Line [\n";
				os << "        point [ x " << AG.x(original(e->source())) << " y " <<
					AG.y(original(e->source())) << " ]\n";

				ListConstIterator<DPoint> it;
				for(it = dpl.begin(); it.valid(); ++it)
					os << "        point [ x " << (*it).m_x << " y " << (*it).m_y << " ]\n";

				os << "        point [ x " << AG.x(original(e->target())) << " y " <<
					AG.y(original(e->target())) << " ]\n";

				os << "      ]\n"; // Line
			}
		}

		os << "    ]\n"; // graphics

		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}
//#endif


void PlanRepInc::writeGML(std::ostream &os, const Layout &drawing, bool colorEmbed)
{
	const Graph &G = *this;

	NodeArray<int> id(*this);
	int nextId = 0;

	os.setf(std::ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::GraphAttributes::writeGML\"\n";

	os << "graph [\n";
	os << "  directed 1\n";

	for(node v : G.nodes) {
		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";

		os << "    graphics [\n";
		os << "      x " << drawing.x(v) << "\n";
		os << "      y " << drawing.y(v) << "\n";
		os << "      w " << 10.0 << "\n";
		os << "      h " << 10.0 << "\n";
		os << "      type \"rectangle\"\n";
		os << "      width 1.0\n";
		if (typeOf(v) == Graph::NodeType::generalizationMerger) {
			os << "      type \"oval\"\n";
			os << "      fill \"#0000A0\"\n";
		}
		else if (typeOf(v) == Graph::NodeType::generalizationExpander) {
			os << "      type \"oval\"\n";
			os << "      fill \"#00FF00\"\n";
		}
		else if (typeOf(v) == Graph::NodeType::highDegreeExpander ||
			typeOf(v) == Graph::NodeType::lowDegreeExpander)
			os << "      fill \"#FFFF00\"\n";
		else if (typeOf(v) == Graph::NodeType::dummy)
			{
				if (isCrossingType(v))
				{
					os << "      fill \"#FF0000\"\n";
				}
				else os << "      fill \"#FFFFFF\"\n";
				os << "      type \"oval\"\n";
			}

		else if (v->degree() > 4)
			os << "      fill \"#FFFF00\"\n";

		else
			os << "      fill \"#000000\"\n";


		os << "    ]\n"; // graphics

		os << "  ]\n"; // node
	}


	NodeArray<bool> proc(*this, false);
	for(edge e : G.edges)
	{
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		os << "    generalization " << typeOf(e) << "\n";
		os << "    graphics [\n";

		os << "      type \"line\"\n";

		int embedNum = 0;
		int sNum = 0;
		int tNum = 0;
		if (colorEmbed)
		{
			//Find out embedding order number
			//dirty hack, quadratic time
			//color after higher degree order
			node w;
			if (proc[e->target()] && !proc[e->source()]) w = e->target();
			else if (proc[e->source()] && !proc[e->target()]) w = e->source();
			else
				w = (e->source()->degree() > e->target()->degree() ? e->source() : e->target());

			proc[w] = true;
			adjEntry adf = w->firstAdj();
			while (adf->theEdge() != e) //ugly
			{
				embedNum++;
				adf = adf->cyclicSucc();
			}

			node uvw = e->source();
			adf = uvw->firstAdj();
			while (adf->theEdge() != e) //ugly
			{
				sNum++;
				adf = adf->cyclicSucc();
			}
			uvw = e->target();
			adf = uvw->firstAdj();
			while (adf->theEdge() != e) //ugly
			{
				tNum++;
				adf = adf->cyclicSucc();
			}
		}

			if (typeOf(e) == Graph::EdgeType::generalization)
			{
				os << "      arrow \"last\"\n";

				if (m_alignUpward[e->adjSource()])
					os << "      fill \"#0000FF\"\n";
				else
				{
					switch (embedNum)
					{
						case 1: os << "      fill \"#F00000\"\n";break;
						case 2: os << "      fill \"#D00000\"\n";break;
						case 3: os << "      fill \"#B00000\"\n";break;
						case 4: os << "      fill \"#900000\"\n";break;
						case 5: os << "fill \"#800000\"\n";break;
						case 6: os << "      fill \"#600000\"\n";break;
						case 7: os << "      fill \"#400000\"\n";break;
						default: os << "      fill \"#FF0000\"\n";
					}
				}
				os << "      width 3.0\n";
			}
			else
			{
				if (typeOf(e->source()) == Graph::NodeType::generalizationExpander ||
					typeOf(e->source()) == Graph::NodeType::generalizationMerger ||
					typeOf(e->target()) == Graph::NodeType::generalizationExpander ||
					typeOf(e->target()) == Graph::NodeType::generalizationMerger)
				{
					os << "      arrow \"none\"\n";
					if (isBrother(e))
						os << "      fill \"#F0F000\"\n"; //gelb
					else if (isHalfBrother(e))
						os << "      fill \"#FF00AF\"\n";
					else
						os << "      fill \"#FF0000\"\n";
				}
				else
					os << "      arrow \"none\"\n";
				if (isBrother(e))
					os << "      fill \"#F0F000\"\n"; //gelb
				else if (isHalfBrother(e))
					os << "      fill \"#FF00AF\"\n";
				else {
					switch (embedNum)
					{
						case 1: os << "      fill \"#000030\"\n";break;
						case 2: os << "      fill \"#000060\"\n";break;
						case 3: os << "      fill \"#200090\"\n";break;
						case 4: os << "      fill \"#3000B0\"\n";break;
						case 5: os << "      fill \"#4000E0\"\n";break;
						case 6: os << "      fill \"#5000F0\"\n";break;
						case 7: os << "      fill \"#8000FF\"\n";break;
						default: os << "      fill \"#000000\"\n";
					}
				}

				os << "      width 1.0\n";
			}

			//insert a bend at each end corresponding to the
			//adjacency order
			if (colorEmbed)
			{
				double rad = 20.0;
				node vs = e->source();
				node vt = e->target();
				double sx, sy, tx, ty;

				const double xs = drawing.x(vs),
					ys = drawing.y(vs); //aufpunkt
				const double xt = drawing.x(vt),
					yt = drawing.y(vt); //aufpunkt

				//double dx = drawing.x(vt) - drawing.x(vs);
				//double dy = drawing.y(vt) - drawing.y(vs);
				//double length = sqrt(dx*dx + dy*dy);
				//reference edge
				//Aufpunkte
				node rws = vs->firstAdj()->twinNode();
				node rwt = vt->firstAdj()->twinNode();
				//Richtungsvektoren
				double rdxs = drawing.x(rws) - xs;
				double rdys = drawing.y(rws) - ys;
				double rdxt = drawing.x(rwt) - xt;
				double rdyt = drawing.y(rwt) - yt;

				double rslength = sqrt(rdxs*rdxs + rdys*rdys);
				double rtlength = sqrt(rdxt*rdxt + rdyt*rdyt);

				double refSx = drawing.x(vs) + (rad/rslength)*rdxs;
				double refSy = drawing.y(vs) + (rad/rslength)*rdys;
				double refTx = drawing.x(vt) + (rad/rtlength)*rdxt;
				double refTy = drawing.y(vt) + (rad/rtlength)*rdyt;

				double refSx2 = drawing.x(vs) + rdxs/rslength;
				double refSy2 = drawing.y(vs) + rdys/rslength;
				double refTx2 = drawing.x(vt) + rdxt/rslength;
				double refTy2 = drawing.y(vt) + rdyt/rslength;

				//do not change reference edge
				if (sNum <0)//== 0)
				{
					sx = refSx;
					sy = refSy;
				}
				else
				{
					OGDF_ASSERT(sNum < vs->degree());
					double angleS = sNum*360.0/vs->degree();

					double refAngleS = DPoint(xs, ys).angleDegrees(
						DPoint(refSx2, refSy2),
						DPoint(xs+1.0, ys)
						);
					double testAngleS = refAngleS-angleS;

					//---
					double dTS = Math::degreesToRadians(testAngleS);
					//--
					sx = xs + rad*cos(dTS);//testAngleS);
					sy = ys + rad*sin(dTS);//testAngleS);
				}
				if (tNum <0)//== 0)
				{
					tx = refTx;
					ty = refTy;
				}
				else
				{
					OGDF_ASSERT(tNum < vt->degree());
					double angleT = tNum*360/vt->degree();

					double refAngleT = DPoint(xt, yt).angleDegrees(
						DPoint(refTx2, refTy2),
						DPoint(xt+1.0, yt)
						);
					double testAngleT = refAngleT-angleT;

					//--
					double dTT = Math::degreesToRadians(testAngleT);
					//--

					tx = xt + rad*cos(dTT);//testAngleT);
					ty = yt + rad*sin(dTT);//testAngleT);

				}

				os << "      Line [\n";
				os << "        point [ x " << xs << " y " << ys << " ]\n";
				os << "        point [ x " << sx << " y " << sy << " ]\n";
				os << "        point [ x " << tx << " y " << ty << " ]\n";
				os << "        point [ x " << xt << " y " << yt << " ]\n";

				os << "      ]\n"; // Line
			}
			os << "    ]\n"; // graphics

		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}
#endif

}
