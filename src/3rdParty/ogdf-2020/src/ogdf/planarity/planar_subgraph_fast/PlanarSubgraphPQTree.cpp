/** \file
 * \brief Implementation of the class PlanarSubgraphPQTree.
 *
 * Implements a PQTree with added features for the planarity test.
 * Used by BoothLueker.
 *
 * \author Sebastian Leipert
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

#include <ogdf/planarity/planar_subgraph_fast/PlanarSubgraphPQTree.h>

namespace ogdf {

// Replaces the pertinent subtree by a P-node with leaves as children
// corresponding to the incoming edges of the node v. These edges
// are to be specified by their keys stored in leafKeys.
void PlanarSubgraphPQTree::
ReplaceRoot(SListPure<PlanarLeafKey*> &leafKeys)
{
	if (m_pertinentRoot->status() == PQNodeRoot::PQNodeStatus::Full)
		ReplaceFullRoot(leafKeys);
	else
		ReplacePartialRoot(leafKeys);
}

// Initializes a PQTree by a set of leaves that will korrespond to
// the set of Keys stored in leafKeys.
int PlanarSubgraphPQTree::
Initialize(SListPure<PlanarLeafKey*> &leafKeys)
{
	SListPure<PQLeafKey<edge, whaInfo*, bool>*> castLeafKeys;
	for (PlanarLeafKey *leafPtr : leafKeys)
		castLeafKeys.pushBack(static_cast<PQLeafKey<edge, whaInfo*, bool>*>(leafPtr));

	return PQTree<edge, whaInfo*, bool>::Initialize(castLeafKeys);
}


// Reduction reduced a set of leaves determined by their keys stored
// in leafKeys. Integer redNumber is for debugging only.
bool PlanarSubgraphPQTree::Reduction(
	SListPure<PlanarLeafKey*>     &leafKeys,
	SList<PQLeafKey<edge, whaInfo*, bool>*> &eliminatedKeys)
{
	SListPure<PQLeafKey<edge, whaInfo*, bool>*> castLeafKeys;

	for (PlanarLeafKey *leafPtr : leafKeys)
	{
		castLeafKeys.pushBack(static_cast<PQLeafKey<edge, whaInfo*, bool>*>(leafPtr));
	}

	determineMinRemoveSequence(castLeafKeys, eliminatedKeys);
	removeEliminatedLeaves(eliminatedKeys);

	SListIterator<PQLeafKey<edge,whaInfo*,bool>* >  itn = castLeafKeys.begin();
	SListIterator<PQLeafKey<edge,whaInfo*,bool>* >  itp = itn++;
	for (; itn.valid();)
	{
		if ((*itn)->nodePointer()->status()== PQNodeRoot::PQNodeStatus::WhaDelete)
		{
			++itn;
			castLeafKeys.delSucc(itp);
		}
		else
			itp = itn++;
	}

	if ((*castLeafKeys.begin())->nodePointer()->status() == PQNodeRoot::PQNodeStatus::WhaDelete)
		castLeafKeys.popFront();


	return Reduce(castLeafKeys);
}

// Function ReplaceFullRoot either replaces the full root
// or one full child of a partial root of a pertinent subtree
// by a single P-node  with leaves corresponding the keys stored in leafKeys.
void PlanarSubgraphPQTree::
ReplaceFullRoot(SListPure<PlanarLeafKey*> &leafKeys)
{

	PQInternalNode<edge, whaInfo*, bool>	*nodePtr = nullptr; // dummy
	PQNode<edge, whaInfo*, bool>		    *currentNode = nullptr; // dummy

	if (!leafKeys.empty() && leafKeys.front() == leafKeys.back())
	{
		//ReplaceFullRoot: replace pertinent root by a single leaf
		auto leafPtr = new PQLeaf<edge, whaInfo*, bool>(m_identificationNumber++,
			PQNodeRoot::PQNodeStatus::Empty, (PQLeafKey<edge, whaInfo*, bool>*)leafKeys.front());
		exchangeNodes(m_pertinentRoot, (PQNode<edge, whaInfo*, bool>*) leafPtr);
		if (m_pertinentRoot == m_root)
			m_root = (PQNode<edge, whaInfo*, bool>*) leafPtr;
	}
	else if (!leafKeys.empty()) // at least two leaves
	{
		//replace pertinent root by a $P$-node
		if ((m_pertinentRoot->type() == PQNodeRoot::PQNodeType::PNode) ||
			(m_pertinentRoot->type() == PQNodeRoot::PQNodeType::QNode))
		{
			nodePtr = (PQInternalNode<edge, whaInfo*, bool>*)m_pertinentRoot;
			nodePtr->type(PQNodeRoot::PQNodeType::PNode);
			nodePtr->status(PQNodeRoot::PQNodeStatus::PertRoot);
			nodePtr->childCount(0);
			while (!fullChildren(m_pertinentRoot)->empty())
			{
				currentNode = fullChildren(m_pertinentRoot)->popFrontRet();
				removeChildFromSiblings(currentNode);
			}
		}
		else if (m_pertinentRoot->type() == PQNodeRoot::PQNodeType::Leaf)
		{
			nodePtr = new PQInternalNode<edge, whaInfo*, bool>(m_identificationNumber++,
				PQNodeRoot::PQNodeType::PNode, PQNodeRoot::PQNodeStatus::Empty);
			exchangeNodes(m_pertinentRoot, nodePtr);
		}

		SListPure<PQLeafKey<edge, whaInfo*, bool>*> castLeafKeys;
		for (PlanarLeafKey* leafPtr : leafKeys)
			castLeafKeys.pushBack(static_cast<PQLeafKey<edge, whaInfo*, bool>*>(leafPtr));
		addNewLeavesToTree(nodePtr, castLeafKeys);
	}
}


// Function ReplacePartialRoot replaces all full nodes by a single P-node
// with leaves corresponding the keys stored in leafKeys.
void PlanarSubgraphPQTree::
	ReplacePartialRoot(SListPure<PlanarLeafKey*> &leafKeys)
{
	PQNode<edge,whaInfo*,bool>  *currentNode = nullptr;

	m_pertinentRoot->childCount(m_pertinentRoot->childCount() + 1 -
		fullChildren(m_pertinentRoot)->size());

	while (fullChildren(m_pertinentRoot)->size() > 1)
	{
		currentNode = fullChildren(m_pertinentRoot)->popFrontRet();
		removeChildFromSiblings(currentNode);
	}

	currentNode = fullChildren(m_pertinentRoot)->popFrontRet();

	currentNode->parent(m_pertinentRoot);
	m_pertinentRoot = currentNode;
	ReplaceFullRoot(leafKeys);
}


void PlanarSubgraphPQTree::
removeEliminatedLeaves(SList<PQLeafKey<edge, whaInfo*, bool>*> &eliminatedKeys)
{
	for (PQLeafKey<edge, whaInfo*, bool> *key : eliminatedKeys)
	{
		PQNode<edge, whaInfo*, bool>* nodePtr = key->nodePointer();
		PQNode<edge, whaInfo*, bool>* parent = nodePtr->parent();
		PQNode<edge, whaInfo*, bool>* sibling = nodePtr->getNextSib(nullptr);

		removeNodeFromTree(parent, nodePtr);
		checkIfOnlyChild(sibling, parent);
		if (parent->status() == PQNodeRoot::PQNodeStatus::ToBeDeleted)
		{
			parent->status(PQNodeRoot::PQNodeStatus::WhaDelete);
		}
		nodePtr->status(PQNodeRoot::PQNodeStatus::WhaDelete);
	}
}

}
