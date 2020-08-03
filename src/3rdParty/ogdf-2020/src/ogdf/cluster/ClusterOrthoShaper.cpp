/** \file
 * \brief Computes the Orthogonal Representation of a Planar
 * Representation of a UML Graph.
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


#include <ogdf/cluster/ClusterOrthoShaper.h>
#include <ogdf/graphalg/MinCostFlowReinelt.h>

namespace ogdf {

enum class NetArcType {defaultArc, angle, backAngle, bend};

//call function: compute a flow in a dual network and interpret
//result as bends and angles (representation shape)
void ClusterOrthoShaper::call(ClusterPlanRep &PG,
	CombinatorialEmbedding &E,
	OrthoRep &OR,
	int startBoundBendsPerEdge,
		bool fourPlanar)
	{

	if (PG.numberOfEdges() == 0)
		return;

	m_fourPlanar = fourPlanar;

	// the min cost flow we use
	MinCostFlowReinelt<int> flowModule;
	const int infinity = flowModule.infinity();

	//fix some values depending on traditional or progressive mode

	//standard flow boundaries for traditional and progressive mode
	const int upperAngleFlow  = (m_traditional ? 4 : 1); //non zero
	const int maxAngleFlow = (m_traditional ? 4 : 2); //use 2 for multialign zero degree
	const int maxBackFlow     = 2; //maximal flow on back arcs in progressive mode
	const int upperBackAngleFlow = 2;   // and 360 back (only progressive mode)
	const int lowerAngleFlow  = (m_traditional ? 1 : 0);
	const int piAngleFlow     = (m_traditional ? 2 : 0);
	const int halfPiAngleFlow = 1;
#if 0
	const int halfPiBackAngleFlow = 0;  //(only progressive mode)
#endif
	const int zeroAngleFlow   = (m_traditional ? 0 : 2);
	const int zeroBackAngleFlow   = 0;    //(only progressive mode)

	//in progressive mode, angles need cost to work out properly
#if 0
	const int tradAngleCost  = 0;
#endif
	const int progAngleCost  = 1;
	const int tradBendCost   = 1;
	const int progBendCost   = 3*PG.numberOfNodes(); //should use supply
	PG.getClusterGraph().setUpdateDepth(true);
	const int clusterTreeDepth = PG.getClusterGraph().treeDepth();


	OR.init(E);
	FaceArray<node> F(E);

	OGDF_ASSERT(PG.representsCombEmbedding());
	OGDF_ASSERT(F.valid());


	// NETWORK VARIABLES

	Graph Network; //the dual network
	EdgeArray<int>  lowerBound(Network,0); // lower bound for flow
	EdgeArray<int>  upperBound(Network,0); // upper bound for flow

	EdgeArray<int>  cost(Network,0);       // cost of an edge
	NodeArray<int>  supply(Network,0);     // supply of every node


	//alignment helper
	NodeArray<bool> fixedVal(Network, false);  //already set somewhere
	EdgeArray<bool> noBendEdge(Network, false); //for splitter, brother edges etc.

	//NETWORK TO PlanRep INFORMATION

	// stores for edges of the Network the corresponding adjEntries
	// nodes, and faces of PG
	EdgeArray<adjEntry> adjCor(Network,nullptr);
	EdgeArray<node>		nodeCor(Network,nullptr);
	EdgeArray<face>		faceCor(Network,nullptr);

	NodeArray<n_type> nodeTypeArray(Network, n_type::low);

	//PlanRep TO NETWORK INFORMATION

	//Contains for every node of PG the corresponding node in the network
	NodeArray<node>		networkNode(PG,nullptr);
	//Contains for every adjEntry of PG the corresponding edge in the network
	AdjEntryArray<edge>	backAdjCor(PG,nullptr); //bends
	//contains for every adjEntry of PG the corresponding angle arc in the network
	//note: this doesn't need to correspond to resulting drawing angles
	//bends on the boundary define angles at expanded nodes
	AdjEntryArray<edge> angleArc(PG, nullptr); //angle
	//contains the corresponding back arc face to node in progressive mode
	AdjEntryArray<edge> angleBackArc(PG, nullptr); //angle

	// OTHER INFORMATION

	// Contains for adjacency Entry of PG the face it belongs to in PG
	AdjEntryArray<face>  adjF(PG,nullptr);

	//Contains for angle network arc progressive mode backward arc
	EdgeArray<edge> angleTwin(Network, nullptr);

	auto setProgressiveBoundsEqually = [&](edge e, int flow, int flowTwin) {
		upperBound[e] = lowerBound[e] = flow;
		const edge aTwin = angleTwin[e];
		if (aTwin != nullptr) {
			upperBound[aTwin] = lowerBound[aTwin] = flowTwin;
		}
	};
	auto setBoundsEqually = [&](edge e, int flow, int flowTwin) {
		if (m_traditional) {
			upperBound[e] = lowerBound[e] = flow;
		} else {
			setProgressiveBoundsEqually(e, flow, flowTwin);
		}
	};

	//types of network edges, to be used in flow to values
	EdgeArray<NetArcType> l_arcType(Network, NetArcType::angle);

	//contains the outer face
	//face theOuterFace = E.externalFace();


	// GENERATE ALL NODES OF THE NETWORK

	//corresponding to the graphs nodes
	int checksum = 0;
	for(node v : PG.nodes)
	{
		OGDF_ASSERT((!m_fourPlanar) || (v->degree() < 5));

		networkNode[v] = Network.newNode();
		//maybe install a shortcut here for degree 4 nodes if not expanded

		if (v->degree() > 4) nodeTypeArray[networkNode[v]] = n_type::high;
		else nodeTypeArray[networkNode[v]] = n_type::low;

		//already set the supply
		if (m_traditional) supply[networkNode[v]] = 4;
		else supply[networkNode[v]] = 2*v->degree() - 4;

		checksum += supply[networkNode[v]];
	}

	//corresponding to the graphs faces
	for (face f : E.faces)
	{
		F[f] = Network.newNode();

		if (f == E.externalFace())
		{
			nodeTypeArray[F[f]] = n_type::outer;
			if (m_traditional) supply[F[f]] = - 2*f->size() - 4;
			else supply[F[f]] = 4;
		}
		else {
			nodeTypeArray[F[f]] = n_type::inner;
			if (m_traditional) supply[F[f]] = - 2*f->size() + 4;
			else supply[F[f]] = -4;
		}
	}

#ifdef OGDF_DEBUG
	//check the supply sum
	checksum = 0;
	for(node v : Network.nodes)
		checksum += supply[v];
	OGDF_ASSERT(checksum == 0);
#endif


#ifdef OGDF_HEAVY_DEBUG
	for(node v : PG.nodes)
		Logger::slout() << " v = " << v << " corresponds to "
		<< networkNode[v] << std::endl;
	for (face f : E.faces) {
		Logger::slout() << " face = " << f->index() << " corresponds to " << F[f];
		if (f == E.externalFace())
			Logger::slout() << " (Outer Face)";
		Logger::slout() << std::endl;
	}
#endif


	// GENERATE ALL EDGES OF THE NETWORK

	// OPTIMIZATION POTENTIAL:
	// Do not insert edges with upper bound 0 into the network.

	// Locate for every adjacency entry its adjacent faces.
	for (face f : E.faces)
	{
		for(adjEntry adj : f->entries)
			adjF[adj] = f;
	}

#ifdef OGDF_HEAVY_DEBUG
	for(face f : E.faces) {
		Logger::slout() << "Face " << f->index() << " : ";
		for(adjEntry adj : f->entries)
			Logger::slout() << adj << "; ";
		Logger::slout() << std::endl;
	}
#endif

	// Insert for every edge the (two) network arcs
	// entering the face nodes, flow defines bends on the edge
	for(edge e : PG.edges)
	{
		OGDF_ASSERT(adjF[e->adjSource()]);
		OGDF_ASSERT(adjF[e->adjTarget()]);
		if (F[adjF[e->adjSource()]] != F[adjF[e->adjTarget()]])
		{
			// not a selfloop.
			edge newE = Network.newEdge(F[adjF[e->adjSource()]],F[adjF[e->adjTarget()]]);

			l_arcType[newE] = NetArcType::bend;

			adjCor[newE] = e->adjSource();
			if ( (PG.typeOf(e) == Graph::EdgeType::generalization) ||
				(PG.isClusterBoundary(e) && (!m_traditional)))
				upperBound[newE] = 0;
			else
				upperBound[newE] = infinity;
			cost[newE] = (m_traditional ?
				clusterTradBendCost(PG.getClusterGraph().clusterDepth(PG.clusterOfEdge(e)), clusterTreeDepth, tradBendCost) :
				clusterProgBendCost(PG.getClusterGraph().clusterDepth(PG.clusterOfEdge(e)), clusterTreeDepth, progBendCost));

			backAdjCor[e->adjSource()] = newE;

			newE = Network.newEdge(F[adjF[e->adjTarget()]],F[adjF[e->adjSource()]]);

			l_arcType[newE] = NetArcType::bend;

			adjCor[newE] = e->adjTarget();
			if ((PG.typeOf(e) == Graph::EdgeType::generalization) ||
				(PG.isClusterBoundary(e) && (m_traditional)))
				upperBound[newE] = 0;
			else
				upperBound[newE] = infinity;
			cost[newE] = (m_traditional ?
				clusterTradBendCost(PG.getClusterGraph().clusterDepth(PG.clusterOfEdge(e)), clusterTreeDepth, tradBendCost) :
				clusterProgBendCost(PG.getClusterGraph().clusterDepth(PG.clusterOfEdge(e)), clusterTreeDepth, progBendCost));
			backAdjCor[e->adjTarget()] = newE;
		}
	}


	// insert for every node edges to all appearances of adjacent faces
	// flow defines angles at nodes
	// progressive: and vice-versa


	// Observe that two generalizations are not allowed to bend on
	// a node. There must be a 180 degree angle between them.

	// assure that there is enough flow between adjacent generalizations
	NodeArray<bool> genshift(PG, false);

	//non-expanded vertex
	for(node v : PG.nodes)
	{
		// Locate possible adjacent generalizations
		adjEntry gen1 = nullptr;
		adjEntry gen2 = nullptr;

		if (PG.typeOf(v) != Graph::NodeType::generalizationMerger
			&& PG.typeOf(v) != Graph::NodeType::generalizationExpander)
		{
			for(adjEntry adj : v->adjEntries)
			{
				if (PG.typeOf(adj->theEdge()) == Graph::EdgeType::generalization)
				{
					if (!gen1) gen1 = adj;
					else gen2 = adj;
				}
			}
		}

		for(adjEntry adj : v->adjEntries)
		{
			edge e2 = Network.newEdge(networkNode[v],F[adjF[adj]]);

			l_arcType[e2] = NetArcType::angle;

			//CHECK bounded edges? and upper == 2 for zero degree
			//progressive and traditional
			upperBound[e2] = upperAngleFlow;
			nodeCor   [e2] = v;
			adjCor    [e2] = adj;
			faceCor   [e2] = adjF[adj];
			angleArc  [adj] = e2;

			//do not allow zero degree at non-expanded vertex
			//&& !m_allowLowZero

			//progressive and traditional (compatible)
			if (m_fourPlanar) lowerBound[e2] = lowerAngleFlow; //trad 1 = 90, prog 0 = 180

			//insert opposite arcs face to node in progressive style
			edge e3;
			if (!m_traditional)
			{
				e3 = Network.newEdge(F[adjF[adj]], networkNode[v]); //flow for >180 degree

				l_arcType[e3] = NetArcType::backAngle;

				angleTwin[e2] = e3;
				angleTwin[e3] = e2;

				cost[e2] = progAngleCost;
				cost[e3] = progAngleCost;

				lowerBound[e3] = lowerAngleFlow; //180 degree,check highdegree drawings
				upperBound[e3] = upperBackAngleFlow; //infinity;
#if 0
				nodeCor   [e3] = v; has no node, is face to node
#endif
				adjCor    [e3] = adj;
				faceCor   [e3] = adjF[adj];
				angleBackArc[adj] = e3;

			}
		}

		//second run to have all angleArcs already initialized
		//set the flow boundaries
		for(adjEntry adj : v->adjEntries)
		{
			edge e2 = angleArc[adj];

			//allow low degree node zero degree angle for non-expanded vertices
#if 0
			if (false) {
				if ((gen1 || gen2) && (PG.isVertex(v)))
				{
					lowerBound[e2] = 0;
				}
			}
#endif

			//hier muss man fuer die Kanten, die rechts ansetzen noch lowerbound 2 setzen

			if ((gen2 == adj && gen1 == adj->cyclicSucc())
			 || (gen1 == adj && gen2 == adj->cyclicSucc())) {
				setBoundsEqually(e2, piAngleFlow, 0);
				genshift[v] = true;
			}
		}
	}


	// Reset upper and lower Bounds for network arcs that
	// correspond to edges of generalizationmerger faces
	// and edges of expanded nodes.

	for(node v : PG.nodes)
	{
		if (PG.expandAdj(v))
		{
			// Get the corresponding face in the original embedding.
			face f = adjF[PG.expandAdj(v)];

			//expanded merger cages
			if (PG.typeOf(v) == Graph::NodeType::generalizationMerger)
			{
				// Set upperBound to 0  for all edges.
				for(adjEntry adj : f->entries)
				{
					//no bends on boundary (except special case following)
					upperBound[backAdjCor[adj]] = 0;
					upperBound[backAdjCor[adj->twin()]] = 0;

					// Node w is in Network
					node w = networkNode[adj->twinNode()];
					for(adjEntry adjW : w->adjEntries) {
						edge e = adjW->theEdge();
						if (e->target() == F[f]) {
							// is this: 180 degree?
							// traditional: 2 progressive: 0
							// if not traditional, limit angle back arc
							setBoundsEqually(e, piAngleFlow, 0);
						}
					}

				}
				//special bend case
				// Set the upper and lower bound for the first edge of
				// the mergeexpander face to guarantee a 90 degree bend.
				if (m_traditional)
				{
					upperBound[backAdjCor[PG.expandAdj(v)]] = 1;
					lowerBound[backAdjCor[PG.expandAdj(v)]]= 1;
				}
				else
				{
					//progressive mode: bends are in opposite direction
					upperBound[backAdjCor[PG.expandAdj(v)->twin()]] = 1;
					lowerBound[backAdjCor[PG.expandAdj(v)->twin()]]= 1;
				}

				// Set the upper and lower bound for the first node in
				// clockwise order of the mergeexpander face to
				// guaranty a 90 degree angle at the node in the interior
				// and a 180 degree angle between the generalizations in the
				// exterior.
				node secFace;

				if (F[f] == backAdjCor[PG.expandAdj(v)]->target())
					secFace = backAdjCor[PG.expandAdj(v)]->source();
				else {
					OGDF_ASSERT(F[f] == backAdjCor[PG.expandAdj(v)]->source());
					// Otherwise: Edges in Network mixed up.
					secFace = backAdjCor[PG.expandAdj(v)]->target();
				}
				node w = networkNode[PG.expandAdj(v)->twinNode()];

				adjEntry adjFound = nullptr;
				for(adjEntry adj : w->adjEntries)
				{
					if (adj->theEdge()->target() == F[f]) {
						// if not traditional, limit angle back arc
						setBoundsEqually(adj->theEdge(), 1, 0);
						adjFound = adj;
						break;
					}
				}

				edge e;
				if (m_traditional)
					e = adjFound->cyclicSucc()->theEdge();
				else
				{
					//we have two edges instead of one per face
					adjEntry ae = adjFound->cyclicSucc();
					e = ae->theEdge();
					if (e->target() != secFace)
						//maybe we have to jump one step further
						e = ae->cyclicSucc()->theEdge();

				}

				if (e->target() == secFace) {
					setBoundsEqually(e, piAngleFlow, piAngleFlow);
				}

				// Set the upper and lower bound for the last edge of
				// the mergeexpander face to guarantee a 90 degree bend.
				if (m_traditional)
				{
					upperBound[backAdjCor[PG.expandAdj(v)->faceCyclePred()]] = 1;
					lowerBound[backAdjCor[PG.expandAdj(v)->faceCyclePred()]] = 1;
				}
				else
				{
					//progressive mode: bends are in opposite direction
					upperBound[backAdjCor[PG.expandAdj(v)->faceCyclePred()->twin()]] = 1;
					lowerBound[backAdjCor[PG.expandAdj(v)->faceCyclePred()->twin()]] = 1;
				}

				// Set the upper and lower bound for the last node in
				// clockwise order of the mergeexpander face to
				// guaranty a 90 degree angle at the node in the interior
				// and a 180 degree angle between the generalizations in the
				// exterior.
				if (F[f] == backAdjCor[PG.expandAdj(v)->faceCyclePred()]->target())
					secFace = backAdjCor[PG.expandAdj(v)->faceCyclePred()]->source();
				else if (F[f] == backAdjCor[PG.expandAdj(v)->faceCyclePred()]->source())
					secFace = backAdjCor[PG.expandAdj(v)->faceCyclePred()]->target();
				else
				{
					OGDF_ASSERT(false); // Edges in Network mixed up.
				}
				w = networkNode[PG.expandAdj(v)->faceCyclePred()->theNode()];

				adjFound = nullptr;
				for(adjEntry adj : w->adjEntries)
				{
					if (adj->theEdge()->target() == F[f]) {
						setBoundsEqually(adj->theEdge(), 1, 0);
						adjFound = adj;
						break;
					}
				}

				if (m_traditional)
					e = adjFound->cyclicPred()->theEdge();
				else
				{
					//we have two edges instead of one per face
					adjEntry ae = adjFound->cyclicPred();
					e = ae->theEdge();
					if (e->target() != secFace)
						//maybe we have to jump one step further
						e = ae->cyclicPred()->theEdge();
				}

				if (e->target() == secFace) {
					setBoundsEqually(e, piAngleFlow, piAngleFlow);
				}
			}

			//expanded high degree cages
			else if (PG.typeOf(v) == Graph::NodeType::highDegreeExpander )
			{
				// Set upperBound to 1 for all edges, allowing maximal one
				// 90 degree bend.
				// Set upperBound to 0 for the corresponding entering edge
				// allowing no 270 degree bend.
				// Set upperbound to 1 for every edge corresponding to the
				// angle of a vertex. This permitts 270 degree angles in
				// the face

				adjEntry splitter = nullptr;


				//assure that edges are only spread around the sides if not too
				//many multi edges are aligned

				//count multiedges at node
				int multis = 0;
				AdjEntryArray<bool> isMulti(PG, false);
				if (m_multiAlign)
				{
					//if all edges are multi edges, find a 360 degree position
					bool allMulti = true;
					for(adjEntry adj : f->entries) //this double iteration slows the algorithm down
					{
						//no facesplitter in attributedgraph
#if 0
						if (!PG.faceSplitter(adj->theEdge()))
#endif
						{
							adjEntry srcadj = adj->cyclicPred();
							adjEntry tgtadj = adj->twin()->cyclicSucc();
							//check if the nodes are expanded
							node vt1, vt2;
							if (PG.expandedNode(srcadj->twinNode()))
								vt1 = PG.expandedNode(srcadj->twinNode());
							else vt1 = srcadj->twinNode();
							if (PG.expandedNode(tgtadj->twinNode()))
								vt2 = PG.expandedNode(tgtadj->twinNode());
							else vt2 = tgtadj->twinNode();
							if (vt1 == vt2)
							{
								//we forbid bends between two incident multiedges
								if (m_traditional)
								{
									lowerBound[backAdjCor[adj]] = upperBound[backAdjCor[adj]] = 0;
									isMulti[adj] = true;
								}
								else
								{
									lowerBound[backAdjCor[adj->twin()]] =
										lowerBound[backAdjCor[adj]] =
										upperBound[backAdjCor[adj]] =
										upperBound[backAdjCor[adj->twin()]] = 0;
									isMulti[adj->twin()] = true;
								}
								multis++;
							} else {
								allMulti = false;
							}
						}
					}
					//multi edge correction: only multi edges => one edge needs 360 degree
					if (allMulti)
					{
						//find an edge that allows 360 degree without bends
						bool twoNodeCC = true; //no foreign non-multi edge to check for
						for(adjEntry adj : f->entries)
						{
							//now check for expanded nodes
							adjEntry adjOut = adj->cyclicPred(); //outgoing edge entry
							node vOpp = adjOut->twinNode();
							if (PG.expandedNode(vOpp))
							{
								adjOut = adjOut->faceCycleSucc(); //on expanded boundary
								//does not end on self loops
								node vStop = vOpp;
								if (PG.expandedNode(vStop)) vStop = PG.expandedNode(vStop);
								while (PG.expandedNode(adjOut->twinNode()) == vStop)
									//we are still on vOpps cage
									adjOut = adjOut->faceCycleSucc();
							}
							//now adjOut is either a "foreign" edge or one of the
							//original multi edges if two-node-CC
							//adjEntry testadj = adjCor[e]->faceCycleSucc()->twin();
							adjEntry testAdj = adjOut->twin();
							node vBack = testAdj->theNode();
							if (PG.expandedNode(vBack))
							{
								vBack = PG.expandedNode(vBack);
							}
							if (vBack != v) //v is expanded node
							{
								//dont use iteration result, set firstedge!
								upperBound[backAdjCor[adj]] = 4; //4 bends for 360
								twoNodeCC = false;
								break;
							}
						}
						//if only two nodes with multiedges are in current CC,
						//assign 360 degree to first edge
						if (twoNodeCC)
						{
							//it would be difficult to guarantee that the networkedge
							//on the other side of the face would get the 360, so alllow
							//360 for all edges or search for the outer face

							for(adjEntry adj : f->entries)
							{
								adjEntry ae = adj->cyclicPred();
								if (adjF[ae] == E.externalFace())
								{
									upperBound[backAdjCor[adj]] = 4; //4 bends for 360
									break;
								}
							}
						}
					}
					//End multi edge correction
				}

				//now set the upper Bounds
				for(adjEntry adj : f->entries)
				{
					//should be: no 270 degrees
					if (m_traditional)
						upperBound[backAdjCor[adj->twin()]] = 0;
					else
						upperBound[backAdjCor[adj]] = 0;

#if 0
					if (false)//(PG.faceSplitter(adj->theEdge()))
					{
						// No bends allowed  on the face splitter
						upperBound[backAdjCor[adj]] = 0;

						//CHECK
						//progressive??? sollte sein:
						upperBound[backAdjCor[adj->twin()]] = 0;

						splitter = adj;
						continue;
					}
					else
					//should be: only one bend
#endif
					{

						if (m_distributeEdges)
							//CHECK
							//maybe we should change this to setting the lower bound too,
							//depending on the degree (<= 4 => deg 90)
						{
							//check the special case degree >=4 with 2
							// generalizations following each other if degree
							// > 4, only 90 degree allowed, nodeType high
							// bloed, da nicht original
#if 0
							if (nodeTypeArray[ networkNode[adj->twinNode()] ] == high)
#endif
							//hopefully size is original degree
							if (m_traditional)
							{
								if (!isMulti[adj]) //m_multiAlign???
								{
									//check if original node degree minus multi edges
									//is high enough
									//Attention: There are some lowerBounds > 1
#ifdef OGDF_DEBUG
									int oldBound = upperBound[backAdjCor[adj]];
#endif
									if ((!genshift[v]) && (f->size()-multis>3))
										upperBound[backAdjCor[adj]] =
										//max(2, lowerBound[backAdjCor[adj]]);
										//due to mincostflowreinelt errors, we are not
										//allowed to set ub 1
										max(1, lowerBound[backAdjCor[adj]]);
									else upperBound[backAdjCor[adj]] =
										max(2, lowerBound[backAdjCor[adj]]);
									//nur zum Testen der Faelle
									OGDF_ASSERT(oldBound >= upperBound[backAdjCor[adj]]);
								}
							} else {
								//preliminary set the bound in all cases

								if (!isMulti[adj])
								{
									//Attention: There are some lowerBounds > 1

									if ((!genshift[v]) && (f->size()-multis>3))
										upperBound[backAdjCor[adj->twin()]] =
										max(1, lowerBound[backAdjCor[adj->twin()]]);
									else upperBound[backAdjCor[adj->twin()]] =
										max(2, lowerBound[backAdjCor[adj->twin()]]);

								}
							}
						}
					}

					// Node w is in Network
					node w = networkNode[adj->twinNode()];

#if 0
					if (w && !(m_traditional && m_fourPlanar && (w->degree() != 4)))
#endif
					{
						//should be: inner face angles set to 180
						for(adjEntry adjW : w->adjEntries) {
							edge e = adjW->theEdge();
							if (e->target() == F[f]) {
								setBoundsEqually(e, piAngleFlow, piAngleFlow);
							}
						}
					}
				}

				// In case a face splitter was used, we need to update the
				// second face of the cage.
				if (splitter)
				{
					// Get the corresponding face in the original embedding.
					face f2 = adjF[splitter->twin()];

					for(adjEntry adj : f2->entries)
					{
						if (adj == splitter->twin())
							continue;

						if (m_traditional)
							upperBound[backAdjCor[adj->twin()]] = 0;
						else //progressive bends are in opposite direction
							upperBound[backAdjCor[adj]] = 0;

						// Node w is in Network
						node w = networkNode[adj->twinNode()];
#if 0
						if (w && !(m_traditional && m_fourPlanar && (w->degree() != 4)))
#endif
						{
							for(adjEntry adjW : w->adjEntries) {
								edge e = adjW->theEdge();
								if (e->target() == F[f2]) {
									setBoundsEqually(e, piAngleFlow, piAngleFlow);
								}
							}
						}
					}
				}
			}
		} else {
			//non-expanded (low degree) nodes
			//check for alignment and for multi edges

			if (PG.isVertex(v))
			{
				node w = networkNode[v];
				if ((nodeTypeArray[w] != n_type::low) || (w->degree()<2)) continue;

				//check for multi edges and decrease lowerbound if align
				//int lowerb = 0;

				bool allMulti = true;
				for(adjEntry adj : w->adjEntries) {
					edge e = adj->theEdge();
					//lowerb += max(lowerBound[e], 0);

					OGDF_ASSERT((!m_traditional) || (e->source() == w));
					if (m_traditional && (e->source() != w)) OGDF_THROW(AlgorithmFailureException);
					if (e->source() != w) continue; //dont treat back angle edges

					if (m_multiAlign && (v->degree()>1))
					{
						adjEntry srcAdj = adjCor[e];
						adjEntry tgtAdj = adjCor[e]->faceCyclePred();

						//check if the nodes are expanded
						node vt1, vt2;
						if (PG.expandedNode(srcAdj->twinNode()))
							vt1 = PG.expandedNode(srcAdj->twinNode());
						else vt1 = srcAdj->twinNode();

						if (PG.expandedNode(tgtAdj->theNode()))
							vt2 = PG.expandedNode(tgtAdj->theNode());
						else vt2 = tgtAdj->theNode();

						if (vt1 == vt2)
						{

							fixedVal[w] = true;

							// we forbid bends between incident multi edges
							// or is it angle?
							setBoundsEqually(e, zeroAngleFlow, zeroBackAngleFlow);
						} else {
							//CHECK
							//to be done: only if multiedges
							if (!genshift[v]) upperBound[e] = upperAngleFlow;
							allMulti = false;
						}
					}
				}

				if (m_multiAlign && allMulti  && (v->degree()>1))
				{
					fixedVal[w] = true;

					//find an edge that allows 360 degree without bends
					bool twoNodeCC = true;
					for(adjEntry adj : w->adjEntries) {
						edge e = adj->theEdge();
						//now check for expanded nodes
						adjEntry runAdj = adjCor[e];
						node vOpp = runAdj->twinNode();
						node vStop;
						vStop = vOpp;
						runAdj = runAdj->faceCycleSucc();
						if (PG.expandedNode(vStop))
						{

							//does not end on self loops
							vStop = PG.expandedNode(vStop);
							while (PG.expandedNode(runAdj->twinNode()) == vStop)
								//we are still on vOpps cage
								runAdj = runAdj->faceCycleSucc();
						}
						//adjEntry testadj = adjCor[e]->faceCycleSucc()->twin();
						adjEntry testAdj = runAdj->twin();
						node vBack = testAdj->theNode();

						if (vBack != v) //not same node
						{
							if (PG.expandedNode(vBack))
							{
								vBack = PG.expandedNode(vBack);
							}
							if (vBack != vStop) //vstop !=0, not inner face in 2nodeCC
							{
								//CHECK: 4? upper
								OGDF_ASSERT(!PG.expandedNode(v)); //otherwise not angle flow
								if (m_traditional) {
									// dont use iteration result, set firstedge!
									upperBound[e] = maxAngleFlow;
								} else {
									setProgressiveBoundsEqually(e, lowerAngleFlow, maxBackFlow);
								}
								twoNodeCC = false;
								break;
							}
						}
					}
					//if only two nodes with multiedges are in current CC,
					//assign 360 degree to first edge
					if (twoNodeCC)
					{
						//it would be difficult to guarantee that the networkedge
						//on the other side of the face would get the 360, so alllow
						//360 for all edges or search for external face
						for(adjEntry adj : w->adjEntries) {
							edge e = adj->theEdge();
							adjEntry adje = adjCor[e];
							if (adjF[adje] == E.externalFace())
							{
								//CHECK: 4? upper
								OGDF_ASSERT(!PG.expandedNode(v)); //otherwise not angle flow
								if (m_traditional) {
									upperBound[e] = maxAngleFlow;//upperAngleFlow;
								} else {
									setProgressiveBoundsEqually(e, lowerAngleFlow, maxBackFlow);
								}
								break;
							}
						}
					}
				}
			}

		}
	}

	//int flowSum = 0;

	//To Be done: hier multiedges testen
	for(node tv : Network.nodes)
	{
		//flowSum += supply[tv];

		//only check representants of original nodes, not faces
		if ( nodeTypeArray[tv] == n_type::low || nodeTypeArray[tv] == n_type::high )
		{
			//if node representant with degree 4, set angles preliminary
			//degree four nodes with two gens are expanded in PlanRepUML
			//all others are allowed to change the edge positions
			if ( (m_traditional && (tv->degree() == 4)) ||
				((tv->degree() == 8) && !m_traditional) )
			{
				//three types: degree4 original nodes and facesplitter end nodes,
				//maybe crossings
				//fixassignment tells us that low degree nodes are not allowed to
				//have zero degree and special nodes are already assigned
				bool fixAssignment = true;

				//check if free assignment is possible for degree 4
				if (m_deg4free)
				{
					fixAssignment = false;
					for(adjEntry adj : tv->adjEntries) {
						edge te = adj->theEdge();
						if (te->source() == tv)
						{
							adjEntry pgEntry = adjCor[te];
							node pgNode = pgEntry->theNode();

							if ((PG.expandedNode(pgNode))
								//|| (PG.faceSplitter(adjCor[te]->theEdge()))
								|| (PG.typeOf(pgNode) == Graph::NodeType::dummy) //test crossings
								)
							{
								fixAssignment = true;
								break;
							}
						}
					}
				}

				//CHECK
				//now set the angles at degree 4 nodes to distribute edges
				for(adjEntry adj : tv->adjEntries) {
					edge te = adj->theEdge();

					if (te->source() == tv)
					{
						if (fixedVal[tv]) continue; //if already special values set

						if (!fixAssignment)
						{
							lowerBound[te] = 0; //lowerAngleFlow maybe 1;//0;
							upperBound[te] = upperAngleFlow;//4;
						}
						else
						{
							//only allow 90 degree arc value
							lowerBound[te] = halfPiAngleFlow;
							upperBound[te] = halfPiAngleFlow;
						}
					} else {
						if (fixedVal[tv]) continue; //if already special values set

						if (!fixAssignment)
						{
							OGDF_ASSERT(lowerAngleFlow == 0); //should only be in progressive mode
							lowerBound[te] = lowerAngleFlow;
							upperBound[te] = upperBackAngleFlow;
						}
						else
						{
							//only allow 0-180 degree back arc value
							lowerBound[te] = 0; //1
							upperBound[te] = 0; //halfPiAngleFlow; //1
						}
					}
				}
			}
			int lowsum = 0, upsum = 0;
			for(adjEntry adj : tv->adjEntries) {
				edge te = adj->theEdge();
				OGDF_ASSERT(lowerBound[te] <= upperBound[te]);
				if (noBendEdge[te]) lowerBound[te] = 0;
				lowsum += lowerBound[te];
				upsum += upperBound[te];
			}
			if (m_traditional) {
				OGDF_ASSERT(lowsum <= supply[tv]);
				OGDF_ASSERT(upsum >= supply[tv]);
			}
		}
	}
	for(node tv : Network.nodes)
	{
		for(adjEntry adj : tv->adjEntries) {
			edge te = adj->theEdge();
			if (noBendEdge[te]) lowerBound[te] = 0;
		}
	}


	bool isFlow = false;
	SList<edge> capacityBoundedEdges;
	EdgeArray<int> flow(Network,0);

	// Set upper Bound to current upper bound.
	// Some edges are no longer capacitybounded, therefore save their status
	EdgeArray<bool> isBounded(Network, false);

	for(edge e : Network.edges)
	{
		if (upperBound[e] == infinity)
		{
			capacityBoundedEdges.pushBack(e);
			isBounded[e] = true;
		}
	}

	int currentUpperBound;
	if (startBoundBendsPerEdge > 0)
		currentUpperBound = startBoundBendsPerEdge;
	else
		currentUpperBound = 4*PG.numberOfEdges();


	while ( (!isFlow) && (currentUpperBound <= 4*PG.numberOfEdges()) )
	{
		for (edge e : capacityBoundedEdges)
			upperBound[e] = currentUpperBound;

		isFlow = flowModule.call(Network,lowerBound,upperBound,cost,supply,flow);

#if 0
		if (isFlow)
		{
			for(edge e : Network.edges) {
				fout << "e = " << e << " flow = " << flow[e];
				if(nodeCor[e] == 0 && adjCor[e])
					fout << " real edge = " << adjCor[e]->theEdge();
				fout << std::endl;
			}
			for(edge e : Network.edges) {
				if(nodeCor[e] == 0 && adjCor[e] != 0 && flow[e] > 0) {
					fout << "Bends " << flow[e] << " on edge "
						<< adjCor[e]->theEdge()
						<< " between faces " << adjF[adjCor[e]]->index() << " - "
						<< adjF[adjCor[e]->twin()]->index() << std::endl;
				}
			}
			for(edge e : Network.edges) {
				if(nodeCor[e] != 0 && faceCor[e] != 0) {
					fout << "Angle " << (flow[e])*90 << "\tdegree   on node "
						<< nodeCor[e] << " at face " << faceCor[e]->index()
						<< "\tbetween edge " << adjCor[e]->faceCyclePred()
						<< "\tand " << adjCor[e] << std::endl;
				}
			}
			if (startBoundBendsPerEdge> 0) {
				fout << "Minimizing edge bends for upper bound "
					<< currentUpperBound;
				if(isFlow)
					fout << " ... Successful";
				fout << std::endl;
			}
		}
#endif

		OGDF_ASSERT(startBoundBendsPerEdge >= 1 || isFlow);

		currentUpperBound++;
	}

	if (startBoundBendsPerEdge && !isFlow)
	{
		// couldn't compute reasonable shape
		OGDF_THROW_PARAM(AlgorithmFailureException, AlgorithmFailureCode::NoFlow);
	}


	int totalNumBends = 0;


	for(edge e : Network.edges)
	{

		if (nodeCor[e] == nullptr && adjCor[e] != nullptr && (flow[e] > 0) &&
			(angleTwin[e] == nullptr) ) //no angle edges
		{
			OGDF_ASSERT(OR.bend(adjCor[e]).size() == 0);

				char zeroChar = (m_traditional ? '0' : '1');
			char oneChar = (m_traditional ? '1' : '0');
			//we depend on the property that there is no flow
			//in opposite direction due to the cost
			OR.bend(adjCor[e]).set(zeroChar,flow[e]);
			OR.bend(adjCor[e]->twin()).set(oneChar,flow[e]);

			totalNumBends += flow[e];

			//check if bends fit bounds
			if (isBounded[e])
			{
				OGDF_ASSERT((int)OR.bend(adjCor[e]).size() <= currentUpperBound);
				OGDF_ASSERT((int)OR.bend(adjCor[e]->twin()).size() <= currentUpperBound);
			}
		}
		else if (nodeCor[e] != nullptr && faceCor[e] != nullptr)
		{
			if (m_traditional) OR.angle(adjCor[e]) = (flow[e]);
			else
			{
				OGDF_ASSERT(angleTwin[e]);
				OGDF_ASSERT(flow[e] >= 0);
				OGDF_ASSERT(flow[e] <= 2);

				const int twinFlow = flow[angleTwin[e]];

				if (flow[e] == 0) {
					OGDF_ASSERT(twinFlow >= 0);
					OGDF_ASSERT(twinFlow <= 2);

					OR.angle(adjCor[e]) = 2 + twinFlow;
				} else {
					OGDF_ASSERT(twinFlow == 0);
					OR.angle(adjCor[e]) = 2 - flow[e];
				}
			}
		}
	}


#ifdef OGDF_HEAVY_DEBUG
	Logger::slout() << "\n\nTotal Number of Bends : "<< totalNumBends << std::endl << std::endl;
#endif

#ifdef OGDF_DEBUG
	string error;
	if (!OR.check(error)) {
		Logger::slout() << error << std::endl;
		OGDF_ASSERT(false);
	}
#endif

}

}
