/** \file
 * \brief planar biconnected augmentation algorithm with fixed
 * 		  embedding
 *
 * \author Bernd Zey
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


#include <ogdf/augmentation/PlanarAugmentationFix.h>

// for debug-outputs
//#define PLANAR_AUGMENTATION_FIX_DEBUG
// for additional planarity checks after each "round"
//#define PLANAR_AUGMENTATION_FIX_DEBUG_PLANARCHECK

#if defined(PLANAR_AUGMENTATION_FIX_DEBUG) || defined(PLANAR_AUGMENTATION_FIX_DEBUG_PLANARCHECK)
 #include <ogdf/basic/extended_graph_alg.h>
 #include <ogdf/basic/simple_graph_alg.h>
#endif

namespace ogdf {

void PlanarAugmentationFix::doCall(Graph& g, List<edge>& list)
{
	list.clear();
	m_pResult = &list;

	m_pGraph = &g;
	m_pEmbedding = new CombinatorialEmbedding(*m_pGraph);

	NodeArray<bool> activeNodes(*m_pGraph, false);
	List<node> activeNodesList;

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << "original graph:" << std::endl;
	for(node n : m_pGraph->nodes) {
		std::cout << n->index() << ": ";
		for(adjEntry nAdj : n->adjEntries)
			std::cout << nAdj << " ";
		std::cout << std::endl;
	}
#endif

	List<face> faces;

	for(face actFace : m_pEmbedding->faces) {
		faces.pushBack(actFace);
	}

	m_eCopy.init(*m_pGraph, nullptr);
	m_graphCopy.createEmpty(*m_pGraph);

	while (faces.size() > 0){
		face actFace = faces.popFrontRet();

		adjEntry adjOuterFace = nullptr;

		adjEntry adjFirst = actFace->firstAdj();
		if (m_pEmbedding->leftFace(adjFirst) != actFace)
			adjFirst = adjFirst->twin();

		adjEntry adjFace = adjFirst;

		if (m_pEmbedding->numberOfFaces() == 1){
			// we just have only one face (this is the outer face)
			adjOuterFace = adjFace;
		}

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		std::cout
		  << "======================" << std::endl
		  << "face " << actFace->index() <<":" << std::endl
		  << adjFace->theNode()->index() << "(" << adjFace << ")"<< ", ";
#endif

		activeNodesList.pushBack(adjFace->theNode());
		activeNodes[adjFace->theNode()] = true;
		adjFace = adjFace->twin()->cyclicSucc();

		bool augmentationRequired = false;

		while (adjFace != adjFirst){
			if ((adjOuterFace == nullptr) && (m_pEmbedding->leftFace(adjFace) != m_pEmbedding->rightFace(adjFace)))
				adjOuterFace = adjFace;

			if (!activeNodes[adjFace->theNode()]) {
				activeNodesList.pushBack(adjFace->theNode());
				activeNodes[adjFace->theNode()] = true;

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				std::cout << adjFace->theNode()->index() << "(" << adjFace << ")"<< ", ";
#endif
			} else {
				augmentationRequired = true;

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				std::cout << adjFace->theNode()->index() << "(" << adjFace << ")(nc), ";
#endif
			}

			adjFace = adjFace->twin()->cyclicSucc();
		}

		if (augmentationRequired){
			m_graphCopy.createEmpty(*m_pGraph);
			m_graphCopy.initByActiveNodes(activeNodesList, activeNodes, m_eCopy);
			m_graphCopy.setOriginalEmbedding();

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			std::cout << " graphCopy:" << std::endl;
			for(node n : m_graphCopy.nodes) {
				std::cout << n->index() << "(" << (m_graphCopy.original(n))->index() << "):" << std::flush;
				for(adjEntry nAdj : n->adjEntries) {
					std::cout << nAdj << "(" << nAdj->theEdge() << "[" << nAdj->index() << "]) " << std::flush;
				}
				std::cout << std::endl;
			}
#endif

			adjEntry adjOuterFaceCopy = (m_graphCopy.copy(adjOuterFace->theEdge()))->adjSource();
			if (adjOuterFaceCopy->theNode() != m_graphCopy.copy(adjOuterFace->theNode()))
				adjOuterFaceCopy = adjOuterFaceCopy->twin();

			augment(adjOuterFaceCopy);
		} else {
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			std::cout << std::endl;
#endif
		}


		// set all entries in activeNodes to default value false
		//  for next iteration
		for (node v : activeNodesList) {
			activeNodes[v] = false;
			for(adjEntry adj : v->adjEntries) {
				m_eCopy[adj->theEdge()] = nullptr;
			}
		}
		activeNodesList.clear();

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		std::cout << "======================" << std::endl;
#endif

	}

	delete m_pEmbedding;

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	if (isBiconnected(*m_pGraph))
		std::cout << " graph is biconnected" << std::endl;
	else
		std::cout << " graph is NOT biconnected!!!" << std::endl;

	if (isPlanar(*m_pGraph))
		std::cout << " graph is planar" << std::endl;
	else
		std::cout << " graph is NOT planar!!!" << std::endl;
#endif
}



// the main augmentation function
void PlanarAugmentationFix::augment(adjEntry adjOuterFace)
{
	CombinatorialEmbedding* actComb = new CombinatorialEmbedding(m_graphCopy);
	m_pActEmbedding = actComb;

	DynamicBCTree* actBCTree = new DynamicBCTree(m_graphCopy);
	m_pBCTree = actBCTree;

	m_pActEmbedding->setExternalFace(m_pActEmbedding->rightFace(adjOuterFace));

	node bFaceNode = m_pBCTree->bcproper(adjOuterFace->theEdge());

	m_isLabel.init(m_pBCTree->bcTree(), nullptr);
	m_belongsTo.init(m_pBCTree->bcTree(), nullptr);
	m_belongsToIt.init(m_pBCTree->bcTree(), nullptr);

	List<node> pendants;

	node root = nullptr;

	// init pendants
	for (node v : m_pBCTree->bcTree().nodes){
		if (m_pBCTree->parent(v) == nullptr){
			root = v;
		}
		if (v->degree() == 1 && v != bFaceNode) { // pendant
			pendants.pushBack(v);
		}
	}
	OGDF_ASSERT(root);

	if (root != bFaceNode){
		// change root of the bc-tree to the b-node that includes the actual face
		modifyBCRoot(root, bFaceNode);
	}

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << " augment(): pendants:" << std::flush;
	for(node v : m_pBCTree->bcTree().nodes) {
		if (m_pBCTree->parent(v) == 0){
			std::cout << "[root: " << v->index() << std::flush;
			if (m_pBCTree->typeOfBNode(v) == BCTree::BNodeType::BComp)
				std::cout << "(b)], " << std::flush;
			else
				std::cout << "(c)], " << std::flush;
		}
		if (v->degree() == 1){
			// pendant
			if (v != bFaceNode){
				std::cout << v->index() << ", " << std::flush;
			}
			else
				std::cout << v->index() << "(is root), " << std::flush;
		}
	}
	std::cout << std::endl;
#endif

	m_actBCRoot = bFaceNode;
	m_labels.clear();

	// call reduceChain for all pendants
	for (node v : pendants) {
		reduceChain(v);
	}

	// main augmentation loop
	while (m_labels.size() > 0){

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		std::cout
		  << std::endl
		  << " *-----------------------------------" << std::endl
		  << " * augment(): #labels  : " << m_labels.size() << std::endl;

		int lNr = 1;
		for (pa_label label : m_labels) {
			std::cout << " * label in list with position " << lNr << " : " << std::flush;
			for(node v : label->m_pendants) {
				std::cout << v->index() << " " << std::flush;
			}
			std::cout << std::endl;
			lNr++;
		}
		std::cout << " *-----------------------------------" << std::endl << std::endl;
#endif


		if (m_labels.size() == 1){
			connectSingleLabel();
		}
		else{
			node pendant1, pendant2;
			adjEntry adjV1, adjV2;
			bool foundMatching = findMatching(pendant1, pendant2, adjV1, adjV2);
			if (!foundMatching)
				findMatchingRev(pendant1, pendant2, adjV1, adjV2);

			connectPendants(pendant1, pendant2, adjV1, adjV2);
		}

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG_PLANARCHECK
		if (isPlanar(m_graphCopy))
			std::cout << " graphCopy is planar" << std::endl;
		else{
			std::cout << " graphCopy is NOT planar!!!" << std::endl;
		}
#endif
	}

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG_PLANARCHECK
	if (isBiconnected(m_graphCopy))
		std::cout << " graphCopy is biconnected" << std::endl;
	else{
		std::cout << " graphCopy is NOT biconnected!!!" << std::endl;
	}

	if (isPlanar(m_graphCopy))
		std::cout << " graphCopy is planar" << std::endl;
	else{
		std::cout << " graphCopy is NOT planar!!!" << std::endl;
	}
#endif

	m_pActEmbedding = nullptr;
	m_pBCTree = nullptr;
	delete actComb;
	delete actBCTree;
}

void PlanarAugmentationFix::reduceChain(node pendant)
{
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << "reduceChain() " << pendant->index();
	if (m_pBCTree->typeOfBNode(pendant) == BCTree::BNodeType::BComp)
		std::cout << "[b]"<< std::flush;
	else
		std::cout << "[c]"<< std::flush;
 #endif

	// parent = parent of pendant in the BC-Tree
	// if pendant is the root, then parent == 0
	node parent = m_pBCTree->DynamicBCTree::parent(pendant);

	// last is going to be the last cutvertex in the computation of followPath()
	node last;
	PALabel::StopCause stopCause;

	// traverse from parent to the root of the bc-tree and check several
	// conditions. last is going to be the last cutvertex on this path
	stopCause = followPath(parent, last);

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << ", stopCause == ";
	switch(stopCause){
	case PALabel::StopCause::Planarity:
		// cannot occur
		break;
	case PALabel::StopCause::CDegree:
		std::cout << "CDegree ";
		break;
	case PALabel::StopCause::BDegree:
		std::cout << "BDegree ";
		break;
	case PALabel::StopCause::Root:
		std::cout << "Root ";
		break;
	}
#endif

	if (stopCause == PALabel::StopCause::CDegree || stopCause == PALabel::StopCause::Root){

		if (m_isLabel[last].valid()){
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			std::cout << "reduceChain(): add " << pendant->index() << " to another label" << std::endl;
#endif
			// label that last is the head of
			pa_label label = *m_isLabel[last];
			// add the actual pendant pendant to label
			addPendant(pendant, label);
			// set the stop-cause
			label->stopCause(stopCause);
		} else {
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			std::cout << "reduceChain(): creating a new label with pendant " << pendant->index() << std::endl;
#endif
			newLabel(last, nullptr,  pendant, stopCause);
		}
	}
	else{
		// stopCause == PALabel::StopCause::BDegree
		parent = m_pBCTree->parent(last);

		if (m_isLabel[parent].valid()){
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			std::cout << "reduceChain(): add " << pendant->index() << " to another label" << std::endl;
#endif
			addPendant(pendant, *m_isLabel[parent]);
		} else {
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			std::cout << "reduceChain(): creating a new label with pendant " << pendant->index() << std::endl;
#endif
			newLabel(last, parent, pendant, PALabel::StopCause::BDegree);
		}
	}
}

PALabel::StopCause PlanarAugmentationFix::followPath(node v, node& last)
{
	last = nullptr;
	node bcNode = m_pBCTree->find(v);

	if (m_pBCTree->typeOfBNode(bcNode) == BCTree::BNodeType::CComp){
		last = bcNode;
	}

	while (bcNode != nullptr){
		int deg = m_pBCTree->m_bNode_degree[bcNode];

		if (deg > 2){
			if (m_pBCTree->typeOfBNode(bcNode) == BCTree::BNodeType::CComp){
				last = bcNode;
				return PALabel::StopCause::CDegree;
			}
			else{
				if (m_pBCTree->DynamicBCTree::parent(bcNode) == nullptr)
					return PALabel::StopCause::Root;
				else
					return PALabel::StopCause::BDegree;
			}
		}

		if (m_pBCTree->typeOfBNode(bcNode) == BCTree::BNodeType::CComp){
			last = bcNode;
		}

		// iterate to parent node
		bcNode = m_pBCTree->DynamicBCTree::parent(bcNode);
	}
	return PALabel::StopCause::Root;
}

bool PlanarAugmentationFix::findMatching(node& pendant1, node& pendant2, adjEntry& adjV1, adjEntry& adjV2)
{
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << "findMatching()" << std::endl;
#endif

	pa_label label = m_labels.front();
	pendant2 = nullptr;
	adjV1 = adjV2 = nullptr;
	pendant1 = m_pBCTree->find(label->getFirstPendant());
	node pendantFirst = pendant1;

	node cutV = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hParNode[pendant1]];
	adjEntry adj = cutV->firstAdj();
	if (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) == pendant1){
		while (m_pBCTree->DynamicBCTree::bcproper(adj->twinNode()) == pendant1){
			adjV1 = adj->twin();
			adj = adj->cyclicSucc();
		}
	}
	else{
		while (m_pBCTree->DynamicBCTree::bcproper(adj->twinNode()) != pendant1)
			adj = adj->cyclicPred();
		adjV1 = adj->twin();
		adj = adj->cyclicSucc();
	}

	// adjV1 is the very adjEntry that belongs to pendant1, points to the cutvertex
	//   and is the rightmost adjEntry with this properties
	adjV1 = adjV1->cyclicPred();

	bool loop = true;
	bool found = false;

	node cutvBFNode = nullptr;
	bool dominatingTree = false;

	while (loop){

		if (m_pBCTree->typeOfGNode(adj->theNode()) == BCTree::GNodeType::CutVertex){
			if (!dominatingTree){
				if (adj->theNode() == cutvBFNode){
					dominatingTree = true;
				}
				else{
					if ((cutvBFNode == nullptr) && (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) == m_actBCRoot))
						cutvBFNode = adj->theNode();
				}
			}
		}
		else{
			node actPendant = m_pBCTree->DynamicBCTree::bcproper(adj->theNode());

			if ((m_pBCTree->m_bNode_degree[actPendant] == 1) && (actPendant != m_actBCRoot) && (actPendant != pendant1)){

				if (m_belongsTo[actPendant] == label){
					// found pendant of same label
					adjV1 = adj->cyclicPred();
					pendant1 = actPendant;
					label->m_pendants.del(m_belongsToIt[pendant1]);
					m_belongsToIt[pendant1] = label->m_pendants.pushFront(pendant1);
					if (dominatingTree){
						cutvBFNode = nullptr;
					 }
				}
				else{
					// found pendant of different label
					if (dominatingTree){
						// we have a dominating tree
						if (cutvBFNode != nullptr){
							// corrupt situation

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
							std::cout << "findMatching(): found dominating tree: corrupt situation " << std::endl;
#endif

							pendant1 = pendantFirst;
							loop = false;
							found = false;
						}
						else{
							// correct situation

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
							std::cout << "findMatching(): found dominating tree: correct situation " << std::endl;
#endif

							adjV2 = adj->cyclicPred();
							pendant2 = actPendant;
							loop = false;
							found = true;
						}
					}
					else{
						// no dominating tree
						adjV2 = adj->cyclicPred();
						pendant2 = actPendant;
						loop = false;
						found = true;
					}
				}
			}
		}

		adj = adj->twin()->cyclicSucc();
	}

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << "findMatching() finished with found == "<< found << std::endl;
#endif

	return found;
}

void PlanarAugmentationFix::findMatchingRev(node& pendant1, node& pendant2, adjEntry& adjV1, adjEntry& adjV2)
{
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << "findMatchingRev() " << std::endl;
#endif

	pa_label label = m_belongsTo[pendant1];
	pendant2 = nullptr;
	adjV1 = adjV2 = nullptr;


	node cutV = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hParNode[pendant1]];
	adjEntry adj = cutV->firstAdj();
	if (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) == pendant1){
		while (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) == pendant1){
			adjV1 = adj->twin();
			adj = adj->cyclicPred();
		}
	}
	else{
		while (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) != pendant1)
			adj = adj->cyclicSucc();
		adjV1 = adj->twin();
		adj = adj->cyclicPred();
	}

	bool loop = true;

	while (loop){

		if (m_pBCTree->typeOfGNode(adj->theNode()) == BCTree::GNodeType::Normal){

			node actPendant = m_pBCTree->DynamicBCTree::bcproper(adj->theNode());

			if (m_pBCTree->m_bNode_degree[actPendant] == 1){

				if (m_belongsTo[actPendant] == label){
					// found pendant of same label
					adjV1 = adj;
					pendant1 = actPendant;
					label->m_pendants.del(m_belongsToIt[pendant1]);
					m_belongsToIt[pendant1] = label->m_pendants.pushBack(pendant1);
				}
				else{
					// found pendant of different label
					adjV2 = adj;
					pendant2 = actPendant;
					loop = false;
				}

			}
		}

		adj = adj->twin()->cyclicPred();
	}
}

void PlanarAugmentationFix::connectPendants(node pendant1, node pendant2, adjEntry adjV1, adjEntry adjV2)
{
	edge newEdgeCopy = m_pActEmbedding->splitFace(adjV1, adjV2);

	adjEntry adjOrigV1 = (m_graphCopy.original(adjV1->theEdge()))->adjSource();
	if (adjOrigV1->theNode() != m_graphCopy.original(adjV1->theNode()))
		adjOrigV1 = adjOrigV1->twin();

	adjEntry adjOrigV2 = (m_graphCopy.original(adjV2->theEdge()))->adjSource();
	if (adjOrigV2->theNode() != m_graphCopy.original(adjV2->theNode()))
		adjOrigV2 = adjOrigV2->twin();

	edge newEdgeOrig = m_pEmbedding->splitFace(adjOrigV1, adjOrigV2);
	m_pResult->pushBack(newEdgeOrig);

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << "connectPendants(" << pendant1->index() << ", " << pendant2->index() << ", " << adjV1 << ", " << adjV2 << ")" << std::endl;
	std::cout << "                leads to edge " << newEdgeCopy << std::endl;
	std::cout << "                and new edge in the orig. " << newEdgeOrig << std::endl;
#endif

	m_pBCTree->updateInsertedEdge(newEdgeCopy);
	m_graphCopy.setEdge(newEdgeOrig, newEdgeCopy);

	pa_label l1 = m_belongsTo[pendant1];
	pa_label l2 = m_belongsTo[pendant2];

	deletePendant(pendant1);
	deletePendant(pendant2);

	if (l2->size() > 0){
		if (l2->size() == 1){
			node pendant = l2->getFirstPendant();
			deleteLabel(l2);
			reduceChain(pendant);
		}
		else{
			removeLabel(l2);
			insertLabel(l2);
		}
	}
	else
		deleteLabel(l2);

	if (l1->size() > 0){
		if (l1->size() == 1){
			node pendant = l1->getFirstPendant();
			deleteLabel(l1);
			reduceChain(pendant);
		}
		else{
			removeLabel(l1);
			insertLabel(l1);
		}
	}
	else
		deleteLabel(l1);

	m_actBCRoot = m_pBCTree->find(m_actBCRoot);

	node bcNode = m_pBCTree->DynamicBCTree::bcproper(newEdgeCopy);
	if ((bcNode != pendant1) && (bcNode != pendant2) && (m_pBCTree->m_bNode_degree[bcNode] == 1)
	&& (bcNode != m_actBCRoot))
		reduceChain(bcNode);
}

void PlanarAugmentationFix::connectSingleLabel()
{
	pa_label label = m_labels.front();
	node pendant1 = label->getFirstPendant();

	node cutV = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hParNode[pendant1]];
	adjEntry adjRun = cutV->firstAdj();
	m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge());

	if (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == pendant1){
		while (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == pendant1){
			adjRun = adjRun->cyclicSucc();
		}
	}
	else{
		while (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) != pendant1)
			adjRun = adjRun->cyclicPred();
		adjRun = adjRun->cyclicSucc();
	}
	adjEntry adj = adjRun->twin();
	adjEntry adjFirst = adj;
	adj = adj->cyclicPred();

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	bool singleEdgePendant = false;
	if (adj == adj->cyclicSucc()) {
		singleEdgePendant = true;
	}

	std::cout
	  << "connectSinlgeLabel(): cutv=" << cutV << ", adj=" << adj << ", adjRun=" << adjRun << "(pred=" << adjRun->cyclicPred() <<")"
	  << ", adjFirst=" << adjFirst << ", singleEdgePendant=" << singleEdgePendant << std::endl;
#endif

	if (label->size() > 1){

		bool connectLeft  = false;
		bool connectRight = false;
		node adjBNode = nullptr;
		node lastConnectedPendant = nullptr;

		node cutvBFNode = nullptr;
		bool loop = true;

		adjBNode = m_pBCTree->bcproper(adj->theEdge());

		// first connect pendants "on the right" of the first pendant
		while (loop){

			if (m_pBCTree->typeOfGNode(adjRun->theNode()) == BCTree::GNodeType::CutVertex){
				if (adjRun->theNode() == cutvBFNode){
					loop = false;
				}
				else{
					if ((cutvBFNode == nullptr) && (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == m_actBCRoot))
						cutvBFNode = adjRun->theNode();
				}
			}
			else{
				node actPendant = m_pBCTree->DynamicBCTree::bcproper(adjRun->theNode());

				if ((m_pBCTree->m_bNode_degree[actPendant] == 1)
				&& (actPendant != m_pBCTree->find(adjBNode))
				&& (actPendant != lastConnectedPendant)
				&& (actPendant != m_actBCRoot)){

					if (!connectRight)
						connectRight = true;

					lastConnectedPendant = actPendant;
					adjRun = adjRun->cyclicPred();

					edge newEdgeCopy = m_pActEmbedding->splitFace(adj, adjRun);

					adjEntry adjOrigV1 = (m_graphCopy.original(adj->theEdge()))->adjSource();
					if (adjOrigV1->theNode() != m_graphCopy.original(adj->theNode()))
						adjOrigV1 = adjOrigV1->twin();
					adjEntry adjOrigV2 = (m_graphCopy.original(adjRun->theEdge()))->adjSource();
					if (adjOrigV2->theNode() != m_graphCopy.original(adjRun->theNode()))
						adjOrigV2 = adjOrigV2->twin();

					edge newEdgeOrig = m_pEmbedding->splitFace(adjOrigV1, adjOrigV2);
					m_pResult->pushBack(newEdgeOrig);

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
					std::cout << "connectSingleLabel(): splitFace (on the right) with " << adj << " and " << adjRun << std::endl;
					std::cout << "                      leads to edge " << newEdgeCopy << std::endl;
					std::cout << "                      newEdge in the orig. graph " << newEdgeOrig << std::endl;
#endif

					m_graphCopy.setEdge(newEdgeOrig, newEdgeCopy);
					adjRun = adjRun->cyclicSucc()->cyclicSucc();
				}
			}
			adjRun = adjRun->twin()->cyclicSucc();
		}

		// now connect pendants "on the left" of the first pendant
		adjRun = adjFirst->twin();
		while (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == pendant1)
			adjRun = adjRun->cyclicPred();
		adj = adjRun->cyclicSucc()->twin();

		cutvBFNode = nullptr;
		loop = true;

		while (loop){

			if (m_pBCTree->typeOfGNode(adjRun->theNode()) == BCTree::GNodeType::CutVertex){
				if (adjRun->theNode() == cutvBFNode){
					loop = false;
				}
				else{
					if ((cutvBFNode == nullptr) && (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == m_actBCRoot))
						cutvBFNode = adjRun->theNode();
				}
			}
			else{
				node actPendant = m_pBCTree->DynamicBCTree::bcproper(adjRun->theNode());
				if  ((m_pBCTree->m_bNode_degree[actPendant] == 1)
				&& (actPendant != m_pBCTree->find(adjBNode))
				&& (actPendant != lastConnectedPendant)
				&& (actPendant != m_actBCRoot)){

					if (!connectLeft)
						connectLeft = true;

					lastConnectedPendant = actPendant;
					edge newEdgeCopy = m_pActEmbedding->splitFace(adj, adjRun);

					adjEntry adjOrigV1 = (m_graphCopy.original(adj->theEdge()))->adjSource();
					if (adjOrigV1->theNode() != m_graphCopy.original(adj->theNode()))
						adjOrigV1 = adjOrigV1->twin();
					adjEntry adjOrigV2 = (m_graphCopy.original(adjRun->theEdge()))->adjSource();
					if (adjOrigV2->theNode() != m_graphCopy.original(adjRun->theNode()))
						adjOrigV2 = adjOrigV2->twin();

					edge newEdgeOrig = m_pEmbedding->splitFace(adjOrigV1, adjOrigV2);
					m_pResult->pushBack(newEdgeOrig);

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
					std::cout << "connectSingleLabel(): splitFace (on the left) with " << adj << " and " << adjRun << std::endl;
					std::cout << "                      leads to edge " << newEdgeCopy << std::endl;
					std::cout << "                      newEdge in the orig. graph " << newEdgeOrig << std::endl;
#endif

					m_graphCopy.setEdge(newEdgeOrig, newEdgeCopy);
					adj = adj->cyclicSucc();
				}
			}

			adjRun = adjRun->twin()->cyclicPred();
		}
	}

	adjRun = adj->cyclicSucc();

	while (m_pBCTree->DynamicBCTree::bcproper(adjRun->theNode()) != m_pBCTree->find(m_actBCRoot)){
		adjRun = adjRun->twin()->cyclicSucc();
	}
	adjRun = adjRun->cyclicPred();

	edge newEdgeCopy = m_pActEmbedding->splitFace(adj, adjRun);

	adjEntry adjOrigV1 = (m_graphCopy.original(adj->theEdge()))->adjSource();
	if (adjOrigV1->theNode() != m_graphCopy.original(adj->theNode()))
		adjOrigV1 = adjOrigV1->twin();
	adjEntry adjOrigV2 = (m_graphCopy.original(adjRun->theEdge()))->adjSource();
	if (adjOrigV2->theNode() != m_graphCopy.original(adjRun->theNode()))
		adjOrigV2 = adjOrigV2->twin();

	edge newEdgeOrig = m_pEmbedding->splitFace(adjOrigV1, adjOrigV2);
	m_pResult->pushBack(newEdgeOrig);

#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << "connectSingleLabel(): splitFace (last edge) with " << adj << " and " << adjRun << std::endl;
	std::cout << "                      leads to edge " << newEdgeCopy << std::endl;
	std::cout << "                      newEdge in the orig. graph " << newEdgeOrig << std::endl;
#endif

	m_graphCopy.setEdge(newEdgeOrig, newEdgeCopy);

	deleteLabel(label);
}

pa_label PlanarAugmentationFix::newLabel(node cutvertex, node parent, node pendant, PALabel::StopCause whyStop)
{
	pa_label label;
	label = new PALabel(parent, cutvertex, whyStop);

	m_belongsTo[pendant] = label;
	m_belongsToIt[pendant] = (label->m_pendants).pushBack(pendant);

	if (parent != nullptr)
		m_isLabel[parent] = m_labels.pushBack(label);
	else
		m_isLabel[cutvertex] = m_labels.pushBack(label);

	return label;
}

void PlanarAugmentationFix::deleteLabel(pa_label& label){
	ListIterator<pa_label> labelIt = m_isLabel[label->parent()];

	m_labels.del(labelIt);
	m_isLabel[label->parent()] = nullptr;

	for(node v : label->m_pendants) {
		m_belongsTo[v] = nullptr;
		m_belongsToIt[v] = nullptr;
	}

	delete label;
	label = nullptr;
}

void PlanarAugmentationFix::removeLabel(pa_label& label){
	ListIterator<pa_label> labelIt = m_isLabel[label->parent()];

	m_labels.del(labelIt);
}

void PlanarAugmentationFix::addPendant(node pendant, pa_label& label)
{
	m_belongsTo[pendant] = label;
	m_belongsToIt[pendant] = (label->m_pendants).pushBack(pendant);

	node newParent = m_pBCTree->find(label->parent());

	m_labels.del(m_isLabel[label->parent()]);
	m_isLabel[newParent] = insertLabel(label);
}

void PlanarAugmentationFix::deletePendant(node pendant){
	(m_belongsTo[pendant])->removePendant(m_belongsToIt[pendant]);
	m_belongsTo[pendant] = nullptr;
	m_belongsToIt[pendant] = nullptr;
}

ListIterator<pa_label> PlanarAugmentationFix::insertLabel(pa_label label)
{
	if (m_labels.size() == 0){
		return m_labels.pushFront(label);
	}
	else{
		ListIterator<pa_label> it = m_labels.begin();
		while (it.valid() && ((*it)->size() > label->size())){
			++it;
		}
		if (!it.valid())
			return m_labels.pushBack(label);
		else
			return m_labels.insert(label, it, Direction::before);
	}
}

void PlanarAugmentationFix::modifyBCRoot(node oldRoot, node newRoot)
{
#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	std::cout << "modifyBCRoot()" << std::endl;
#endif

	SList<node> *path = m_pBCTree->findPathBCTree(oldRoot, newRoot);
	SListIterator<node> it = path->begin();
	SListIterator<node> it2 = path->begin();

	while (it.valid()){
		if (it2 != it){
			changeBCRoot((*it2), (*it));
		}

		it2 = it;
		++it;
	}

	delete path;
}

void PlanarAugmentationFix::changeBCRoot(node oldRoot, node newRoot)
{
	//   for the old root:
	m_pBCTree->m_bNode_hRefNode[oldRoot] = m_pBCTree->m_bNode_hParNode[newRoot];
	m_pBCTree->m_bNode_hParNode[oldRoot] = m_pBCTree->m_bNode_hRefNode[newRoot];

	//   for the new root:
#if 0
	m_pBCTree->m_bNode_hRefNode[newRoot] = no update required;
#endif
	m_pBCTree->m_bNode_hParNode[newRoot] = nullptr;
}

}
