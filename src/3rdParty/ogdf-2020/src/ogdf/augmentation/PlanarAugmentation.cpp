/** \file
 * \brief planar biconnected augmentation approximation algorithm
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

#include <ogdf/augmentation/PlanarAugmentation.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>

// for debug-outputs
//#define PLANAR_AUGMENTATION_DEBUG

// for checking planarity directly after inserting a new edge
//   and additional planarity tests after each augmentation round
//#define PLANAR_AUGMENTATION_DEBUG_PLANARCHECK

namespace ogdf {

void PALabel::removePendant(node pendant)
{
	if (m_pendants.size() > 0){
		ListIterator<node> it = m_pendants.begin();
		for (; it.valid(); ++it)
			if ((*it) == pendant){
				m_pendants.del(it);
				break;
			}
	}
}


void PlanarAugmentation::doCall(Graph& g, List<edge>& list)
{
	m_nPlanarityTests = 0;

	list.clear();
	m_pResult = &list;

	m_pGraph = &g;

#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout << "Graph G has no self loops = " << isLoopFree(*m_pGraph) << std::endl;
	std::cout << "Graph G is planar         = " << isPlanar(*m_pGraph)<< std::endl;
	std::cout << "Graph G is connected      = " << isConnected(*m_pGraph) << std::endl;
	std::cout << "Graph G is biconnected    = " << isBiconnected(*m_pGraph) << std::endl;
#endif

	// create the bc-tree
	if (m_pGraph->numberOfNodes() > 1){

		if (!isConnected(*m_pGraph)){
			if(m_pGraph->numberOfEdges() == 0){
				// one edge is required
				m_pResult->pushBack(m_pGraph->newEdge(m_pGraph->firstNode(), m_pGraph->firstNode()->succ()));
			}

			makeConnectedByPendants();
		}

		m_pBCTree = new DynamicBCTree(*m_pGraph);

		// init the m_adjNonChildren-NodeArray with all adjEntries of the bc-tree
		m_adjNonChildren.init(m_pBCTree->m_B);

		for(node v : m_pBCTree->bcTree().nodes){
			if (v->firstAdj() != nullptr){
				m_adjNonChildren[v].pushFront(v->firstAdj());
				adjEntry adj = v->firstAdj()->cyclicSucc();
				while (adj != v->firstAdj()){
					m_adjNonChildren[v].pushBack(adj);
					adj = adj->cyclicSucc();
				}
			}
		}
		m_isLabel.init(m_pBCTree->bcTree(), nullptr);
		m_belongsTo.init(m_pBCTree->bcTree(), nullptr);

		// call main function
		augment();
	}
}



// makes graph connected by inserting edges between
// nodes of pendants of the connected components
void PlanarAugmentation::makeConnectedByPendants()
{
	DynamicBCTree bcTreeTemp(*m_pGraph, true);

	NodeArray<int> components;
	components.init(*m_pGraph, 0);

	int compCnt = connectedComponents(*m_pGraph, components);

	List<node> getConnected;

	Array<bool> compConnected(compCnt);
	for (int i=0; i<compCnt; i++){
		compConnected[i] = false;
	}

	for(node v : m_pGraph->nodes) {
		if (v->degree() == 0){
			// found a seperated node that will be connected
			getConnected.pushBack(v);
			compConnected[components[v]] = true;
		}
	}

	for(node v : m_pGraph->nodes) {
		if (!compConnected[components[v]] && bcTreeTemp.bcproper(v)->degree() <= 1) {
			// found a node that will be connected
			getConnected.pushBack(v);
			compConnected[components[v]] = true;
		}
	}

	ListIterator<node> it = getConnected.begin();
	ListIterator<node> itBefore = getConnected.begin();
	while (it.valid()){
		if (it != itBefore){
			// insert edge between it and itBefore
			m_pResult->pushBack(m_pGraph->newEdge(*it, *itBefore));
			++itBefore;
		}
		++it;
	}
}

// the main augmentation function
void PlanarAugmentation::augment()
{
	node rootPendant = nullptr;

	// first initialize the list of pendants
	for(node v : m_pBCTree->bcTree().nodes){
		if (v->degree() == 1){
#ifdef PLANAR_AUGMENTATION_DEBUG
			std::cout << "augment(): found pendant with index " << v->index();
#endif
			if (m_pBCTree->parent(v) == nullptr){
				rootPendant = v;
#ifdef PLANAR_AUGMENTATION_DEBUG
				std::cout << " is root! (also inserted into pendants-list!)" << std::endl;
#endif
			} else {
#ifdef PLANAR_AUGMENTATION_DEBUG
				std::cout << std::endl;
#endif
			}
			m_pendants.pushBack(v);
		}
	}

	if (rootPendant != nullptr){
		// the root of the bc-tree is also a pendant
		// this has to be changed

		node bAdjNode = rootPendant->firstAdj()->twinNode();

#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout << "augment(): changing root in bc-tree because root is a pendant!" << std::endl;
		std::cout << "augment(): index of old root = " << rootPendant->index() << ", new root = " << bAdjNode->index() << std::endl;
#endif

		// modify the bc-tree-structure
		modifyBCRoot(rootPendant, bAdjNode);
	}

	// call reduceChain for all pendants
	if (m_pendants.size() > 1) {
		for (node v : m_pendants) {
			reduceChain(v);
		}
	}

	// it can appear that reduceChain() inserts some edges
	//  in case of non-planarity (Planarity)
	// so there are new pendants and obsolete pendants
	//  the obsolete pendants are collected in m_pendantsToDel
	if (m_pendantsToDel.size() > 0){
		ListIterator<node> delIt = m_pendantsToDel.begin();
		for (; delIt.valid(); delIt = m_pendantsToDel.begin()){
			deletePendant(*delIt);
			m_pendantsToDel.del(delIt);
		}
	}

#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout << "augment(): after reduceChain() for every pendant:" << std::endl;
	std::cout << "           #labels = " << m_labels.size() << std::endl;
	std::cout << "           #pendants = " << m_pendants.size() << std::endl << std::endl;
	std::cout << "STARTING MAIN LOOP:" << std::endl;
	std::cout << std::endl;
#endif

	// main loop
	while(!m_labels.empty()){
		// foundMatching indicates if there are 2 labels that can be connected
		bool foundMatching;
		// labels first and second are going to be computed by findMatching
		// and foundMatching=true or foundMatching=false
		// first is always != 0 after findMatching(...)
		pa_label first, second = nullptr;

		foundMatching = findMatching(first, second);

		// no matching labels were found
		if (!foundMatching){

			// we have only one label
			if (m_labels.size() == 1){

				if (m_pendants.size() > 1)
					//m_labels.size() == 1 &&  m_pendants.size() > 1
					// join the pendants of this label
					joinPendants(first);
				else{
					//m_labels.size() == 1 &&  m_pendants.size() == 1
					connectInsideLabel(first);
				}
			}
			else{
				// m_labels.size() > 1

				if (first->size() == 1){
					// m_labels.size() > 1 && first->size() == 1
					// connect the
					connectInsideLabel(first);
				}
				else{
					// m_labels.size() > 1 && first->size() > 1
					// so connect all pendants of label first
					joinPendants(first);
				}
			}
		} else {
			connectLabels(first, second);
		}

		// output after each round:
#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout << std::endl << "augment(): output after one round:" << std::endl;
		std::cout         << "           #labels   = " << m_labels.size() << std::endl;
		std::cout         << "           #pendants = " << m_pendants.size() << std::endl;
#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARCHECK
		std::cout << "graph is planar == " << isPlanar(*m_pGraph) << std::endl;
		std::cout << "graph is biconnected == " << isBiconnected(*m_pGraph) << std::endl;
#endif
		std::cout << std::endl;

		ListIterator<pa_label> labelIt = m_labels.begin();
		int pos = 1;
		for (; labelIt.valid(); labelIt++){
			std::cout << "pos " << pos << ": ";
			if ((m_isLabel[(*labelIt)->parent()]).valid()) {
				std::cout
				  << " OK, parent-index = " << (*labelIt)->parent()->index()
				  << ", size = " << (*labelIt)->size() << std::endl;
			} else {
				std::cout
				  << " ERROR, parent-index = " << (*labelIt)->parent()->index()
				  << ", size = " << (*labelIt)->size() << std::endl;
			}

			pos++;
		}
		std::cout << std::endl;
#endif
		// : output after each round
	}

#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout << std::endl << "FINISHED MAIN LOOP" << std::endl << std::endl;
	std::cout << "# planarity tests = " << m_nPlanarityTests << std::endl;
#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARITY
	std::cout << "resulting Graph is biconnected = " << isBiconnected(*m_pGraph) << std::endl;
	std::cout << "resulting Graph is planar = " << isPlanar(*m_pGraph) << std::endl;
#endif
	std::cout << std::endl;
#endif

	terminate();
}



// finds the "parent" (->label) for a pendant p of the BC-Tree
// and creates a new label or inserts the pendant to another
// label
// reduceChain can also insert edges in case of Planarity
void PlanarAugmentation::reduceChain(node pendant, pa_label labelOld)
{
#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout << "reduceChain(" << pendant->index() << ")";
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

#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout << ", stopCause == ";
	switch(stopCause){
	case PALabel::StopCause::Planarity:
		std::cout << "Planarity" << std::endl;
		break;
	case PALabel::StopCause::CDegree:
		std::cout << "CDegree" << std::endl;
		break;
	case PALabel::StopCause::BDegree:
		std::cout << "BDegree" << std::endl;
		break;
	case PALabel::StopCause::Root:
		std::cout << "Root" << std::endl;
		break;
	}
#endif


	if (stopCause == PALabel::StopCause::Planarity){
		node adjToCutP    = adjToCutvertex(pendant);
		node adjToCutLast = adjToCutvertex(m_pBCTree->DynamicBCTree::parent(last), last);

		// computes path in bc-tree between bcproper(adjToCutP) and bcproper(adjToCutLast)
		SList<node>& path = m_pBCTree->findPath(adjToCutP, adjToCutLast);

#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout
		  << "reduceChain(): inserting edge between " << adjToCutP->index()
		  << " and " << adjToCutLast->index() << std::endl;
#endif

		// create new edge
		edge e = m_pGraph->newEdge(adjToCutP, adjToCutLast);

		// insert the edge into the result-list
		m_pResult->pushBack(e);

		// update the bc-Tree with new edge
		m_pBCTree->updateInsertedEdge(e);

		// find the new arised pendant
		node newPendant = m_pBCTree->find(pendant);

		if (newPendant != pendant){
			// delete the old pendant
			// cannot delete the pendant immediatly
			// because that would affect the outer loop in augment()
			m_pendantsToDel.pushBack(pendant);
			// insert the new arised pendant
			// at the front of m_pendants becuse that doesn't affect the outer loop in augment()
			m_pendants.pushFront(newPendant);
		}

		// updating m_adjNonChildren
		updateAdjNonChildren(newPendant, path);

		// check if newPendant is the new root of the bc-tree
		if (m_pBCTree->DynamicBCTree::parent(newPendant) == nullptr){
#ifdef PLANAR_AUGMENTATION_DEBUG
			std::cout
			  << "reduceChain(): new arised pendant is the new root of the bc-tree, it has degree "
			  << m_pBCTree->m_bNode_degree[newPendant] << std::endl;
#endif

			node newRoot = (*(m_adjNonChildren[newPendant].begin()))->twinNode();

			// modify bc-tree-structure
			modifyBCRoot(newPendant, newRoot);
		}

		delete &path;

		// delete label if necessary
		if (labelOld != nullptr){
			deleteLabel(labelOld);
		}

#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout << "reduceChain(): calling reduceChain() with newPendant = " << newPendant->index() << std::endl;
#endif

		// call reduceChain with the new arised pendant
		reduceChain(newPendant);
	}

	if (stopCause == PALabel::StopCause::CDegree || stopCause == PALabel::StopCause::Root){

		if (labelOld != nullptr){
			if (labelOld->head() == last){
				// set the stop-cause
				labelOld->stopCause(stopCause);
			}
			else
				deleteLabel(labelOld);
		}

		if (m_isLabel[last].valid()){
			// label is the label that last is the head of
			pa_label label = *(m_isLabel[last]);
			// add the actual pendant pendant to label
			addPendant(pendant, label);
			// set the stop-cause
			label->stopCause(stopCause);
		}
		else{
			newLabel(last, pendant, stopCause);
		}
	}

	if (stopCause == PALabel::StopCause::BDegree){
		if (labelOld != nullptr){
			if (labelOld->head() != last){
				deleteLabel(labelOld);
				newLabel(last, pendant, PALabel::StopCause::BDegree);
			}
			else{
				labelOld->stopCause(PALabel::StopCause::BDegree);
			}
		}
		else{
			newLabel(last, pendant, PALabel::StopCause::BDegree);
		}
	}
}

// traverses the BC-Tree upwards from v
// (v is always a parent of a pendant)
//
// last becomes the last cutvertex before we return
PALabel::StopCause PlanarAugmentation::followPath(node v, node& last)
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
			else
				return PALabel::StopCause::BDegree;
		}

		// deg == 2 (case deg < 2 cannot occur)
		if (m_pBCTree->typeOfBNode(bcNode) == BCTree::BNodeType::CComp){
			last = bcNode;
		}
		else{
			// bcNode is a BComp and degree is 2
			if (m_pBCTree->numberOfNodes(bcNode) > 4){
				// check planarity if number of nodes > 4
				// because only than a K5- or k33-Subdivision can be included

				node adjBCNode = nullptr;

				bool found = false;
				SListIterator<adjEntry> childIt = m_adjNonChildren[bcNode].begin();
				while (!found && childIt.valid()){
					if (m_pBCTree->find((*childIt)->twinNode()) != last){
						found = true;
						adjBCNode = m_pBCTree->find((*childIt)->twinNode());
					}
					++childIt;
				}

				// get nodes in biconnected-components graph of m_pBCTree
				node hNode = m_pBCTree->m_bNode_hRefNode[last];
				node hNode2 = m_pBCTree->m_bNode_hRefNode[adjBCNode];

				// check planarity for corresponding graph-nodes of hNode and hNode2
				if (!planarityCheck(m_pBCTree->m_hNode_gNode[hNode],
									m_pBCTree->m_hNode_gNode[hNode2])){
					return PALabel::StopCause::Planarity;
				}
			}
		}
		// iterate to parent node
		bcNode = m_pBCTree->DynamicBCTree::parent(bcNode);
	}
	// reached the bc-tree-root
	return PALabel::StopCause::Root;
}



// checks planarity for the new edge (v1, v2)
// v1 and v2 are nodes in the original graph
bool PlanarAugmentation::planarityCheck(node v1, node v2)
{
	// first simple tests
	if (v1 == v2){
		return true;
	}

	// check if edge (v1, v2) already exists
	if (v1->firstAdj()->twinNode() == v2){
		return true;
	}
	adjEntry adjTest = v1->firstAdj()->cyclicSucc();
	while (adjTest != v1->firstAdj()){
		if (v1->firstAdj()->twinNode() == v2){
			return true;
		}
		adjTest = adjTest->cyclicSucc();
	}

	// test planarity for edge (v1, v2)
	edge e = m_pGraph->newEdge(v1, v2);

	m_nPlanarityTests++;

	bool planar = planarEmbed(*m_pGraph);

	// finally delete the edge
	m_pGraph->delEdge(e);

	return planar;
}



// returns the vertex in the original graph that
// belongs to v (B-Component in the BC-Graph and pendant)
// and is adjacent to the cutvertex (also node of the BC-Graph)
// if cutvertex == 0 then the cutvertex of the parent of v
// is considered
node PlanarAugmentation::adjToCutvertex(node v, node cutvertex)
{
	node nodeAdjToCutVertex;

	if (cutvertex == nullptr){

		// set nodeAdjToCutVertex to the node in the original graph,
		//  that corresponds to the parent (c-component) of v in the bc-tree
		nodeAdjToCutVertex = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hParNode[v]];

		// adj = adjEntry at the cutvertex
		adjEntry adj = nodeAdjToCutVertex->firstAdj();

		while (m_pBCTree->DynamicBCTree::bcproper(adj->twinNode()) != v)
			adj = adj->cyclicSucc();

		nodeAdjToCutVertex = adj->twinNode();

	}
	else{
		// set nodeAdjToCutVertex to the node in the original graph,
		//  corresponding to the cutvertex in the bc-tree
		nodeAdjToCutVertex = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[cutvertex]];

		// adj = adjEntry at the cutvertex
		adjEntry adj = nodeAdjToCutVertex->firstAdj();

		bool found = false;

		if (m_pBCTree->bComponent(nodeAdjToCutVertex, adj->twinNode()) == v){
			found = true;
			nodeAdjToCutVertex = adj->twinNode();
		}
		else{
			adj = adj->cyclicSucc();
			while ((!found) && (adj != nodeAdjToCutVertex->firstAdj())){
				if (m_pBCTree->bComponent(nodeAdjToCutVertex, adj->twinNode()) == v){
					nodeAdjToCutVertex = adj->twinNode();
					found = true;
				}
				adj = adj->cyclicSucc();
			}
		}
	}
	return nodeAdjToCutVertex;
}



// returns the last vertex before ancestor
// on the path from pendant to ancestor
node PlanarAugmentation::findLastBefore(node pendant, node ancestor)
{
	node bcNode = pendant;
	while ((bcNode) && (m_pBCTree->DynamicBCTree::parent(bcNode) != ancestor))
		bcNode = m_pBCTree->DynamicBCTree::parent(bcNode);

	if (!bcNode){
		// should never occur
		return nullptr;
	}

	return bcNode;
}

// deletes pendant from the list of all pendants
// and also from the label it belongs to
void PlanarAugmentation::deletePendant(node pendant, bool removeFromLabel)
{
	ListIterator<node> mPendantsIt = m_pendants.begin();

	bool deleted = false;
	while (!deleted && mPendantsIt.valid()){
		ListIterator<node> itSucc = mPendantsIt.succ();
		if ((*mPendantsIt) == pendant){
			m_pendants.del(mPendantsIt);
			deleted = true;
		}
		mPendantsIt = itSucc;
	}

	if ((removeFromLabel) && (m_belongsTo[pendant] != nullptr)){
		(m_belongsTo[pendant])->removePendant(pendant);
		m_belongsTo[pendant] = nullptr;
	}
}



// deletes a label
// and - if desired - removes the pendants belonging to label
void PlanarAugmentation::removeAllPendants(pa_label& label)
{
	while (label->size() > 0){
		m_belongsTo[label->getFirstPendant()] = nullptr;
		label->removeFirstPendant();
	}
}



// adds a pendant to label
// re-inserts also label to m_labels
void PlanarAugmentation::addPendant(node pendant, pa_label& label)
{
	m_belongsTo[pendant] = label;
	label->addPendant(pendant);

	node newParent = m_pBCTree->find(label->parent());

	m_labels.del(m_isLabel[label->parent()]);
	m_isLabel[newParent] = insertLabel(label);
}



// connects all pendants of the label
void PlanarAugmentation::joinPendants(pa_label& label)
{
#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout << "joinPendants(): label->size()==" << label->size() << std::endl;
#endif

	node pendant1 = label->getFirstPendant();
	// delete pendant from m_pendants but not from the label it belongs to
	deletePendant(pendant1, false);

	SList<edge> newEdges;

	// traverse through pendant-list and connect them
	ListIterator<node> pendantIt = (label->m_pendants).begin();
	while (pendantIt.valid()){

		if (*pendantIt != pendant1){

			// delete pendant from m_pendants but not from the label it belongs to
			deletePendant(*pendantIt, false);

#ifdef PLANAR_AUGMENTATION_DEBUG
			std::cout
			  << "joinPendants(): connectPendants: " << pendant1->index()
			  << " and " << (*pendantIt)->index() << std::endl;
#endif

			// connect pendants and insert edge in newEdges
			newEdges.pushBack(connectPendants(pendant1, *pendantIt));

			// iterate pendant1
			pendant1 = *pendantIt;
		}
		++pendantIt;
	}

	// update new edges
	updateNewEdges(newEdges);

	removeAllPendants(label);

	SListIterator<edge> edgeIt = newEdges.begin();
	node newBlock = (m_pBCTree->DynamicBCTree::bcproper(*edgeIt));
	if (m_pBCTree->m_bNode_degree[newBlock] == 1){
#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout << "joinPendants(): new block " << newBlock->index() << " has degree 1 " << std::endl;
#endif

		m_belongsTo[newBlock] = label;
		addPendant(newBlock, label);
		m_pendants.pushBack(newBlock);

	}
	else{
#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout << "joinPendants(): new block has degree " << m_pBCTree->m_bNode_degree[newBlock] << std::endl;
#endif
		deleteLabel(label);
	}
}



// connects the only pendant of label with a computed "ancestor"
void PlanarAugmentation::connectInsideLabel(pa_label& label)
{
#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout
	  << "connectInsideLabel(): label->size() == " << label->size() << ", parent = " << label->parent()->index()
	  << ", head = " << label->head()->index() << std::endl;
#endif

	node head = label->head();
	node pendant = label->getFirstPendant();

	node ancestor = m_pBCTree->DynamicBCTree::parent(head);

	node v1 = adjToCutvertex(pendant);

	// check if head is the root of the BC-Tree
	if (ancestor == nullptr){
		node wrongAncestor = findLastBefore(pendant, head);

		SListIterator<adjEntry> adjIt = m_adjNonChildren[head].begin();
		bool found = false;
		while ((!found) && (adjIt.valid())){

			if (m_pBCTree->find((*adjIt)->twinNode()) != wrongAncestor){
				ancestor = m_pBCTree->find((*adjIt)->twinNode());
				found = true;
			}
			++adjIt;
		}
	}

	node v2 = adjToCutvertex(ancestor, head);

#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout << "connectInsideLabel(): inserting edge between " << v1->index() << " and " << v2->index() << std::endl;
#endif

	SList<edge> newEdges;
	edge e = m_pGraph->newEdge(v1, v2);
	newEdges.pushFront(e);

#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARCHECK
	if (!isPlanar(*m_pGraph))
		std::cout << "connectInsideLabel(): CRITICAL ERROR!!! inserted non-planar edge!!! (in connectInsideLabel())" << std::endl;
#endif

	updateNewEdges(newEdges);

	node newBlock = m_pBCTree->DynamicBCTree::bcproper(e);

	// delete label label, and also the pendant
	deleteLabel(label);

	if (m_pBCTree->m_bNode_degree[newBlock] == 1){
#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout << "connectInsideLabel(): new block " << newBlock->index() << " has degree 1... calling reduceChain() ";
#endif
		m_pendants.pushBack(newBlock);
		if ((m_belongsTo[newBlock] != nullptr) && (m_belongsTo[newBlock]->size() == 1)){
			reduceChain(newBlock, m_belongsTo[newBlock]);
		}
		else{
			reduceChain(newBlock);
			// it can appear that reduceChain() inserts some edges
			// so there are new pendants and obsolete pendants
			//  the obsolete pendants are collected in m_pendantsToDel
			if (m_pendantsToDel.size() > 0){
				ListIterator<node> delIt = m_pendantsToDel.begin();
				for (; delIt.valid(); delIt = m_pendantsToDel.begin()){
					deletePendant(*delIt);
					m_pendantsToDel.del(delIt);
				}
			}
		}
	}
}



// connects the two pendants with a new edge
edge PlanarAugmentation::connectPendants(node pendant1, node pendant2)
{
	node v1 = adjToCutvertex(pendant1);
	node v2 = adjToCutvertex(pendant2);

#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout << "connectPendants(): inserting edge between " << v1->index() << " and " << v2->index() << std::endl;
#endif

	edge e = m_pGraph->newEdge(v1, v2);

#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARCHECK
	if (!(isPlanar(*m_pGraph)))
		std::cout << "connectLabels(): CRITICAL ERROR in connectPendants: inserted edge is not planar!!!" << std::endl;
#endif

	return e;
}



// inserts a label at the correct position in the list
ListIterator<pa_label> PlanarAugmentation::insertLabel(pa_label label)
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



// deletes a label and - if desired - removes the pendants belonging to label
void PlanarAugmentation::deleteLabel(pa_label& label, bool removePendants)
{
	ListIterator<pa_label> labelIt = m_isLabel[label->parent()];

	m_labels.del(labelIt);
	m_isLabel[label->parent()] = nullptr;

	for (node v : label->m_pendants)
		m_belongsTo[v] = nullptr;

	if (removePendants) {
		for (node v : label->m_pendants)
		{
			ListIterator<node> mPendantsIt = m_pendants.begin();

			bool deleted = false;
			while (!deleted && mPendantsIt.valid()){
				ListIterator<node> itSucc = mPendantsIt.succ();
				if ((*mPendantsIt) == v) {
					m_pendants.del(mPendantsIt);
					deleted = true;
				}
				mPendantsIt = itSucc;
			}
		}
	}

	delete label;
	label = nullptr;
}



// connects the pendants of first with the pendants of second.
// first.size() >= second.size()
void PlanarAugmentation::connectLabels(pa_label first, pa_label second)
{
#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout
	  << "connectLabels(), first->size()==" << first->size() << " , second->size()=="
	  << second->size() << std::endl;

	std::cout << "connectLabels(): label first = ";
	for (node v : first->m_pendants) {
		std::cout << v->index() << ", ";
	}
	std::cout << " || " << std::endl << "label second = ";
	for (node v : second->m_pendants) {
		std::cout << v->index() << ", ";
	}
	std::cout << std::endl;
#endif

	SList<edge> newEdges;
	ListIterator<node> pendantIt = (second->m_pendants).begin();

	// stores the pendants of label first that were connected
	// because first.size() => second.size()
	SList<node> getConnected;
	int n = 0;

	while (pendantIt.valid()){
		node v2 = first->getPendant(n);
		getConnected.pushBack(v2);
		newEdges.pushBack(connectPendants(v2, *pendantIt));

#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARCHECK
		if (!(isPlanar(*m_pGraph)))
			std::cout << "connectLabels(): CRITICAL ERROR: inserted edge is not planar!!!" << std::endl;
#endif

		n++;
		++pendantIt;
	}

	updateNewEdges(newEdges);
	deleteLabel(second);

	node newBlock = m_pBCTree->DynamicBCTree::bcproper(newEdges.front());
#ifdef PLANAR_AUGMENTATION_DEBUG
	std::cout
	  << "connectLabels(): newBlock->index() == " << newBlock->index() << ", degree == "
	  << m_pBCTree->m_bNode_degree[newBlock] << std::endl;
#endif

	for (node v : getConnected) {
#if 0
		first->removePendant(v);
#endif
		deletePendant(v);
	}

	if (first->size() != 0){
		m_labels.del(m_isLabel[first->parent()]);
		m_isLabel[m_pBCTree->find(first->parent())] = insertLabel(first);

		for (node v : first->m_pendants) {
			m_belongsTo[m_pBCTree->find(v)] = first;
		}
	}
	else{	// first->size() == 0
		deleteLabel(first);
	}

	if (m_pBCTree->m_bNode_degree[newBlock] == 1){
#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout << "connectLabels(): m_bNode_degree[" << newBlock->index() << "] == 1... calling reduceChain()" << std::endl;
#endif

		m_pendants.pushBack(newBlock);

		if ((m_belongsTo[newBlock] != nullptr) && (m_belongsTo[newBlock]->size() == 1)){
			reduceChain(newBlock, m_belongsTo[newBlock]);
		}
		else{
			reduceChain(newBlock);

			// it can appear that reduceChain() inserts some edges
			// so there are new pendants and obsolete pendants
			//  the obsolete pendants are collected in m_pendantsToDel
			if (m_pendantsToDel.size() > 0){
				ListIterator<node> delIt = m_pendantsToDel.begin();
				for (; delIt.valid(); delIt = m_pendantsToDel.begin()){
					deletePendant(*delIt);
					m_pendantsToDel.del(delIt);
				}
			}
		}
	}
	else{
#ifdef PLANAR_AUGMENTATION_DEBUG
		std::cout << "connectLabels(): newBlock is no new pendant ! degree == " << m_pBCTree->m_bNode_degree[newBlock] << std::endl;
#endif
	}
}



// creates a new label and inserts it into m_labels
pa_label PlanarAugmentation::newLabel(node cutvertex, node pendant, PALabel::StopCause whyStop)
{
	pa_label label = new PALabel(nullptr, cutvertex, whyStop);
	label->addPendant(pendant);
	m_belongsTo[pendant] = label;
	m_isLabel[cutvertex] = m_labels.pushBack(label);
	return label;
}



// trys to find two matching labels
// first will be the label with max. size that has a matching label
bool PlanarAugmentation::findMatching(pa_label& first, pa_label& second)
{
	first = m_labels.front();
	second = nullptr;
	pa_label label = nullptr;

	ListIterator<pa_label> it = m_labels.begin();
	while (it.valid()){
		second = *it;

		if (second != first) {
			if ( (label != nullptr) && (second->size() < label->size()) ){
				second = label;
				return true;
			}

			if (label != nullptr) {

				if ( connectCondition(second, first)
					&& planarityCheck(m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[second->head()]],
					m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[first->head()]]) )
				{
						return true;
				}
			}
			else {	// label == 0

				if ( planarityCheck(m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[second->head()]],
									m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[first->head()]]) ) {
					if (connectCondition(second, first)){
						return true;
					}
					label = second;
				}
			}
		}
		++it;
	}

	if (!label)
		return false;

	second = label;
	return true;
}



// checks the connect-condition for label a and b
bool PlanarAugmentation::connectCondition(pa_label a, pa_label b)
{
	bool found = false;

	if ( (a->isBLabel()) && (b->size() == 1) ){
		found = true;
	}

	int deg1 = m_pBCTree->m_bNode_degree[m_pBCTree->find(a->head())] - b->size() +1;
	int deg2 = m_pBCTree->m_bNode_degree[m_pBCTree->find(b->head())] - b->size() +1;

	if ((deg1 > 2) && (deg2 > 2)){
		return true;
	}
	if ((deg1 > 2) || (deg2 > 2)){
		if (found){
			return true;
		}
		else
			found = true;
	}
	SList<node> *path = m_pBCTree->findPathBCTree(a->head(), b->head());

	for (node v : *path) {
		node bcNode = m_pBCTree->find(v);

		if ((bcNode != a->parent()) && (bcNode != b->parent())){
			if (m_pBCTree->m_bNode_degree[bcNode] > 2){
				if (found) {
					delete path;
					return true;
				} else
					found = true;
			}
			if ((m_pBCTree->typeOfBNode(bcNode) == BCTree::BNodeType::BComp)
				&& (m_pBCTree->m_bNode_degree[bcNode] > 3))
			{
				delete path;
				return true;
			}
		}
	}

	delete path;
	return !found;
}



// update of m_adjNonChildren
// newBlock is the node all nodes on path now belong to
void PlanarAugmentation::updateAdjNonChildren(node newBlock, SList<node>& path)
{
	SListIterator<node> pathIt = path.begin();

	SListIterator<adjEntry> childIt = m_adjNonChildren[newBlock].begin();
	SListIterator<adjEntry> prevIt  = m_adjNonChildren[newBlock].begin();
	// first update m_adjNonChildren[newBlock] by deleting wrong adjEntries
	while (childIt.valid()){
		if (m_pBCTree->find((*childIt)->twinNode()) == newBlock){
			if (childIt == m_adjNonChildren[newBlock].begin()){
				m_adjNonChildren[newBlock].popFront();
				childIt = m_adjNonChildren[newBlock].begin();
				prevIt  = m_adjNonChildren[newBlock].begin();
			}
			else{
				childIt = prevIt;
				m_adjNonChildren[newBlock].delSucc(prevIt);
				++childIt;
			}
		}
		else{
			prevIt = childIt;
			++childIt;
		}
	}

	// now run through list of all path-nodes
	// and update m_adjNonChildren[pathIt] if they do not belong to another bc-node
	// or insert adjEntries to m_adjNonChildren[newBlock]
	while (pathIt.valid()){

		if (*pathIt != newBlock){
			if (*pathIt == m_pBCTree->find(*pathIt)){

				childIt = m_adjNonChildren[*pathIt].begin();
				prevIt  = m_adjNonChildren[*pathIt].begin();

				while (childIt.valid()){
					if (m_pBCTree->find((*childIt)->twinNode()) == (*pathIt)){
						if (childIt == m_adjNonChildren[*pathIt].begin()){
							m_adjNonChildren[*pathIt].popFront();
							childIt = m_adjNonChildren[*pathIt].begin();
							prevIt  = m_adjNonChildren[*pathIt].begin();
						}
						else{
							childIt = prevIt;
							m_adjNonChildren[*pathIt].delSucc(prevIt);
							++childIt;
						}
					}
					else{
						prevIt = childIt;
						++childIt;
					}
				}
			}
			else{	// (*pathIt != m_pBCTree->find(*pathIt))
				childIt = m_adjNonChildren[*pathIt].begin();

				while (childIt.valid()){
					if (m_pBCTree->find((*childIt)->twinNode()) != newBlock){
						// found a child of *pathIt, that has an adjacent bc-node
						//  that doesn't belong to newBlock
							m_adjNonChildren[newBlock].pushBack(*childIt);
					}
					++childIt;
				}
				m_adjNonChildren[*pathIt].clear();
			}
		}
		++pathIt;
	}
}



// modifys the root of the bc-tree
void PlanarAugmentation::modifyBCRoot(node oldRoot, node newRoot)
{
	// status before updates:
	//   m_pBCTree->m_bNode_hRefNode[oldRoot] = 0
	//   m_pBCTree->m_bNode_hParNode[oldRoot] = 0

	//   m_pBCTree->m_bNode_hRefNode[newRoot] = single isolated vertex in b-comp-graph of this c-comp
	//   m_pBCTree->m_bNode_hParNode[newRoot] = cutvertex in b-comp-graph corresponding to rootPendant

	// updates:
	//   for the old root:
	m_pBCTree->m_bNode_hRefNode[oldRoot] = m_pBCTree->m_bNode_hParNode[newRoot];
	m_pBCTree->m_bNode_hParNode[oldRoot] = m_pBCTree->m_bNode_hRefNode[newRoot];

	//   for the new root:
#if 0
	m_pBCTree->m_bNode_hRefNode[newRoot] = no update required;
#endif
	m_pBCTree->m_bNode_hParNode[newRoot] = nullptr;
}



// updates the bc-tree-structure and m_adjNonChildren,
// also adds all edges of newEdges to m_pResult
void PlanarAugmentation::updateNewEdges(const SList<edge> &newEdges)
{
	for (edge e : newEdges) {
		m_pResult->pushBack(e);

		SList<node>& path = m_pBCTree->findPath(e->source(), e->target());

		m_pBCTree->updateInsertedEdge(e);
		node newBlock = m_pBCTree->DynamicBCTree::bcproper(e);

		updateAdjNonChildren(newBlock, path);

		if ((m_pBCTree->parent(newBlock) == nullptr) && (m_pBCTree->m_bNode_degree[newBlock] == 1))
		{
			// the new block is a pendant and also the new root of the bc-tree
			node newRoot = nullptr;
			newRoot = (*(m_adjNonChildren[newBlock].begin()))->twinNode();
			modifyBCRoot(newBlock, newRoot);
		}

		delete &path;
	}
}

// cleanup before finish
void PlanarAugmentation::terminate()
{
	while (m_labels.size() > 0){
		pa_label label = m_labels.popFrontRet();
		delete label;
	}

	m_pendants.clear();
	for(node v : m_pBCTree->m_B.nodes)
		m_adjNonChildren[v].clear();

	delete m_pBCTree;
}

}
