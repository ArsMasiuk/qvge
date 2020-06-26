/** \file
 * \brief Implementation of TopologyModule (sets embedding
 * from layout)
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

//debug
#include <ogdf/planarity/PlanRepInc.h>

#include <ogdf/planarity/TopologyModule.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf {

namespace topology_module {

//! Sorts EdgeLegs according to their xp distance to a reference point.
class PointComparer : public GenericComparer<ListIterator<EdgeLeg*>, double> {
public:
	explicit PointComparer(DPoint refPoint) : GenericComparer([&](const ListIterator<EdgeLeg*>& it) {
		const DPoint& p = (*it)->m_xp;

		double xDiff = p.m_x - refPoint.m_x;
		double yDiff = p.m_y - refPoint.m_y;

		return xDiff*xDiff + yDiff*yDiff;
	}) {}
};

}

using namespace topology_module;

//sets the embedding in PG corresponding to the layout in GA
//returns false if planarization fails (there may be diagram-
//inherent reasons for that)
bool TopologyModule::setEmbeddingFromGraph(
	PlanRep &PG,
	GraphAttributes &GA,
	adjEntry &adjExternal,
	bool setExternal,
	bool reuseAGEmbedding)
{
	const Graph & GG = GA.constGraph();
	m_eLegs.init(GG);
	//TODO: we have to check consistency:
	//input: (same graph in GA and PG)
	//after planarize: is planar


	//intialize the crossing positions nodearray
	m_crossPosition.init(PG);

	//do we have to compute the new embedding or is it given?
	if (!reuseAGEmbedding)
	{
		//we order the edges around each node corresponding to
		//the input embedding in the GraphAttributes layout.
		//we could use the sorting in UMLGraph/GraphAttributes
		//here, but GA can't access its graph non-const.
		NodeArray<SListPure<adjEntry> > adjList(PG);

		adjExternal = nullptr;

		//sorts edges by layout information
		EdgeComparer* ec = new EdgeComparer(GA, PG);

		//we only allow PlanReps that have no bend nodes for the bends
		for(node v : PG.nodes)
		{
			for(adjEntry ae : v->adjEntries)
			{
				adjList[v].pushBack(ae);
			}
			//sort the entries
			adjList[v].quicksort(*ec);
			PG.sort(v, adjList[v]);
		}

		delete ec;
	}

	//now insert necessary crossings
	//efficient check, if any crossings are necessary
	//before computing them (#e^2)

	//TODO: hier mal checken, ob PG zusammenhaengend ist,
	//und schauen ob es sein muss.
	//Resultat: Fuer representsCombEmbedding (Genus) und
	//den planaritytest muss es nicht sein.

	if(!PG.representsCombEmbedding())
	{
#ifdef OGDF_DEBUG
		std::ofstream ofs_vor("Vorplanaris.gml", std::ofstream::out);
		((PlanRepInc&)PG).writeGML(ofs_vor, GA);
		ofs_vor.close();
#endif
		planarizeFromLayout(PG, GA);
		if (!PG.representsCombEmbedding())
			handleImprecision(PG);
#ifdef OGDF_DEBUG
		std::ofstream ofs_nach("Nachplanaris.gml", std::ofstream::out);
		((PlanRepInc&)PG).writeGML(ofs_nach, GA);
		ofs_nach.close();
#endif
	}

	//TODO: there may be non-planarizable drawings,
	//so we should not assert, but planarise without
	//respect to the given drawing
	//Result: Done, but this must be consistent with the
	//surrounding algorithm (should be aware of the fact
	//that the embedding does not represent the layout
	//information
	if (!isPlanar(PG))
		return false;

	if(!PG.representsCombEmbedding())
	{
		//we don't have a correct embedding, but there
		//may be several reasons, some of them are repairable
		//if we still have no correct embedding, it is typically
		//because the positioning input didn't make any sense in
		//an interpretation, e.g. all positions zero
		planarEmbed(PG);
	}

	PG.removePseudoCrossings();
	postProcess(PG);
	if (!isPlanar(PG))
		return false;

	//if it is still not correct, we have to reembed without
	//respecting the user input
	if(!PG.representsCombEmbedding())
	{
#ifdef OGDF_DEBUG
		GraphIO::write(PG, "outputEmbed.gml", GraphIO::writeGML);
#endif
		planarEmbed(PG);
		PG.removePseudoCrossings();
	}

	//now compute the external face
	if(setExternal &&(PG.numberOfEdges() > 0))
	{
		face f = getExternalFace(PG, GA);
		adjExternal = f->firstAdj();
	}

//face fExternal = E.maximalFace();
//face fExternal = findBestExternalFace(PG,E);
//adjExternal = fExternal->firstAdj();
//debug
#ifdef OGDF_DEBUG
	std::ofstream f("AdjSortCopy.txt");
	for(node v : PG.nodes)
	{
		if (PG.original(v))
		{
			f << "\nNode: " << PG.original(v)->index() <<"\n";
			for(adjEntry ae : v->adjEntries)
			{
				node w = PG.original(ae->twinNode());
				if (w)
					f << "  Eintrag: " << w->index() << "\n";
				else f <<"No original\n";

			}
		}
	}
#endif
	return true;
}

//we handle and correct cases were imprecision occurs, e.g.
//double computations for mergers and crossings
void TopologyModule::handleImprecision(PlanRep &PG)
{
	List<node> problems;
	//we don't need to check processing from both sides as long
	//as we only process crossings at original nodes
#if 0
	EdgeArray<bool> processed(PG, false);
#endif
	for(node v : PG.nodes)
	{
		if (PG.isCrossingType(v))
		{
			//if cases can occur where the assertion fails,
			//the following loop condition may fail, too
			OGDF_ASSERT(v->degree() == 4);
			adjEntry adFirst = v->firstAdj();
			adjEntry adRun = adFirst;
			do {
				adjEntry adNext = adRun->cyclicSucc();
				node w = adRun->theEdge()->opposite(v);
				if (w == adNext->theEdge()->opposite(v)
				   // We only take the cases into account
				   // were the crossing is near an input node
				   // thereby respecting the input embedding
				   // vs. crossing imprecision dilemma
				 && PG.original(w)
				 && adNext->twin() == adRun->twin()->cyclicSucc()) {
					// wrong order at w
					PG.swapAdjEdges(adNext->twin(), adRun->twin()->cyclicSucc());
					problems.pushBack(v);
				}
				adRun = adNext;
			} while (adRun != adFirst);
		}
	}
	ListIterator<node> it = problems.begin();
	while (it.valid())
	{
		//hier entweder switchen oder Kreuzung entfernen
		//PG.removeCrossing((*it));
		++it;
	}

	//TODO: take care not to handle crossing cascades
	//leading to multiple
}

void TopologyModule::postProcess(PlanRep &PG)
{
	//remove crossings between edges at the same node
	//TODO: should be automatically detected while inserting
	//crossings

	OGDF_ASSERT(PG.representsCombEmbedding());
	//remove consecutive crossings between two edges
	if (m_options & Options::Loop)
	{
		List<node> obsoleteCrossings;
		NodeArray<bool> processed(PG, false);
		for(node v : PG.nodes)
		{
			if (processed[v]) continue;
			if (v->degree() != 4) continue;
			if (!PG.isCrossingType(v)) continue;

			adjEntry ad1 = v->firstAdj();
			adjEntry adRun1 = ad1;
			adjEntry adRun2 = adRun1->cyclicSucc();
			do
			{
				node w = adRun2->twinNode();
				node u = adRun1->twinNode();
				if (w->degree() == 4
				 && w == u
				 && w != v
				 && PG.isCrossingType(w)
				 && !processed[w]) {
					obsoleteCrossings.pushBack(w);
					processed[w] = true;

					if (!processed[v]) { //obsolete
						processed[v] = true;
						obsoleteCrossings.pushBack(v);
						continue;
					}
				}
				adRun1 = adRun2;
				adRun2 = adRun2->cyclicSucc();
			} while (adRun1 != ad1);

		}

		OGDF_ASSERT((obsoleteCrossings.size() % 2) == 0);
		ListIterator<node> it = obsoleteCrossings.begin();
		while (it.valid())
		{
			PG.removeCrossing((*it));
			++it;
		}
	}

	OGDF_ASSERT(PG.representsCombEmbedding());

	if (m_options & Options::CrossFlip)
	{
		//this is useful only if the crossing is the first crossing
		//near the common node
		//check which edges are to be deleted by
		//removeunnecessaryCrossings, then flip
		//slow, should be detected when inserting crossings
		List<node> flipper;
		for(node v : PG.nodes)
		{
			//we remove the crossing and flip the edge order
			if (PG.isCrossingType(v))//(v->degree() == 4)
			{
				bool flip = checkFlipCrossing(PG, v, false);
				if (flip)
					flipper.pushBack(v);

			}
		}
		//TODO: we should reuse the computation above
		ListIterator<node> itFlip = flipper.begin();
		while (itFlip.valid())
		{
			checkFlipCrossing(PG, (*itFlip), true);
			++itFlip;
		}
	}

	OGDF_ASSERT(PG.representsCombEmbedding());
}

void TopologyModule::planarizeFromLayout(PlanRep &PG,
                                         GraphAttributes &GA)
{
	//Debug
	Layout xy(PG);
	for(node u : PG.nodes)
	{
		if (PG.original(u))
		{
			xy.x(u) = GA.x(PG.original(u));
			xy.y(u) = GA.y(PG.original(u));
		}
	}

	//First, we create a set of EdgeLegs representing the
	//segments in the given layout, we assign the (ASSERT!)
	//only copy of an edge as the EdgeLeg copy edge

	//Preliminary we use a list to save and process all edgelegs
	//TODO: Insert a storage here to sort the legs and process
	//them by sweepline
	//TODO: Check if pointer from edge to its legs is necessary
	//(via edgearray?)

	List<EdgeLeg*> legList;
	//--
	//test to get only the edges in cc
	//--

	for(int i = PG.startEdge(); i < PG.stopEdge(); ++i)
	{
		edge e = PG.e(i);
		//--

		//create edgeLeg objects for edge segments
		//while (e)
		//{
		//there may be edges in other CCs without representation
		if ((PG.chain(e)).size() == 0)
		{
			//TODO: this doesnt seem to be correct anymore, was it?
			//e = e->succ();
			continue;
		}
		//guarantee that there are no bends or crossings yet
		OGDF_ASSERT((PG.chain(e)).size() == 1);

		m_eLegs[e].clear();

		const DPolyline &bends1 = GA.bends(e);
		int legNumber = 0; //start legcount at 0
		//run over all segments
		double startX = GA.x(e->source());
		double startY = GA.y(e->source());
		double endX;
		double endY;

		//create all legs for bends
		ListConstIterator<DPoint> it = bends1.begin();
		while (it.valid())
		{
			endX = (*it).m_x;
			endY = (*it).m_y;

			//now we create an EdgeLeg and initialize it with
			//the only copy of e in PG
			EdgeLeg* eLeg = new EdgeLeg(PG.copy(e),legNumber,
				DPoint(startX, startY), DPoint(endX, endY));
			//insert the leg into the list of legs for e
			eLeg->m_eIterator = m_eLegs[e].pushBack(eLeg);
			//insert the leg into the crossing search list
			legList.pushBack(eLeg);


			//proceed with the next leg
			legNumber++;
			startX = endX;
			startY = endY;

			++it;
		}

		//create the final leg
		endX = GA.x(e->target());
		endY = GA.y(e->target());
		EdgeLeg* eLeg = new EdgeLeg(PG.copy(e),legNumber,
			DPoint(startX, startY), DPoint(endX, endY));
		//insert the leg into the list of legs for e
		eLeg->m_eIterator = m_eLegs[e].pushBack(eLeg);
		legList.pushBack(eLeg);
	}

	//Now we compute the crossings between segments, thereby
	//introducing new segments if crossings are detected
	//We add the newly created edges as edgelegs into the set of crossings,
	//which gives an computational overhead depending on the number
	//of crossings compared to the precomputation of crossings,
	//but helps in detecting the correct position of the crossing
	//on the edge (in the sorting order of the edge crossings)
	//We run over the list of edgelegs, delete the current edgeleg
	//from the list and compute its crossings with the edgelegs
	//remaining in the list. The crossings are inserted into the
	//PlanRep und the crossed (second) edgeleg is replaced by
	//the edgelegs introduced by the crossings.

	while (legList.size() > 0)
	{
		EdgeLeg* crossLeg = legList.popFrontRet();
		//compute crossings of crossLeg with all other edgeLegs
		ListIterator<EdgeLeg*> runIt = legList.begin();
		DPoint xp(0, 0); //crossing point
		//Lists of crossed EdgeLegs (their iterators in legList)
		//and the corresponding crossing points
		List<ListIterator< EdgeLeg* > > iterList;

		//from now on, we store the crossing in the edgeleg
		//(they are only there for this purpose)
		//List<DPoint> xpList;
		while (runIt.valid())
		{
			//special case of parallel generalization segments
			//at mergers in drawings
#if 0
			if (PG.isGeneralization(crossLeg->copyEdge())
			 && PG.isGeneralization((*runIt)->copyEdge())) {
				if ((crossLeg->start().m_x == (*runIt)->start().m_x)
				 && (crossLeg->start().m_y == (*runIt)->start().m_y)
				 && (crossLeg->end().m_x == (*runIt)->end().m_x)
				 && (crossLeg->end().m_y == (*runIt)->end().m_y)) {
					EdgeLeg* teste = (*runIt);
				}
			}
#endif
			if (hasCrossing(crossLeg, (*runIt), xp))
			{
				//check crossings for the special case
				//of two generalizations:
				if (PG.isGeneralization(crossLeg->copyEdge())
				 && PG.isGeneralization((*runIt)->copyEdge())
				 // TODO: if (m_options & Options::DegOneCrossings)
				 // Should be done first, then GenToAss... but for now:
				 && m_options & Options::GenToAss) {
					// TODO: check if we have to set all copies
					// TODO: check if we prefer an edge, e.g. most crossed
					// TODO: should we set original as well.
					edge converter = crossLeg->copyEdge();
					// check if incident to merger
					if (PG.typeOf(converter->source()) == Graph::NodeType::generalizationMerger
					 || PG.typeOf(converter->target()) == Graph::NodeType::generalizationMerger) {
						converter = (*runIt)->copyEdge();
					}
					// TODO: converter may still be incident to merger
					GA.type(PG.original(converter)) = Graph::EdgeType::association;
					PG.oriEdgeTypes(PG.original(converter)) = static_cast<long>(Graph::EdgeType::association);
					for (const edge e : PG.chain(PG.original(converter))) {
						PG.setAssociation(e);
					}
				}
				//first we store all crossings together
				//with their crossing point, these are only
				//used to compute an ordering
				//xpList.pushBack(xp);
				(*runIt)->m_xp = xp;
				iterList.pushBack(runIt);
				//then we check if we have topDown or
				//bottom up crossing direction
				//we have top Down crossing from runitleg
				//over crossleg, if the segment
				//crosslegs startpoint -> runitleg startpoint lies
				//clockwise before the segment defined by
				//crossleg
				EdgeComparer ec(GA, PG);
				DPoint u = crossLeg->start();
				DPoint v = (*runIt)->start();
				DPoint w = crossLeg->end();
				if (ec.before(u, v, w))
				//if increasing y values go up, before
				//gives the correct value, but we flip
					(*runIt)->m_topDown = false; //depends on y-axis direction
				else (*runIt)->m_topDown = true;
			}
			++runIt;
		}
		if (iterList.size() > 0)
		{
			// first sort the crossings corresponding to the
			// edgeleg direction
			if (iterList.size() > 1)
			{
				//just compute the distance to the start vertex
				//sqrt(sqr(x2-x1) + sqr(y2-y1));
				PointComparer pc(crossLeg->start());
				iterList.quicksort(pc);
			}
			// then we insert the detected crossings:
			// we insert the crossing into PG and update the
			// crossed edgeLeg, insert the new edge leg
			// we do not have to worry about crossLeg, because
			// after this loop, we will never return there (no
			// more crossings possible)
			ListIterator< ListIterator< EdgeLeg* > > it = iterList.begin();
			//ListIterator<DPoint> itXp = xpList.begin();

			while (it.valid())
			{
				//we insert the crossing for the current leg
				ListIterator< EdgeLeg* > itLeg = (*it);

				//before we create the crossing and thereby destroy the
				//crossLeg copy edge, we save all subsequent edgelegs on
				//the same copy edge to update the afterwards
				List<EdgeLeg*> affectedSegments;
				ListIterator< EdgeLeg* > itSearch2 = crossLeg->m_eIterator;
				++itSearch2; //crossLeg itself is automatically updated in insertcrossing
				while (itSearch2.valid() &&
					( (*itSearch2)->copyEdge() ==  crossLeg->copyEdge()) )
				{
					affectedSegments.pushBack((*itSearch2));
					++itSearch2;
				}
				//TODO: check if automatic update of new crossleg
				//copyedge works after first crossing
				//we implicitly change the crossleg edge and do not
				//introduce new edgelegs for the crossing edge,
				//because they won't be used anymore
				OGDF_ASSERT(PG.original((*itLeg)->copyEdge()));
				edge newEdge = PG.insertCrossing(crossLeg->copyEdge(),
					(*itLeg)->copyEdge(), ((*itLeg)->m_topDown));
				m_crossPosition[newEdge->source()] = (*itLeg)->m_xp;//(*itXp);
				OGDF_ASSERT(PG.original((*itLeg)->copyEdge()));
				//automatischer Seiteneffekt
				//crossLeg->copyEdge() = newEdge;
				//debug
				xy.x(newEdge->source()) = (*itLeg)->m_xp.m_x;
				xy.y(newEdge->source()) = (*itLeg)->m_xp.m_y;

				//insert the edgeleg for newEdge and update (*itleg)
				//accordingly (endpoint is now crossing point, TODO: depending
				//on the direction!)

				//TODO: check: legnumber should be ok, otherwise
				//we would have to update it for all subsequent legs
				//it only tells us where we are in the bend ordering of the
				//original edge (after legnumber #bends) , no change after crossing
				EdgeLeg* eLeg = new EdgeLeg(newEdge,(*itLeg)->number(),
					//DPoint((*itXp)),
					(*itLeg)->m_xp,
					(*itLeg)->end()); //TODO: decide end/start
					//depending on direction
				(*itLeg)->end() = (*itLeg)->m_xp;//DPoint((*itXp));

				//we insert the new leg into the list of legs for e
				//TODO: we have to check the right direction??
				//muesste automatisch immer hinter der alten Kante liegen
				edge searchEdge = (*itLeg)->copyEdge();
				OGDF_ASSERT(PG.original(searchEdge) == PG.original(newEdge));
				eLeg->m_eIterator =
					m_eLegs[PG.original(newEdge)].insert(eLeg, (*itLeg)->m_eIterator);
				//We update (depending on the direction) all
				//following edgelegs with the same copy edge to newedge
				ListIterator< EdgeLeg* > itSearch = eLeg->m_eIterator;
				++itSearch; //eLeg itself is ok
				while (itSearch.valid() && ( (*itSearch)->copyEdge() == searchEdge ) )
				{
					(*itSearch)->copyEdge() = newEdge;
					++itSearch;
				}
				//We update (depending on the direction) all
				//following edgelegs with the same copy edge to newedge
				itSearch2 = affectedSegments.begin();
				while (itSearch2.valid())
				{
					(*itSearch2)->copyEdge() = crossLeg->copyEdge();
					++itSearch2;
				}

				//TODO: we simply push the new leg into the list,
				// check if this can cause problems
				legList.pushBack(eLeg);

				//itXp++;
				++it;
			}
		}
	}

	//delete the created edgeLegs
	ListIterator<EdgeLeg*> legIt = legList.begin();
	while (legIt.valid())
	{
		EdgeLeg* el = (*legIt);
		delete el;
		++legIt;
	}

#ifdef OGDF_DEBUG
#ifdef OGDF_TOPOLOGY_MODULE_DEBUG_OUTPUT
	((PlanRepInc&)PG).genusLayout(xy);
	((PlanRepInc&)PG).writeGML("planarisiert.gml", xy);
#endif
#endif
}

//compute sum of angles in face
double TopologyModule::faceSum(PlanRep &PG,
	const GraphAttributes &GA, face f)
{
	DPolyline pl;
	double rho = 0;
	DPolyline B;

	//Run through the faces and count the angles.
	//we can't run over original edges because segments
	//may lie in different faces after crossing insertion.
	//(bends are not yet replaced by nodes, crossings can
	//therefore split them)
	//on every copy edge the original edge is unambigious
	//we can check at which point in the range of crossings
	//we lie //unefficient if many crossings/bends, but we
	//can not efficiently store this at the CROSSED edge
	//when inserting crossings

	adjEntry iti = f->firstAdj();
	while (iti != nullptr)
	{
		//list of points along copy iti
		B.clear();
		edge eOrig = PG.original(iti->theEdge());
		//only the source node is considered in CASE A
		//we do not use endpoints(would be doublettes)
		node srcNode = PG.original((iti)->theNode());
		DPolyline dp = GA.bends(eOrig);
		//check direction
		bool reversed = (iti)->theNode() != (iti)->theEdge()->source();
		if (reversed)
			dp.reverse();

		//CASE A: no crossings
		if (PG.chain(PG.original(iti->theEdge())).size() == 1 )
		{
			OGDF_ASSERT(PG.original((iti)->theNode()));
			OGDF_ASSERT(PG.original((iti->twin())->theNode()));

			B.pushFront(GA.point(srcNode));

			B.conc(dp);
			pl.conc(B);

			iti = f->nextFaceEdge(iti);
			continue;
		}

		//CASE B: we have crossings, but no bends on the edge
		if (dp.empty())
		{
			DPoint dp1;
			if (srcNode)
				dp1 = GA.point(srcNode);
			else
				dp1 = DPoint(m_crossPosition[(iti)->theNode()]);

			pl.pushBack(dp1);

			iti = f->nextFaceEdge(iti);
			continue;
		}

		//CASE C: we have crossings on the original edge,
		//we need to exactly define the list of bends/crossings
		//on the COPY edge (bends are not nodes here!)

		node tgtNode = PG.original((iti->twin())->theNode());
		//three cases:
		//1) From original to crossing
		bool case1 = (srcNode != nullptr);
		//2) From crossing to original
		bool case2 = (tgtNode != nullptr);
		//3) From crossing to crossing
		//fuers debuggen eigentlich testen: beide Kreuzungen
		bool case3 = !(case1 || case2);

		//We basically run through all the bends on the original edge
		//to check where the crossing lies
		//maybe we can save some time if we save this for consecutive
		//segments of the same edge (but may be non-contiguous)

		//die drei cases in Funktion DPolyline getSegmentBends() packen
		if (case1)
		{
			OGDF_ASSERT(!case2);
			OGDF_ASSERT((iti->twin())->theNode()->degree() == 4);
			DPoint sNode = GA.point(srcNode);
			//sourceNode is on face border
			B.pushFront(sNode);
			//search segment with crossing point
#if 0
			if (!(dp.empty())) // always a bend in Case C
#endif
			{
				DPoint p1 = m_crossPosition[(iti->twin())->theNode()];

				//Searchloop
				ListIterator<DPoint> it = dp.begin();
				DPoint tNode = (*it);
				DSegment dl(sNode, tNode);
				while(!(dl.contains(p1)))
				{
					B.pushBack(tNode);
					it = it.succ();
					sNode = tNode;
					if (it.valid())
					{
						tNode = (*it);
						dl = DSegment(sNode, tNode);
					}
					else break;
				}
			}
		}

		//case 2: segment from crossing to original node
		if (case2)
		{
			OGDF_ASSERT(!case1);
			OGDF_ASSERT(iti->theNode()->degree() == 4);

			DPoint dp1 = m_crossPosition[(iti)->theNode()];
			B.pushFront(dp1);
			//we start searching from the edge's original start node
			node srcONode = (reversed ? eOrig->target() : eOrig->source());
			DPoint sNode = GA.point(srcONode);
				//GA.point(srcNode));

			//Searchloop
			ListIterator<DPoint> it = dp.begin();
			DPoint tNode = (*it);
			DSegment dl(sNode, tNode);
			while(!(dl.contains(dp1)))
			{
				it = it.succ();
				sNode = tNode;
				if (it.valid())
				{
					tNode = (*it);
					dl = DSegment(sNode, tNode);
				}
				else break;
			}
			while (it.valid())
			{
				B.pushBack((*it));
				++it;
			}
		}
		if (case3)
		{
			//first, we search for the position of the first crossing,
			//then look for the second
			DPoint dp1 = m_crossPosition[(iti)->theNode()];
			DPoint dp2 = m_crossPosition[(iti->twin())->theNode()];

			B.pushFront(dp1);
			//we start searching from the edge's original start node
			node srcONode = (reversed ? eOrig->target() : eOrig->source());
			DPoint sNode = GA.point(srcONode);
			//DPoint sNode = GA.point(srcNode);
			//Searchloop1
			ListIterator<DPoint> it = dp.begin();
			DPoint tNode = (*it);
			DSegment dl(sNode, tNode);
			while (!(dl.contains(dp1)))
			{
				it = it.succ();
				sNode = tNode;
				if (it.valid())
				{
					tNode = (*it);
					dl = DSegment(sNode, tNode);
				}
				else break;
			}
			//are there any bends after crossing 1?
			if (it.valid())
			{
				//search for dp2
				while (!(dl.contains(dp2)))
				{
					B.pushBack(tNode);
					it = it.succ();
					sNode = tNode;
					if (it.valid())
					{
						tNode = (*it);
						dl = DSegment(sNode, tNode);
					}
					else break;
				}
			}
		}

		pl.conc(B);
		//it++;
		iti = f->nextFaceEdge(iti);
	}

	for (ListIterator<DPoint> it = pl.begin(); it.valid(); ++it) {
		DPoint p = *it;
		DPoint r = *pl.cyclicSucc(it);
		DPoint q = *pl.cyclicPred(it);
		// XXX: swap r, q?
		rho += p.angle(r, q) - Math::pi;
	}

	return rho;
}

face TopologyModule::getExternalFace(
	PlanRep &PG,
	const GraphAttributes &GA)
{
	CombinatorialEmbedding E(PG); //computes faces

	for(face f : E.faces)
	{
		if (faceSum(PG, GA, f) < 0) return f;

	}

	//debug or release? sollte das abgefangen werden??
	throw AlgorithmFailureException(AlgorithmFailureCode::ExternalFace);

#if 0
	return 0;
#endif
}


bool TopologyModule::skipable(EdgeLeg* legA, EdgeLeg* legB)
{
	//we spare out edgelegs of the same edge
	//TODO: if we widen the definition for allowed input shapes,
	//this should be reviewed, e.g. selfcrossings
	if (legA->copyEdge() == legB->copyEdge()) return true;
	//we check if endpoints of one edge lie on the other edge
	//which can be the case for merger (this can also be the case
	//if an edge cuts a vertex, we have to reembed then)
	//TODO: SPEED do not allocate these variables twice
	double x1 = legA->start().m_x;
	double x2 = legA->end().m_x;
	double y1 = legA->start().m_y;
	double y2 = legA->end().m_y;

	double xB1 = legB->start().m_x;
	double xB2 = legB->end().m_x;
	double yB1 = legB->start().m_y;
	double yB2 = legB->end().m_y;
	DPoint s1(x1, y1), t1(x2, y2),
		s2(xB1, yB1), t2(xB2, yB2);
	DSegment l1(s1,t1), l2(s2,t2);
	if (l1.contains(s2) || l1.contains(t2) ||
		l2.contains(s1) || l2.contains(t1))
		return true;
	return false;
}


//TODO: implement this efficiently, do not create all the
//variables every time
//returns true if edgeLegs leg1 and leg2 cross each other
bool TopologyModule::hasCrossing(EdgeLeg* legA, EdgeLeg* legB, DPoint& xp)
{
	//we can skip this for same special cases
	if (skipable(legA, legB)) return false;

	//Could be more efficient with integers...
	double x1 = legA->start().m_x;
	double x2 = legA->end().m_x;
	double y1 = legA->start().m_y;
	double y2 = legA->end().m_y;

	double xB1 = legB->start().m_x;
	double xB2 = legB->end().m_x;
	double yB1 = legB->start().m_y;
	double yB2 = legB->end().m_y;

	double aVal1 = x2 - x1;
	double aVal2 = y2 - y1;
	double bVal1 = xB2 - xB1;
	double bVal2 = yB2 - yB1;

	if ( ((OGDF_GEOM_ET.less(aVal1*yB1 - aVal2*xB1,
		aVal1*y1  - aVal2*x1))
		^
		(OGDF_GEOM_ET.less(aVal1*yB2 - aVal2*xB2,
		aVal1*y1  - aVal2*x1)))
		&&
		((OGDF_GEOM_ET.less(bVal1*y1 - bVal2*x1,
		bVal1*yB1 - bVal2*xB1))
		^
		(OGDF_GEOM_ET.less(bVal1*y2  - bVal2*x2,
		bVal1*yB1 - bVal2*xB1)))
		)
	{
		// we only do the geometric computation with division
		// if really necessary
		DPoint s1(x1, y1), t1(x2, y2),
			s2(xB1, yB1), t2(xB2, yB2);
		DSegment l1(s1,t1), l2(s2,t2);

		// TODO: What to do when IntersectionType::Overlapping is returned?
		bool result = l1.intersection(l2, xp, false) == IntersectionType::SinglePoint;
		//due to the false parameter, this might not be the case:
		//OGDF_ASSERT(result);
		return result;
	}
	return false;
}


bool TopologyModule::checkFlipCrossing(PlanRep& PG, node v, bool flip)
{
	if (v->indeg() != 2) return false;

	if (PG.isCrossingType(v))
	{
		bool crossing = false;
		OGDF_ASSERT(v->degree() == 4);
		adjEntry a1 = v->firstAdj();
		adjEntry b1 = a1->cyclicSucc();
		adjEntry a2 = b1->cyclicSucc();
		adjEntry b2 = a2->cyclicSucc();

		//check if the edges have a common endpoint
		node va1 = a1->twinNode();
		node va2 = a2->twinNode();
		node vb1 = b1->twinNode();
		node vb2 = b2->twinNode();

		auto noFlipping = [&](adjEntry a, adjEntry b) {
			if (m_options & Options::FlipUML) {
				const bool isGenA = PG.isGeneralization(a->theEdge());
				const bool isGenB = PG.isGeneralization(b->theEdge());
				return (isGenA ^ isGenB) != 0;
			}
			return false;
		};

		//to match the crossflip case, two successive edges
		//need to have the same endpoint, with original node
		if (PG.original(va1))
		{
			if (va1 == vb1) //case1
			{
				if (noFlipping(a1, b1)) {
					return false;
				}
				crossing = true;
				//remove the crossing and flip the order
				//in removeCrossing, the first two adjacency entries survive
				if (flip)
				{
					PG.removeCrossing(v);
					//TODO: check did the adjentries survive?
					//TODO: check is it behind or before adj?

					if (a1->twin() == b1->twin()->cyclicSucc())
					{
						PG.moveAdj(a1->twin(), Direction::before,b1->twin());
					}
					else
					{
						OGDF_ASSERT(a1->twin() == b1->twin()->cyclicPred());
					}
				}

			}
			else //TODO: check can both b1, b2 be the same?
				if (va1 == vb2)
				{
					if (noFlipping(a1, b2)) {
						return false;
					}
					//see comments  in case1
					crossing = true;
					if (flip)
					{
						PG.removeCrossing(v);
						OGDF_ASSERT(a1->twin() == b1->cyclicPred());
						PG.moveAdj(a1->twin(), Direction::after, b1);
					}
				}
		}
		if (PG.original(va2))
		{
			if (va2 == vb1)
			{
				//check if flipping allowed
				if (noFlipping(a2, b1)) {
					return false;
				}
				crossing = true;
				//see comments  in case1
				if (flip)
				{
					PG.removeCrossing(v);
					OGDF_ASSERT(a1 == b1->twin()->cyclicPred());
					PG.moveAdj(a1, Direction::after,b1->twin());
				}
			}
			else
				if (va2 == vb2)
				{
					if (noFlipping(a2, b2)) {
						return false;
					}
					crossing = true;
					if (flip)
					{
						PG.removeCrossing(v);
						OGDF_ASSERT(a1 == b1->cyclicSucc());
						PG.moveAdj(a1, Direction::before,b1);
					}
				}
		}
		return crossing;
	}

	return false;
}

///*copied back from umlgraph
//sorts the edges around all nodes of GA corresponding to the
//layout given in GA
//there is no check of the embedding afterwards because this
//method could be used as a first step of a planarization
void TopologyModule::sortEdgesFromLayout(Graph &G, GraphAttributes& GA)
{
	//we order the edges around each node corresponding to
	//the input embedding in the GraphAttributes layout
	NodeArray<SListPure<adjEntry> > adjList(G);

	EdgeComparer ec(GA);

	for(node v : G.nodes)
	{
		for(adjEntry ae : v->adjEntries)
		{
			adjList[v].pushBack(ae);
		}
		//sort the entries
		adjList[v].quicksort(ec);
		G.sort(v, adjList[v]);
	}
}

}
