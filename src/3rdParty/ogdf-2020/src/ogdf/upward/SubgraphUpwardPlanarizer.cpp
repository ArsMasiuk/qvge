/** \file
 * \brief Implementation of SubgraphUpwardPlanarizer class.
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/upward/SubgraphUpwardPlanarizer.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/upward/FaceSinkGraph.h>

namespace ogdf {

Module::ReturnType SubgraphUpwardPlanarizer::doCall(UpwardPlanRep &UPR,
		const EdgeArray<int>  &cost,
		const EdgeArray<bool> &forbid)
{
	const Graph &G = UPR.original();

	if(G.numberOfNodes() < 2) {
		if (G.numberOfNodes() == 1)
			UPR.newNode(G.firstNode());
		return Module::ReturnType::Optimal;
	}

	GraphCopy GC(G);

	//reverse some edges in order to obtain a DAG
	List<edge> feedBackArcSet;
	m_acyclicMod->call(GC, feedBackArcSet);
	for(edge e : feedBackArcSet) {
		GC.reverseEdge(e);
	}

	OGDF_ASSERT(isSimple(G));

	//mapping cost
	EdgeArray<int> cost_GC(GC);
	for(edge e : GC.edges) {
		if (forbid[GC.original(e)])
			cost_GC[e] = std::numeric_limits<int>::max();
		else
			cost_GC[e] = cost[GC.original(e)];
	}

	// tranform to single source graph by adding a super source s_hat and connect it with the other sources
	EdgeArray<bool> sourceArcs(GC, false);
	node s_hat = GC.newNode();
	for(node v : GC.nodes) {
		if (v->indeg() == 0 && v != s_hat) {
			edge e_tmp = GC.newEdge(s_hat, v);
			cost_GC[e_tmp] = 0; // crossings source arcs cause not cost
			sourceArcs[e_tmp] = true;
		}
	}


#if 0
	GraphAttributes AG_GC(GC, GraphAttributes::nodeGraphics |
	                          GraphAttributes::edgeGraphics |
	                          GraphAttributes::nodeStyle |
	                          GraphAttributes::edgeStyle |
	                          GraphAttributes::nodeLabel |
	                          GraphAttributes::edgeLabel);
	AG_GC.setAllHeight(30.0);
	AG_GC.setAllWidth(30.0);
	for(node z : AG_GC.constGraph().nodes) {
		AG_GC.label(z) = to_string(z->index());
	}
	AG_GC.writeGML("c:/temp/GC.gml");
#endif

	BCTree BC(GC);
	const Graph &bcTree = BC.bcTree();

	GraphCopy G_dummy;
	G_dummy.createEmpty(G);
	NodeArray<GraphCopy> biComps(bcTree, G_dummy); // bicomps of G; init with an empty graph
	UpwardPlanRep UPR_dummy;
	UPR_dummy.createEmpty(G);
	NodeArray<UpwardPlanRep> uprs(bcTree, UPR_dummy); // the upward planarized representation of the bicomps; init with an empty UpwarPlanRep

	constructComponentGraphs(BC, biComps);

	for(node v : bcTree.nodes) {

		if (BC.typeOfBNode(v) == BCTree::BNodeType::CComp)
			continue;

		GraphCopy &block = biComps[v];

		OGDF_ASSERT(m_subgraph);

		// construct a super source for this block
		node s, s_block;
		hasSingleSource(block, s);
		s_block = block.newNode();
		block.newEdge(s_block, s); //connect s

		UpwardPlanRep bestUPR;

		//upward planarize if not upward planar
		if (!UpwardPlanarity::upwardPlanarEmbed_singleSource(block)) {

			for (int i = 0; i < m_runs; i++) {// i multistarts
				UpwardPlanRep UPR_tmp;
				UPR_tmp.createEmpty(block);
				List<edge> delEdges;

				m_subgraph->call(UPR_tmp, delEdges);

				OGDF_ASSERT( isSimple(UPR_tmp) );
				UPR_tmp.augment();

				//mark the source arcs of block
				UPR_tmp.m_isSourceArc[UPR_tmp.copy(s_block->firstAdj()->theEdge())] = true;
				for (adjEntry adj_tmp : UPR_tmp.copy(s_block->firstAdj()->theEdge()->target())->adjEntries) {
					edge e_tmp = UPR_tmp.original(adj_tmp->theEdge());
					if (e_tmp != nullptr && block.original(e_tmp) != nullptr && sourceArcs[block.original(e_tmp)])
						UPR_tmp.m_isSourceArc[adj_tmp->theEdge()] = true;
				}

				//assign "crossing cost"
				EdgeArray<int> cost_Block(block);
				for (edge e : block.edges) {
					if (block.original(e) == nullptr || GC.original(block.original(e)) == nullptr)
						cost_Block[e] = 0;
					else
						cost_Block[e] = cost_GC[block.original(e)];
				}

#if 0
				LayerBasedUPRLayout uprLayout;
				UpwardPlanRep upr_bug(UPR_tmp.getEmbedding());
				adjEntry adj_bug = upr_bug.getAdjEntry(upr_bug.getEmbedding(), upr_bug.getSuperSource(), upr_bug.getEmbedding().externalFace());
				node s_upr_bug = upr_bug.newNode();
				upr_bug.getEmbedding().splitFace(s_upr_bug, adj_bug);
				upr_bug.m_isSourceArc.init(upr_bug, false);
				upr_bug.m_isSourceArc[s_upr_bug->firstAdj()->theEdge()] = true;
				upr_bug.s_hat = s_upr_bug;
				upr_bug.augment();

				GraphAttributes GA_UPR_tmp(UPR_tmp, GraphAttributes::nodeGraphics |
						GraphAttributes::edgeGraphics |
						GraphAttributes::nodeStyle |
						GraphAttributes::edgeStyle |
						GraphAttributes::nodeLabel |
						GraphAttributes::edgeLabel
						);
				GA_UPR_tmp.setAllHeight(30.0);
				GA_UPR_tmp.setAllWidth(30.0);

				uprLayout.call(upr_bug, GA_UPR_tmp);

				// label the nodes with their index
				for(node z : GA_UPR_tmp.constGraph().nodes) {
					GA_UPR_tmp.label(z) = to_string(z->index());
					GA_UPR_tmp.y(z)=-GA_UPR_tmp.y(z);
					GA_UPR_tmp.x(z)=-GA_UPR_tmp.x(z);
				}
				for(edge eee : GA_UPR_tmp.constGraph().edges) {
					DPolyline &line = GA_UPR_tmp.bends(eee);
					ListIterator<DPoint> it;
					for(it = line.begin(); it.valid(); it++) {
						(*it).m_y = -(*it).m_y;
						(*it).m_x = -(*it).m_x;
					}
				}
				GA_UPR_tmp.writeGML("c:/temp/UPR_tmp_fups.gml");
				std::cout << "UPR_tmp/fups faces:";
				UPR_tmp.outputFaces(UPR_tmp.getEmbedding());
#endif

				delEdges.permute();
				m_inserter->call(UPR_tmp, cost_Block, delEdges);

				if (i != 0) {
					if (UPR_tmp.numberOfCrossings() < bestUPR.numberOfCrossings()) {
#if 0
						std::cout << std::endl << "new cr_nr:" << UPR_tmp.numberOfCrossings() << " old  cr_nr : " << bestUPR.numberOfCrossings() << std::endl;
#endif
						bestUPR = UPR_tmp;
					}
				}
				else
					bestUPR = UPR_tmp;
			}
		} else { // block is upward planar
			CombinatorialEmbedding Gamma(block);
			FaceSinkGraph fsg((const CombinatorialEmbedding &) Gamma, s_block);
			SList<face> faceList;
			fsg.possibleExternalFaces(faceList);
			Gamma.setExternalFace(faceList.front());

			UpwardPlanRep UPR_tmp(Gamma);
			UPR_tmp.augment();

			//mark the source arcs of  block
			UPR_tmp.m_isSourceArc[UPR_tmp.copy(s->firstAdj()->theEdge())] = true;
			for (adjEntry adj_tmp : UPR_tmp.copy(s->firstAdj()->theEdge()->target())->adjEntries) {
				edge e_tmp = UPR_tmp.original(adj_tmp->theEdge());
				if (e_tmp != nullptr && block.original(e_tmp) != nullptr && sourceArcs[block.original(e_tmp)])
					UPR_tmp.m_isSourceArc[adj_tmp->theEdge()] = true;
			}

			bestUPR = UPR_tmp;

#if 0
			GraphAttributes GA_UPR_tmp(UPR_tmp, GraphAttributes::nodeGraphics |
					GraphAttributes::edgeGraphics |
					GraphAttributes::nodeStyle |
					GraphAttributes::edgeStyle |
					GraphAttributes::nodeLabel |
					GraphAttributes::edgeLabel
					);
			GA_UPR_tmp.setAllHeight(30.0);
			GA_UPR_tmp.setAllWidth(30.0);

			// label the nodes with their index
			for(node z : GA_UPR_tmp.constGraph().nodes) {
				GA_UPR_tmp.label(z) = to_string(z->index());
				GA_UPR_tmp.y(z)=-GA_UPR_tmp.y(z);
				GA_UPR_tmp.x(z)=-GA_UPR_tmp.x(z);
			}
			for(edge eee : GA_UPR_tmp.constGraph().edges) {
				DPolyline &line = GA_UPR_tmp.bends(eee);
				ListIterator<DPoint> it;
				for(it = line.begin(); it.valid(); it++) {
					(*it).m_y = -(*it).m_y;
					(*it).m_x = -(*it).m_x;
				}
			}
			GA_UPR_tmp.writeGML("c:/temp/UPR_tmp_fups.gml");
			std::cout << "UPR_tmp/fups faces:";
			UPR_tmp.outputFaces(UPR_tmp.getEmbedding());
#endif
		}
		uprs[v] = bestUPR;
	}

	// compute the number of crossings
	int nr_cr = 0;
	for(node v : bcTree.nodes) {
		if (BC.typeOfBNode(v) != BCTree::BNodeType::CComp)
			nr_cr = nr_cr + uprs[v].numberOfCrossings();
	}

	//merge all component to a graph
	node parent_BC = BC.bcproper(s_hat);
	NodeArray<bool> nodesDone(bcTree, false);
	dfsMerge(GC, BC, biComps, uprs, UPR, nullptr, parent_BC, nodesDone); // start with the component which contains the super source s_hat

	//augment to single sink graph
	UPR.augment();

	//set crossings
	UPR.crossings = nr_cr;

#if 0
	LayerBasedUPRLayout uprLayout;
	UpwardPlanRep upr_bug(UPR.getEmbedding());
	adjEntry adj_bug = upr_bug.getAdjEntry(upr_bug.getEmbedding(), upr_bug.getSuperSource(), upr_bug.getEmbedding().externalFace());
	node s_upr_bug = upr_bug.newNode();
	upr_bug.getEmbedding().splitFace(s_upr_bug, adj_bug);
	upr_bug.m_isSourceArc.init(upr_bug, false);
	upr_bug.m_isSourceArc[s_upr_bug->firstAdj()->theEdge()] = true;
	upr_bug.s_hat = s_upr_bug;
	upr_bug.augment();
	GraphAttributes AG(UPR, GraphAttributes::nodeGraphics |
	                        GraphAttributes::edgeGraphics |
	                        GraphAttributes::nodeStyle |
	                        GraphAttributes::edgeStyle |
	                        GraphAttributes::nodeLabel |
	                        GraphAttributes::edgeLabel);
	AG.setAllHeight(30.0);
	AG.setAllWidth(30.0);

	uprLayout.call(upr_bug, AG);

	for(node v : AG.constGraph().nodes) {
		int idx;
		idx = v->index();

		if (UPR.original(v) != 0)
			idx = UPR.original(v)->index();

		AG.label(v) = to_string(idx);
		if (UPR.isDummy(v))
			AG.fillColor(v) = "#ff0000";
		AG.y(v)=-AG.y(v);
	}
	// label the edges with their index
	for(edge e : AG.constGraph().edges) {
		AG.label(e) = to_string(e->index());
		if (UPR.isSourceArc(e))
			AG.strokeColor(e) = "#00ff00";
		if (UPR.isSinkArc(e))
			AG.strokeColor(e) = "#ff0000";

		DPolyline &line = AG.bends(e);
		ListIterator<DPoint> it;
		for(it = line.begin(); it.valid(); it++) {
			(*it).m_y = -(*it).m_y;
		}
	}
	AG.writeGML("c:/temp/upr_res.gml");
#if 0
	std::cout << "UPR_RES";
	UPR.outputFaces(UPR.getEmbedding());
	std::cout << "Mapping :" << std::endl;
	for(node v : UPR.nodes) {
		if (UPR.original(v) != 0) {
			std::cout << "node UPR  " << v << "   node G  " << UPR.original(v) << std::endl;
		}
	}
#endif
#endif

	OGDF_ASSERT(hasSingleSource(UPR));
	OGDF_ASSERT(isSimple(UPR));
	OGDF_ASSERT(isAcyclic(UPR));
	OGDF_ASSERT(UpwardPlanarity::isUpwardPlanar_singleSource(UPR));

#if 0
	for(edge eee : UPR.original().edges) {
		if (UPR.isReversed(eee))
			std::cout << std::endl << eee << std::endl;
	}
#endif
	return Module::ReturnType::Feasible;
}



void SubgraphUpwardPlanarizer::dfsMerge(
	const GraphCopy &GC,
	BCTree &BC,
	NodeArray<GraphCopy> &biComps,
	NodeArray<UpwardPlanRep> &uprs,
	UpwardPlanRep &UPR_res,
	node parent_BC,
	node current_BC,
	NodeArray<bool> &nodesDone)
{
	// only one component.
	if (current_BC->degree() == 0) {
		merge(GC, UPR_res, biComps[current_BC], uprs[current_BC]);
		return;
	}

	for(adjEntry adj : current_BC->adjEntries) {
		node next_BC = adj->twin()->theNode();
		if (BC.typeOfBNode(current_BC) == BCTree::BNodeType::CComp) {
			if (parent_BC != nullptr && !nodesDone[parent_BC]) {
				merge(GC, UPR_res, biComps[parent_BC], uprs[parent_BC]);
				nodesDone[parent_BC] = true;
			}
			if (!nodesDone[next_BC]) {
				merge(GC, UPR_res, biComps[next_BC], uprs[next_BC]);
				nodesDone[next_BC] = true;
			}
		}
		if (next_BC != parent_BC )
			dfsMerge(GC, BC, biComps, uprs, UPR_res, current_BC, next_BC, nodesDone);
	}
}



void SubgraphUpwardPlanarizer::merge(
	const GraphCopy &GC,
	UpwardPlanRep &UPR_res,
	const GraphCopy &block,
	UpwardPlanRep &UPR)
{
	node startUPR = UPR.getSuperSource()->firstAdj()->theEdge()->target();
	node startRes;

	node startG = GC.original(block.original(UPR.original(startUPR)));

	bool empty = UPR_res.empty();

	if (empty) {

		OGDF_ASSERT(startG == nullptr);

		// contruct a node in UPR_res assocciated with startUPR
		startRes = UPR_res.newNode();
		UPR_res.m_isSinkArc.init(UPR_res, false);
		UPR_res.m_isSourceArc.init(UPR_res, false);
		UPR_res.s_hat = startRes;
	}
	else {
		startRes = UPR_res.copy(startG);
	}

	OGDF_ASSERT(startRes != nullptr);

	// compute the adjEntry position (in UPR_res) of the cutvertex startRes
	adjEntry pos = nullptr;
	if (!empty) {
		adjEntry adj_ext = nullptr, adj_int = nullptr;
		for(adjEntry run : startRes->adjEntries) {
			if (UPR_res.getEmbedding().rightFace(run) == UPR_res.getEmbedding().externalFace()) {
				adj_ext = run;
				break;
			}
			if (run->theEdge()->source() == startRes)
				adj_int = run;
		}
		// cutvertex is a sink in UPR_res
		if (adj_ext == nullptr && adj_int == nullptr) {
			pos = UPR_res.sinkSwitchOf(startRes);
		}
		else {
			if (adj_ext == nullptr)
				pos = adj_int;
			else
				pos = adj_ext;
		}
		OGDF_ASSERT(pos != nullptr);
	}

	// construct for each node (except the two super sink and the super source) of UPR a associated of UPR to UPR_res
	NodeArray<node> nodeUPR2UPR_res(UPR, nullptr);
	nodeUPR2UPR_res[startUPR] = startRes;
	for(node v : UPR.nodes) {

		// already constructed or is super sink or super source
		if (v == startUPR || v == UPR.getSuperSink() || v == UPR.getSuperSink()->firstAdj()->theEdge()->source() || v == UPR.getSuperSource())
			continue;

		node vNew;
		if (UPR.original(v) != nullptr ) {
			node vG = GC.original(block.original((UPR.original(v))));
			if (vG != nullptr)
				vNew = UPR_res.newNode(vG);
			else
				vNew = UPR_res.newNode(); //vG is the super source
		}
		else // crossing dummy, no original node
			vNew = UPR_res.newNode();
		nodeUPR2UPR_res[v] = vNew;
	}

	//add edges of UPR to UPR_res
	EdgeArray<edge> edgeUPR2UPR_res(UPR, nullptr);
	for(edge e : block.edges) {

		if (e->source()->indeg()==0) // the artificial edge with the super source
			continue;

		List<edge> chains = UPR.chain(e);
		edge eG = nullptr, eGC = block.original(e);
		eG = GC.original(eGC);

		OGDF_ASSERT(!chains.empty());

		//construct new edges in UPR_res
		for(edge eChain : chains) {
			node tgt = nodeUPR2UPR_res[eChain->target()];
			node src = nodeUPR2UPR_res[eChain->source()];
			edge eNew = UPR_res.newEdge(src, tgt);
			edgeUPR2UPR_res[eChain] = eNew;

			if (UPR.isSinkArc(UPR.copy(e)))
				UPR_res.m_isSinkArc[eNew] = true;
			if (UPR.isSourceArc(UPR.copy(e)))
				UPR_res.m_isSourceArc[eNew] = true;

			if (eG == nullptr) { // edge is associated with a sink arc
				UPR_res.m_eOrig[eNew] = nullptr;
				continue;
			}

			UPR_res.m_eOrig[eNew] = eG;
			if (chains.size() == 1) { // e is not split
				UPR_res.m_eCopy[eG].pushBack(eNew);
				UPR_res.m_eIterator[eNew] = UPR_res.m_eCopy[eG].begin();
				break;
			}
			UPR_res.m_eCopy[eG].pushBack(eNew);
			UPR_res.m_eIterator[eNew] = UPR_res.m_eCopy[eG].rbegin();
		}
	}

	// embed the new component in UPR_res with respect to the embedding of UPR
	// for the cut vertex
	if (!empty) {
		adjEntry run = UPR.getAdjEntry(UPR.getEmbedding(), startUPR, UPR.getEmbedding().externalFace());
		run = run->cyclicSucc();
		adjEntry adjStart = run;
		do {
			if (edgeUPR2UPR_res[run->theEdge()] != nullptr) {
				adjEntry adj_UPR_res = edgeUPR2UPR_res[run->theEdge()]->adjSource();
				UPR_res.moveAdjAfter(adj_UPR_res, pos);
				pos = adj_UPR_res;
			}
			run = run->cyclicSucc();
		} while(run != adjStart);
	}

	for(node v : UPR.nodes) {
		if (v == startUPR && !empty)
			continue;

		node v_UPR_res = nodeUPR2UPR_res[v];
		List<adjEntry> adj_UPR, adj_UPR_res;
		v->allAdjEntries(adj_UPR);

		// convert adj_UPR of v to adj_UPR_res of v_UPR_res
		for(adjEntry adj : adj_UPR) {
			edge e_res = edgeUPR2UPR_res[adj->theEdge()];
			if (e_res == nullptr) // associated edges in UPR_res
				continue;
			adjEntry adj_res = e_res->adjSource();
			if (adj_res->theNode() != v_UPR_res)
				adj_res = adj_res->twin();
			adj_UPR_res.pushBack(adj_res);
		}

		if(v_UPR_res) // old sort code seems to not have done anything if first argument was nullptr. -- so this fixes up the old (weird) use
			UPR_res.sort(v_UPR_res, adj_UPR_res);
	}

#if 0
	if (!UPR_res.empty()) {
		GraphAttributes GA_UPR_res(UPR_res, GraphAttributes::nodeGraphics |
				GraphAttributes::edgeGraphics |
				GraphAttributes::nodeStyle |
				GraphAttributes::edgeStyle |
				GraphAttributes::nodeLabel |
				GraphAttributes::edgeLabel
				);
		GA_UPR_res.setAllHeight(30.0);
		GA_UPR_res.setAllWidth(30.0);
		// label the nodes with their index
		for(node z : GA_UPR_res.constGraph().nodes) {
			GA_UPR_res.label(z) = to_string(z->index());
		}
		GA_UPR_res.writeGML("c:/temp/UPR_res_tmp.gml");
		std::cout << "UPR_res_tmp faces:";
		UPR_res.outputFaces(UPR_res.getEmbedding());
	}

	GraphAttributes GA_UPR(UPR, GraphAttributes::nodeGraphics |
				GraphAttributes::edgeGraphics |
				GraphAttributes::nodeStyle |
				GraphAttributes::edgeStyle |
				GraphAttributes::nodeLabel |
				GraphAttributes::edgeLabel
				);
	GA_UPR.setAllHeight(30.0);
	GA_UPR.setAllWidth(30.0);
	// label the nodes with their index
	for(node z : GA_UPR.constGraph().nodes) {
		GA_UPR.label(z) = to_string(z->index());
	}
	GA_UPR.writeGML("c:/temp/UPR_tmp.gml");
	std::cout << "UPR_tmp faces:";
	UPR.outputFaces(UPR.getEmbedding());
#endif

	// update UPR_res
	UPR_res.initMe();
}


void SubgraphUpwardPlanarizer::constructComponentGraphs(BCTree &BC, NodeArray<GraphCopy> &biComps)
{
	NodeArray<int> constructed(BC.originalGraph(), -1);
	const Graph &bcTree = BC.bcTree();
	int i = 0; // comp. number
	for(node v : bcTree.nodes) {

		if (BC.typeOfBNode(v) == BCTree::BNodeType::CComp)
			continue;

		const SList<edge> &edges_comp = BC.hEdges(v); //bicomp edges
		List<edge> edges_orig;
		for(edge e : edges_comp)
			edges_orig.pushBack(BC.original(e));

		GraphCopy GC;
		GC.createEmpty(BC.originalGraph());
		// construct i-th component graph
		for(edge eOrig : edges_orig) {
			node srcOrig = eOrig->source();
			node tgtOrig = eOrig->target();
			if (constructed[srcOrig] != i) {
				constructed[srcOrig] = i;
				GC.newNode(srcOrig);
			}
			if (constructed[tgtOrig] != i) {
				constructed[tgtOrig] = i;
				GC.newNode(tgtOrig);
			}
			GC.newEdge(eOrig);
		}
		biComps[v] = GC;
		i++;
	}
}

}
