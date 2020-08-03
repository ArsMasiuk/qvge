/** \file
 * \brief Implementation of FUPSSimple class.
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

#include <ogdf/upward/FUPSSimple.h>
#include <ogdf/upward/UpwardPlanarity.h>
#include <ogdf/upward/FaceSinkGraph.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {

Module::ReturnType FUPSSimple::doCall(
	UpwardPlanRep &UPR,
	List<edge> &delEdges)
{

	delEdges.clear();
	computeFUPS(UPR, delEdges);
	for (int i = 1; i < m_nRuns; ++i) {
		UpwardPlanRep UPR_cur = UPR;
		List<edge> delEdges_cur;
		computeFUPS(UPR_cur, delEdges_cur);

		// use new result??
		if (delEdges_cur.size() < delEdges.size()) {
			UPR = UPR_cur;
			delEdges = delEdges_cur;
		}
	}
	return Module::ReturnType::Feasible;
}



void FUPSSimple::computeFUPS(UpwardPlanRep &UPR, List<edge> &delEdges)
{
	const Graph &G = UPR.original();
	GraphCopy FUPS(G);
	node s_orig;
	hasSingleSource(G, s_orig);
	List<edge> nonTreeEdges_orig;
	bool random = (m_nRuns != 0);

	getSpanTree(FUPS, nonTreeEdges_orig, random);

	CombinatorialEmbedding Gamma(FUPS);

	if (random)
		nonTreeEdges_orig.permute(); // random order

	adjEntry extFaceHandle = nullptr;

	//insert nonTreeEdges
	while (!nonTreeEdges_orig.empty()) {

#if 0
		GraphAttributes AG(FUPS, GraphAttributes::nodeGraphics |
		                         GraphAttributes::edgeGraphics |
		                         GraphAttributes::nodeStyle |
		                         GraphAttributes::edgeStyle |
		                         GraphAttributes::nodeLabel |
		                         GraphAttributes::edgeLabel);
		// label the nodes with their index
		for(node v : AG.constGraph().nodes) {
			AG.label(v) = to_string(v->index());
		}
		AG.writeGML("c:/temp/spannTree.gml");
#endif

		// make identical copy FUPSCopy of FUPS
		//and insert e_orig in FUPSCopy
		GraphCopy FUPSCopy((const GraphCopy &) FUPS);
		edge e_orig = nonTreeEdges_orig.popFrontRet();
		FUPSCopy.newEdge(e_orig);

		if (UpwardPlanarity::upwardPlanarEmbed_singleSource(FUPSCopy)) { //upward embedded the fups and check feasibility
			CombinatorialEmbedding Beta(FUPSCopy);

			//choose a arbitrary feasibel ext. face
			FaceSinkGraph fsg(Beta, FUPSCopy.copy(s_orig));
			SList<face> ext_faces;
			fsg.possibleExternalFaces(ext_faces);

			OGDF_ASSERT(!ext_faces.empty());

			Beta.setExternalFace(ext_faces.front());


#if 0
			std::cout << std::endl << "FUPS : " << std::endl;
			for(face ff : Beta.faces) {
				std::cout << "face " << ff->index() << ": ";
				adjEntry adjNext = ff->firstAdj();
				do {
					std::cout << adjNext->theEdge() << "; ";
					adjNext = adjNext->faceCycleSucc();
				} while(adjNext != ff->firstAdj());
				std::cout << std::endl;
			}
			if (Beta.externalFace() != 0)
				std::cout << "ext. face of the graph is: " << Beta.externalFace()->index() << std::endl;
			else
				std::cout << "no ext. face set." << std::endl;
#endif

			GraphCopy M((const GraphCopy &) FUPSCopy); // use a identical copy of FUPSCopy to construct the merge graph of FUPSCopy
			adjEntry extFaceHandle_cur = getAdjEntry(Beta, FUPSCopy.copy(s_orig), Beta.externalFace());
			adjEntry adj_orig = FUPSCopy.original(extFaceHandle_cur->theEdge())->adjSource();

			List<edge> missingEdges = nonTreeEdges_orig, listTmp = delEdges;
			missingEdges.conc(listTmp);
			if (constructMergeGraph(M, adj_orig, missingEdges)) {
				FUPS = FUPSCopy;
				extFaceHandle = FUPS.copy(FUPSCopy.original(extFaceHandle_cur->theEdge()))->adjSource();
				continue;
			}
			else {
				//Beta is not feasible
				delEdges.pushBack(e_orig);
			}
		}
		else {
			// not ok, GC is not feasible
			delEdges.pushBack(e_orig);
		}
	}
	UpwardPlanRep fups_tmp (FUPS, extFaceHandle);
	UPR = fups_tmp;
}


void FUPSSimple::getSpanTree(GraphCopy &GC, List<edge> &delEdges, bool random)
{
	if (GC.numberOfNodes() == 1)
		return; // nothing to do

	node s;
	hasSingleSource(GC, s);
	NodeArray<bool> visited(GC, false);
	EdgeArray<bool> isTreeEdge(GC,false);
	List<node> toDo;

	//mark the incident edges e1..e_i of super source s and the incident edges of the target node of the edge e1.._e_i as tree edge.
	visited[s] = true;
	for(adjEntry adj : s->adjEntries) {
		isTreeEdge[adj] = true;
		visited[adj->theEdge()->target()];
		for(adjEntry adjTmp : adj->theEdge()->target()->adjEntries) {
			isTreeEdge[adjTmp] = true;
			node tgt = adjTmp->theEdge()->target();
			if (!visited[tgt]) {
				toDo.pushBack(tgt);
				visited[tgt] = true;
			}
		}
	}

	//traversing with dfs
	for(node start : toDo) {
		for(adjEntry adj : start->adjEntries) {
			node v = adj->theEdge()->target();
			if (!visited[v])
				dfs_visit(GC, adj->theEdge(), visited, isTreeEdge, random);
		}
	}

	// delete all non tree edgesEdges to obtain a span tree
	List<edge> list;
	for(edge e : GC.edges) {
		if (!isTreeEdge[e])
			list.pushBack(e);
	}
	while (!list.empty()) {
		edge e = list.popFrontRet();
		delEdges.pushBack(GC.original(e));
		GC.delEdge(e);
	}
}



void FUPSSimple::dfs_visit(
	const Graph &G,
	edge e,
	NodeArray<bool> &visited,
	EdgeArray<bool> &treeEdges,
	bool random)
{
	treeEdges[e] = true;
	List<edge> elist;
	e->target()->outEdges(elist);
	if (!elist.empty()) {
		if (random)
			elist.permute();
		ListIterator<edge> it;
		for (it = elist.begin(); it.valid(); ++it) {
			edge ee = *it;
			if (!visited[ee->target()])
				dfs_visit(G, ee, visited, treeEdges, random);
		}
	}
	visited[e->target()] = true;
}


bool FUPSSimple::constructMergeGraph(GraphCopy &M, adjEntry adj_orig, const List<edge> &orig_edges)
{
	CombinatorialEmbedding Beta(M);

	//set ext. face of Beta
	adjEntry ext_adj = M.copy(adj_orig->theEdge())->adjSource();
	Beta.setExternalFace(Beta.rightFace(ext_adj));

#if 0
	std::cout << std::endl << "FUPS : " << std::endl;
	for(face ff : Beta.faces) {
		std::cout << "face " << ff->index() << ": ";
		adjEntry adjNext = ff->firstAdj();
		do {
			std::cout << adjNext->theEdge() << "; ";
			adjNext = adjNext->faceCycleSucc();
		} while(adjNext != ff->firstAdj());
		std::cout << std::endl;
	}
	if (Beta.externalFace() != 0)
		std::cout << "ext. face of the graph is: " << Beta.externalFace()->index() << std::endl;
	else
		std::cout << "no ext. face set." << std::endl;
#endif

	FaceSinkGraph fsg(Beta, M.copy(adj_orig->theNode()));
	SList<node> aug_nodes;
	SList<edge> aug_edges;
	SList<face> fList;
	fsg.possibleExternalFaces(fList); // use this method to call the methode checkForest()
	node v_ext = fsg.faceNodeOf(Beta.externalFace());

	OGDF_ASSERT(v_ext != nullptr);

	fsg.stAugmentation(v_ext, M, aug_nodes, aug_edges);

#if 0
	GraphAttributes AG(M, GraphAttributes::nodeGraphics|
						GraphAttributes::edgeGraphics |
						GraphAttributes::nodeStyle |
						GraphAttributes::edgeStyle |
						GraphAttributes::nodeLabel |
						GraphAttributes::edgeLabel
						);
	// label the nodes with their index
	for(node v : AG.constGraph().nodes) {
		AG.label(v) = to_string(v->index());
	}
	AG.writeGML("c:/temp/MergeFUPS.gml");
#endif

	OGDF_ASSERT(isStGraph(M));

	//add the deleted edges
	for(edge eOrig : orig_edges) {
		node a = M.copy(eOrig->source());
		node b = M.copy(eOrig->target());
		M.newEdge(a, b);
	}
	return isAcyclic(M);
}

}
