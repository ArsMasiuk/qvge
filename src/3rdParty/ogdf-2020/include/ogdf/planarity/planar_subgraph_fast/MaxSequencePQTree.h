/** \file
 * \brief Declaration and definition of the class MaxSequencePQTree.
 *
 *  Derivedsfrom base class PQTree and computes a maximal sequence
 *  of pertinent leaves that can be reduced.
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

#pragma once

#include <string.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/PQTree.h>
#include <ogdf/planarity/planar_subgraph_fast/whaInfo.h>

namespace ogdf {

/**
 * The class template MaxSequencePQTree is designed to compute a maximal consecutive
 * sequence of pertinent leaves in a PQ-tree. In order to achieve this
 * goal, the class template MaxSequencePQTree is a derived class form the class
 * template PQTree. We assume that the reader is familiar
 * with the data structure of a PQ-Tree and therefore give only a very
 * brief overview of the functionality of this data structure.
 *
 * The PQ-Tree is a tool to represent the permutations of a set \a U
 * in which various subsets of \a U occur consecutively. More precisely,
 * the PQ-tree together with a package of efficient algorithms, all
 * included in this template class PQTree, finds permissible
 * permutations on the set \a U. The permissible permutations are those,
 * in which certain subsets \a S of \a U occur as consecutive subsets.
 * A PQ-tree represents an equivalence class of permissible
 * permutations and as the elements of any subset \a S are
 * constraint to appear together, the number of permissible
 * permutations is reduced. The corresponding PQ-tree operation is
 * called reduction with respect to \a S.
 *
 * In case that a set \a S of \a U of pertinent elements is not reducible,
 * it might be a necessary to compute a maximal subset \a S' of \a S
 * deleting the elements of \a S - \a S' from the PQ-tree such that the
 * elements  of \a S' are reducible in the PQ-tree. The class template
 * MaxSequencePQTree provides the user the functionality of computing a subset
 * \a S'. In fact, MaxSequencePQTree depicts the user the elements of \a S - \a S'$ that
 * have to be deleted in order to get a reducible PQ-tree. However,
 * the class template MaxSequencePQTree <b>does not</b> delete the elements of
 * \a S - \a S'. It is up the client using this class to manage the deletion of
 * \a S - \a S'.
 *
 * All computation done by this class to obtain a maximal pertinent
 * sequence is done according to the rules presented in
 * [Jayakumar, Thulasiraman, Swamy, 1989]. For
 * detailed information about the computation, we refer kindly to the
 * authors paper.
 *
 * The declaration of the template class MaxSequencePQTree.
 *
 * The formal parameter \a X of the base class template PQTree was
 * designed to hold the general node information PQNodeKey available
 * at every node of the tree. The class template MaxSequencePQTree defines this
 * parameter to be of type whaInfo. This allows the class template to
 * store informations needed during the computation of a maximal
 * pertinent sequence at every node.
 *
 * @tparam T
 *   the user-defined type of an element in the above mentioned set \a U.
 * @tparam Y
 *   the user-defined type of the information only available for internal nodes PQInternalKey
 */
template<class T,class Y>
class MaxSequencePQTree: public PQTree<T,whaInfo*,Y>
{
public:
	using PQTree<T,whaInfo*,Y>::emptyNode;

	MaxSequencePQTree() : PQTree<T,whaInfo*,Y>() { }

	~MaxSequencePQTree() {
		while (!eliminatedNodes.empty())
		{
			PQNode<T,whaInfo*,Y> *nodePtr = eliminatedNodes.popFrontRet();
			CleanNode(nodePtr);
			delete nodePtr;
		}
	}

	/**
	 * Frees the memory allocated for the node information class of a
	 * node in the PQTree. It is an overloaded function of PQTree and called
	 * before deallocating the memory of the node \p nodePtr (by
	 * #emptyAllPertinentNodes or the destructor).
	 */
	virtual void CleanNode(PQNode<T,whaInfo*,Y>* nodePtr);

	//! Does a clean up of a node. Called by #emptyAllPertinentNodes.
	/**
	 * The function #clientDefinedEmptyNode() is the overloaded virtual function of the
	 * template base class PQTree called by default
	 * by the function #emptyAllPertinentNodes() of PQTree.
	 * The overloaded function handles the different labels used during the
	 * computation and reduction of the maximal pertinent sequence.
	 */
	virtual void clientDefinedEmptyNode(PQNode<T,whaInfo*,Y>* nodePtr);

	//!  Does a clean up after a reduction.
	/**
	 * The function emptyAllPertinentNodes() is the overloaded virtual
	 * function of the template base class PQTree. This function handles all
	 * necessary cleanup after the computation of the maximal pertinent sequence and
	 * the reduction of the maximal pertinent sequence and frees the memory
	 * of all nodes that are no longer in the PQ-tree.
	 *
	 * Most nodes that are regarded for deletion are marked with the status
	 * PQNodeRoot::PQNodeStatus::ToBeDeleted. This causes a valid removal by the function
	 * PQTree<T,whaInfo*,Y>::emptyAllPertinentNodes() if the node is
	 * contained in the stack #m_pertinentNodes of the
	 * base template class. Otherwise, the function
	 * emptyAllPertinentNodes() has to remove the nodes directly from the tree.
	 *
	 * The function emptyAllPertinentNodes() differs the following nodes by
	 * their labels or status.
	 *   - PQNodeRoot::PQNodeStatus::WhaDelete: the node had to be removed from the tree in order
	 *     to get a maximal pertinent sequence. The memory of the nodes has to
	 *     be freed. Hence mark it as PQNodeRoot::PQNodeStatus::ToBeDeleted.
	 *   - PQNodeRoot::PQNodeType::Leaf: the node was a full leaf. Delete it immediately, if it
	 *     was marked PQNodeRoot::PQNodeStatus::WhaDelete, since it is not contained in the
	 *     #m_pertinentNodes stack of the base class.
	 *   - PQNodeRoot::PQNodeStatus::ToBeDeleted: the node was a partial node in the remaining
	 *     pertinent subtree and replaced during the template matching
	 *     algorithm by some other node. For the computation of valid parent
	 *     pointers during the Bubble phase, the node has to be kept. Hence it
	 *     is marked as PQNodeRoot::PQNodeStatus::Eliminated, which leaves the node untouched by the
	 *     base class call of emptyAllPertinentNodes(). Observe that the node
	 *     is stored in a special stack called #eliminatedNodes for a valid
	 *     cleanup after the termination of the algorithm.
	 *   - PQNodeRoot::PQNodeStatus::Full: the node is a full node of the pertinent subtree. Since
	 *     the pertinent subtree is replaced by a single P-node and some
	 *     leaves representing the incoming edges of a node, this node has to
	 *     be removed from the tree. Hence it is marked as PQNodeRoot::PQNodeStatus::ToBeDeleted.
	 *     Observe that the pertinent root was probably marked as PQNodeRoot::PQNodeStatus::PertRoot
	 *     and therefore is not removed from the tree.
	 *
	 * This function handles another important
	 * fact. During the reduction of the maximal pertinent sequence,
	 * PQNodeRoot::PQNodeStatus::Partial nodes have been introduced
	 * into the PQ-tree, that do not have a node information.
	 * This function detects these nodes equipping them with the
	 * container class of type PQNodeKey.
	 */
	virtual void emptyAllPertinentNodes();

	/**
	 * Computes the maximal pertinent sequence
	 * \a S' of elements of the set \a S, that can be reduced in a PQ-tree. The
	 * function expects the set \a S stored in an SListPure<PQLeafKey*> called
	 * \p leafKeys. Since the elements of \a S - \a S' have to be removed from
	 * the PQ-tree by the client, the function determineMinRemoveSequence() returns
	 * the elements of \a S - \a S' in an array of type PQLeafKey** called
	 * \a EliminatedElements. The return value of the function is |\a S - \a S'|.
	 *
	 * In order to compute the maximal pertinent sequence the function
	 * determineMinRemoveSequence() computes the [w,h,a]-number of every pertinent
	 * node in the pertinent subtree of the PQ-tree. If the minimum
	 * of the h- and a-number of the root of the pertinent
	 * subtree is not 0, then the PQ-tree is not reducible.
	 * According to the [w,h,a]-numbering, this procedure computes a minimal number of
	 * pertinent leaves that have to be removed from of the PQ-tree to gain reducibility.
	 *
	 * The user should observe that removing the leaves from the PQ-tree,
	 * depicted by the pointers to their information class stored in
	 * \a EliminatedElements is a necessary but not sufficient action to
	 * gain reducibility. The client calling this function has to make sure
	 * that nodes, where the complete frontier has been removed during the
	 * process must be removed as well. This can easily be done using functions of
	 * the base class PQTree such as checkIfOnlyChild().
	 */
	int determineMinRemoveSequence(
		SListPure<PQLeafKey<T,whaInfo*,Y>*> &leafKeys,
		SList<PQLeafKey<T,whaInfo*,Y>*>     &eliminatedKeys);

protected:
	using PQTree<T,whaInfo*,Y>::fullChildren;
	using PQTree<T,whaInfo*,Y>::partialChildren;

	/**
	 * The function Bubble() is an overloaded function of the base
	 * template class PQTree. This overloaded version
	 * of Bubble() covers several tasks.
	 *   -# It bubbles the tree up from the pertinent leaves, stored in an
	 *      SListPure of type PQLeafKey, to find all
	 *      pertinent nodes. Every pertinent node is stored in the stack
	 *      #cleanUp for a valid cleanup after the reduction step.
	 *   -# It makes sure that every pertinent node has a valid parent
	 *      pointer. (Different to the original Bubble() that only tests
	 *      if every node has a valid parent pointer)
	 *
	 * @param leafKeys a SListPure to the PQLeafKey's of the pertinent leaves.
	 */
	virtual bool Bubble(SListPure<PQLeafKey<T,whaInfo*,Y>*> &leafKeys);

	/**
	 * Computes for the node \p nodePtr its valid parent in the PQ-tree.
	 * The parent pointer is needed during the #Bubble() phase.
	 *
	 * In case that \p nodePtr does not have a valid pointer to its parent,
	 * it points to a node that is not contained in the tree anymore. Since we
	 * do not free the memory of such nodes, using the parent pointer of \p nodePtr
	 * does not cause runtime errors. The previous parent of \p nodePtr itself is
	 * marked as PQNodeRoot::PQNodeStatus::Eliminated, denoting a node that has
	 * been removed from the tree. Since such a \p nodePtr with a non-valid parent
	 * pointer can only appear somewhere between the children of a Q-node, the function
	 * GetParent() sweeps through the siblings of \p nodePtr to get a
	 * valid parent pointer from the endmost child, thereby updating the
	 * parent pointers of all the
	 * siblings between the endmost child and \p nodePtr. Since the number
	 * of children of Q-nodes corresponds to the number of cutvertices in
	 * the bushform, the total number of children updated by GetParent() is
	 * in O(n) for every call of #Bubble(). Hence the complexity of the
	 * update procedure is bounded by O(n^2).
	 */
	PQNode<T,whaInfo*,Y>* GetParent(PQNode<T,whaInfo*,Y>* nodePtr);

	/**
	 * Used to store all pertinent Nodes of the pertinent subtree before
	 * removing the minimal pertinent subsequence.
	 * Necessary for updates and cleanups after a reduction on the maximal
	 * pertinent sequence was successful.
	 */
	SListPure<PQNode<T,whaInfo*,Y>*> cleanUp;

	/**
	 * Used to store all eliminated nodes (\a status == PQNodeRoot::PQNodeStatus::Eliminated) of the PQTree.
	 * An  eliminated node is one that has been removed during the application
	 * of the template matching algorithm from the PQ-tree. These nodes
	 * are kept (and their memory is not freed) in order to find out
	 * if a node has a valid parent pointer.
	 */
	SListPure<PQNode<T,whaInfo*,Y>*> eliminatedNodes;

private:
	/**
	 * Checks the [w,h,a]-number of the pertinent root. In case that the
	 * min{\a a,\a h} = 0, where \a a, \a h belong to the pertinent root, the
	 * PQ-tree is reducible and nothing needs to be done. In case that
	 * min{\a a,\a h} > 0, a min{\a a,\a h} number of leaves have to be removed
	 * from the tree in order to achieve reducibility for the set \a S.
	 *
	 * Knowing precisely the [w,h,a]-number of every pertinent node, this
	 * can be achieved in a top down manner, according to the rules presented
	 * in Jayakumar et al. The procedure findMinWHASequence() implements this using
	 * the stack of nodes called \p archiv. This archiv contains all
	 * pertinent nodes. Since parents have been stored on top of their
	 * children in the stack which supports the top down method of the
	 * procedure.
	 *
	 * The procedure findMinWHASequence() is called by the procedure
	 * determineMinRemoveSequence().
	 *
	 * Returns an SList \p eliminatedKeys
	 * of pointers to PQLeafKey, describing the leaves that have to be removed.
	 */
	void findMinWHASequence(
		ArrayBuffer<PQNode<T,whaInfo*,Y>*> &archiv,
		SList<PQLeafKey<T,whaInfo*,Y>*>  &eliminatedKeys);

	/**
	 * Processes the children of a Q-node,
	 * marking a full sequence of children with at most incident partial child on
	 * one side of the Q-node, as b-nodes respectively as h-node. The
	 * pointer \p hChild1 depicts the endmost child of the Q-node, where
	 * the sequence starts.
	 *
	 * @param hChild1 of the Q-node
	 * @return number of pertinent children, corresponding to the [w,h,a]-numbering.
	 */
	int setHchild(PQNode<T,whaInfo*,Y> *hChild1);

	/**
	 * Traces all children of the largest
	 * consecutive sequence of pertinent children of a Q-node. Notice, that
	 * it does not mark a single node as a-node, but a sequence of full
	 * children with possible a partial child at each end as b-nodes,
	 * respectively as h-nodes.
	 *
	 * @param hChild2 the first node of the sequence
	 * @param hChild2Sib its pertinent sibling
	 *    It allows immediate scanning of the sequence.
	 *
	 * @return the number of pertinent children of the Q-node according to the [w,h,a]-numbering.
	 */
	int setAchildren(
		PQNode<T,whaInfo*,Y> *hChild2,
		PQNode<T,whaInfo*,Y> *hChild2Sib);

	/**
	 * Marks all full and/or partial children
	 * of \p nodePtr as either an a-, b-, h- or w-node.
	 *
	 * @param nodePtr a pointer to the node
	 * @param label
	 *   Describes which children have to be marked.
	 *   Can be either PQNodeRoot::PQNodeStatus::Full,
	 *   PQNodeRoot::PQNodeStatus::Partial or PQNodeRoot::PQNodeStatus::Pertinent.
	 * @param deleteType
	 *   Can be either whaType::W, whaType::B, whaType::H or whaType::A
	 */
	void markPertinentChildren(
		PQNode<T,whaInfo*,Y> *nodePtr,
		PQNodeRoot::PQNodeStatus label,
		whaType deleteType);

	//! Computes the h- and a-number of a P-node \p nodePtr.
	void haNumPnode(PQNode<T,whaInfo*,Y> *nodePtr);

	/**
	 * Computes the h- and a-number of the partial Q-node \p nodePtr.
	 * Furthermore sets the children \a aChild, \a hChild1 and \a hChild2
	 * of the node information class whaInfo* of \p nodePtr.
	 */
	void haNumQnode(PQNode<T,whaInfo*,Y> *nodePtr);

	/**
	 * Computes the a-number of the partial Q-node \p nodePtr.
	 * Furthermore sets the children \a aChild, \a hChild1 and \a hChild2 of the node
	 * information class whaInfo* of \p nodePtr.
	 *
	 * It Checks for consecutive sequences between all children of the Q-node
	 * \p nodePtr. The children which form a consecutive sequence are stored
	 * in a stack called \a sequence that is emptied, as soon as the end of
	 * the sequence is reached. When the stack is emptied, we count for the
	 * pertinent leaves in the front of the sequence, and update if necessary
	 * the sequence holding the maximum number of pertinent leaves in its
	 * frontier.
	 *
	 * Observe that if the sequence ends with a partial node, this node may
	 * form  a consecutive sequence with its other siblings. Hence the
	 * partial node is pushed back onto the stack \a sequence after the
	 * stack has been emptied.
	 */
	void aNumQnode(PQNode<T,whaInfo*,Y> *nodePtr, int sumAllW);

	/**
	 * Computes the h-number of the partial Q-node \p nodePtr.
	 * Furthermore sets the child \a hChild1 of the node information
	 * class whaInfo* of \p nodePtr.
	 */
	void hNumQnode(PQNode<T,whaInfo*,Y> *nodePtr, int sumAllW);

	/**
	 * Returns
	 * alpha_1 = beta_1 = sum_{i in P(\p nodePtr)} w_i - max_{i in P(\p nodePtr)}{(w_i = a_i)},
	 * where P(\p nodePtr) denotes the set of all pertinent
	 * children of the node \p nodePtr regardless whether \p nodePtr is a
	 * P- or a Q-node. Depicts the a-number if just one
	 * child of \p nodePtr is made a-node. This child is returned by the function
	 * alpha1beta1Number() using the pointer \p aChild.
	 */
	int alpha1beta1Number(
		PQNode<T,whaInfo*,Y> *nodePtr,
		PQNode<T,whaInfo*,Y> **aChild);

	/**
	 * Returns \a w = sum_{i in P(\p nodePtr)}w_i, where \p nodePtr
	 * is any pertinent node of the PQ-tree.
	 */
	int sumPertChild(PQNode<T,whaInfo*,Y> *nodePtr);
};

template<class T,class Y>
bool MaxSequencePQTree<T,Y>::Bubble(SListPure<PQLeafKey<T,whaInfo*,Y>*> &leafKeys)
{
	// Queue for storing all pertinent nodes that still have
	// to be processed.
	Queue<PQNode<T,whaInfo*,Y>*>     processNodes;

	/*
	Enter the [[Full]] leaves into the queue [[processNodes]].
	In a first step the pertinent leaves have to be identified in the tree
	and entered on to the queue [[processNodes]]. The identification of
	the leaves can be done with the help of a pointer stored in every
	[[PQLeafKey]] (see \ref{PQLeafKey}) in constant time for every element.
	*/
	SListIterator<PQLeafKey<T,whaInfo*,Y>*> it;
	for (it  = leafKeys.begin(); it.valid(); ++it)
	{
		PQNode<T,whaInfo*,Y>* checkLeaf = (*it)->nodePointer();
		processNodes.append(checkLeaf);
		cleanUp.pushBack(checkLeaf);
		if (!checkLeaf->getNodeInfo()) { // if leaf does not have an information class for storing the [wha]-number allocate one.
			whaInfo *newInfo = new whaInfo;
			PQNodeKey<T,whaInfo*,Y> *infoPtr = new PQNodeKey<T,whaInfo*,Y>(newInfo);
			checkLeaf->setNodeInfo(infoPtr);
			infoPtr->setNodePointer(checkLeaf);
		}
		checkLeaf->getNodeInfo()->userStructInfo()->m_notVisitedCount = 1;
		checkLeaf->mark(PQNodeRoot::PQNodeMark::Queued);
	}

	/*
	For every node in [[processNodes]], its father is detected using the
	function [[GetParent]] \ref{GetParent}. The father is placed onto the
	queue as well, if [[_nodePtr]] is its first child, that is popped from
	the queue. In that case, the father is marked as [[Queued]] to
	prevent the node from queue more than once. In any case, the number
	of pertinent children of the father is updated. This is an important
	number for computing the maximal pertinent sequence.
	*/
	while (!processNodes.empty())
	{
		PQNode<T,whaInfo*,Y>* nodePtr = processNodes.pop();
		nodePtr->parent(GetParent(nodePtr));
		if (nodePtr->parent() &&
			!nodePtr->parent()->getNodeInfo())
				// if the parent does not have an information
				// class for storing the [wha]-number
				// allocate one.
		{
			whaInfo *newInfo = new whaInfo;
			PQNodeKey<T,whaInfo*,Y> *infoPtr = new PQNodeKey<T,whaInfo*,Y>(newInfo);
			nodePtr->parent()->setNodeInfo(infoPtr);
			infoPtr->setNodePointer(nodePtr->parent());
		}
		if (nodePtr != this->m_root)
		{
			if (nodePtr->parent()->mark() == PQNodeRoot::PQNodeMark::Unmarked)
			{
				processNodes.append(nodePtr->parent());
				cleanUp.pushBack(nodePtr->parent());
				nodePtr->parent()->mark(PQNodeRoot::PQNodeMark::Queued);
			}
			nodePtr->parent()->getNodeInfo()->userStructInfo()->m_notVisitedCount++;
			int childCount = nodePtr->parent()->pertChildCount();
			nodePtr->parent()->pertChildCount(++childCount);
		}
	}


	/*
	This chunk belongs to the function [[Bubble]]. After doing some
	cleaning up and resetting the flags that have been left at the
	pertinent nodes during the first bubble up, this chunk computes the
	maximal pertinent sequence by calling the function [[determineMinRemoveSequence]].
	It then removes
	all leaves from the tree that have been marked as [[Eliminated]] and
	possibly some internal nodes of the $PQ$-tree and stores pointers of
	type [[leafKey]] for the pertinent leaves in the maximal sequence in the
	array [[survivedElements]].
	*/

	SListIterator<PQNode<T,whaInfo*,Y>*> itn;
	for (itn = cleanUp.begin(); itn.valid(); ++itn)
		(*itn)->mark(PQNodeRoot::PQNodeMark::Unmarked);

	return true;
}

template<class T,class Y>
void MaxSequencePQTree<T,Y>::CleanNode(PQNode<T,whaInfo*,Y>* nodePtr)
{
	if (nodePtr->getNodeInfo())
	{
		delete nodePtr->getNodeInfo()->userStructInfo();
		delete nodePtr->getNodeInfo();
	}
}

template<class T,class Y>
void MaxSequencePQTree<T,Y>::clientDefinedEmptyNode(PQNode<T,whaInfo*,Y>* nodePtr)
{
	if (nodePtr->status() == PQNodeRoot::PQNodeStatus::Eliminated)
	{
		emptyNode(nodePtr);
		nodePtr->status(PQNodeRoot::PQNodeStatus::Eliminated);
	}
	else if (nodePtr->status() == PQNodeRoot::PQNodeStatus::PertRoot)
		emptyNode(nodePtr);
	else {
		// Node has an invalid status?
		OGDF_ASSERT(nodePtr->status() == PQNodeRoot::PQNodeStatus::Empty);
		emptyNode(nodePtr);
	}
}

template<class T,class Y>
void MaxSequencePQTree<T,Y>::emptyAllPertinentNodes()
{
	PQNode<T,whaInfo*,Y>* nodePtr = nullptr;

	while (!cleanUp.empty())
	{
		nodePtr = cleanUp.popFrontRet();
		nodePtr->pertChildCount(0);
		if (nodePtr->status() == PQNodeRoot::PQNodeStatus::WhaDelete && nodePtr->type() == PQNodeRoot::PQNodeType::Leaf)
		{
			CleanNode(nodePtr);
			delete nodePtr;
		}

		else {
			// node must have an Information if contained in cleanUp
			OGDF_ASSERT(nodePtr->getNodeInfo() != nullptr);

			nodePtr->getNodeInfo()->userStructInfo()->m_notVisitedCount = 0;
			nodePtr->getNodeInfo()->userStructInfo()->m_pertLeafCount = 0;
		}
	}

	ListIterator<PQNode<T,whaInfo*,Y>*> it;
	for (it = this->m_pertinentNodes->begin();it.valid();++it)
	{
		nodePtr = *it;
		if (nodePtr->status() == PQNodeRoot::PQNodeStatus::ToBeDeleted)
		{
			nodePtr->status(PQNodeRoot::PQNodeStatus::Eliminated);
			eliminatedNodes.pushBack(nodePtr);
		}
		else if (nodePtr->status() == PQNodeRoot::PQNodeStatus::Full)
			nodePtr->status(PQNodeRoot::PQNodeStatus::ToBeDeleted);
		else if (nodePtr->status() == PQNodeRoot::PQNodeStatus::WhaDelete)
			nodePtr->status(PQNodeRoot::PQNodeStatus::ToBeDeleted);
		else if (nodePtr->getNodeInfo())
			nodePtr->getNodeInfo()->userStructInfo()->defaultValues();
	}
	PQTree<T,whaInfo*,Y>::emptyAllPertinentNodes();
}

template<class T,class Y>
int MaxSequencePQTree<T,Y>::determineMinRemoveSequence(
	SListPure<PQLeafKey<T,whaInfo*,Y>*> &leafKeys,
	SList<PQLeafKey<T,whaInfo*,Y>*> &eliminatedKeys)
{
	PQNode<T,whaInfo*,Y>				*nodePtr            = nullptr; // dummy

	//Number of leaves that have to be deleted
	int									countDeletedLeaves	= 0;
	//Number of pertinent leaves
	int									maxPertLeafCount	= 0;

	// A queue storing the nodes whose $[w,h,a]$-number has to be computed next.
	// A node is stored in [[processNodes]], if for all of its children the
	// [w,h,a]-number has been computed.
	Queue<PQNode<T,whaInfo*,Y>*>		processNodes;

	// A stack storing all nodes where a $[w,h,a]$-number
	// has been computed. This is necessary for a valid cleanup.
	ArrayBuffer<PQNode<T,whaInfo*,Y>*>	archiv;

	// Compute a valid parent pointer for every pertinent node.
	Bubble(leafKeys);

	// Get all pertinent leaves and stores them on [[processNodes]]
	// and [[_archiv]].
	SListIterator<PQLeafKey<T,whaInfo*,Y>*> it;
	for (it = leafKeys.begin(); it.valid(); ++it)
	{
		PQNode<T, whaInfo*, Y> *checkLeaf = (*it)->nodePointer();
		checkLeaf->getNodeInfo()->userStructInfo()->m_pertLeafCount = 1;
		checkLeaf->getNodeInfo()->userStructInfo()->m_notVisitedCount--;
		processNodes.append(checkLeaf);
		archiv.push(checkLeaf);

		maxPertLeafCount++;
	}

	while (!processNodes.empty())
	{
		nodePtr = processNodes.pop();
		// Compute the $[w,h,a]$ number of [[nodePtr]]. Computing this number is
		// trivial for leaves and full nodes. When considering a partial node, the
		// computation has to distinguish between $P$- and $Q$-nodes.
		if (nodePtr->getNodeInfo()->userStructInfo()->m_pertLeafCount < maxPertLeafCount)
		{
			// In case that the actual node, depicted by the pointer
			// [[nodePtr]] is not the root, the counts of the pertinent
			// children of [[nodePtr]]'s parent are
			// updated. In case that all pertinent children of the parent of
			// [[nodePtr]] have a valid $[w,h,a]$-number, it is possible to compute
			// the $[w,h,a]$-number of parent. Hence the parent is placed onto
			// [[processNodes]]and [[_archiv]].
			nodePtr->parent()->getNodeInfo()->userStructInfo()->m_pertLeafCount =
				nodePtr->parent()->getNodeInfo()->userStructInfo()->m_pertLeafCount
				+ nodePtr->getNodeInfo()->userStructInfo()->m_pertLeafCount;
			nodePtr->parent()->getNodeInfo()->userStructInfo()->m_notVisitedCount--;
			if (!nodePtr->parent()->getNodeInfo()->userStructInfo()->m_notVisitedCount)
			{
				processNodes.append(nodePtr->parent());
				archiv.push(nodePtr->parent());
			}
		}
		if (nodePtr->type() == PQNodeRoot::PQNodeType::Leaf)
		{
			// Compute the $[w,h,a]$-number of a leaf. The computation is trivial.
			nodePtr->status(PQNodeRoot::PQNodeStatus::Full);
			nodePtr->getNodeInfo()->userStructInfo()->m_w = 1;
			nodePtr->getNodeInfo()->userStructInfo()->m_h = 0;
			nodePtr->getNodeInfo()->userStructInfo()->m_a = 0;
			if (nodePtr->getNodeInfo()->userStructInfo()->m_pertLeafCount < maxPertLeafCount)
				fullChildren(nodePtr->parent())->pushFront(nodePtr);
		}
		else
		{
			// [[nodePtr]] is a $P$- or $Q$-node
			// The $[w,h,a]$ number of $P$- or $Q$-nodes is computed identically.
			// This is done calling the function [[sumPertChild]].
			nodePtr->getNodeInfo()->userStructInfo()->m_w = sumPertChild(nodePtr);

			if (fullChildren(nodePtr)->size() == nodePtr->childCount())
			{
				// computes the $h$- and $a$-numbers of a full node. The computation
				// is trivial. It also updates the list of full nodes of the parent
				// of [[nodePtr]].
				nodePtr->status(PQNodeRoot::PQNodeStatus::Full);
				if (nodePtr->getNodeInfo()->userStructInfo()->m_pertLeafCount
					< maxPertLeafCount)
					fullChildren(nodePtr->parent())->pushFront(nodePtr);
				nodePtr->getNodeInfo()->userStructInfo()->m_h = 0;
				nodePtr->getNodeInfo()->userStructInfo()->m_a = 0;
			}
			else
			{
				// computes the $[w,h,a]$-number of a partial node. The computation is
				// nontrivial for both $P$- and $Q$-nodes and is performed in the
				// function [[haNumPnode]] for $P$-nodes and in the
				// function [[haNumQnode]] for $Q$-nodes.
				// This chunk also updates the partial children stack of the parent.
				nodePtr->status(PQNodeRoot::PQNodeStatus::Partial);
				if (nodePtr->getNodeInfo()->userStructInfo()->m_pertLeafCount <
					maxPertLeafCount)
					partialChildren(nodePtr->parent())->pushFront(nodePtr);

				if (nodePtr->type() == PQNodeRoot::PQNodeType::PNode)
					haNumPnode(nodePtr);
				else
					haNumQnode(nodePtr);
			}
		}
	}

	// Determine the root of the pertinent subtree, which the last processed node
	//[[nodePtr]] and finds the minimum of the $h$- and $a$-number of
	//[[m_pertinentRoot]]. In case that the minimum is equal to $0$, the
	//[[m_pertinentRoot]] stays of type $B$. Otherwise the type is selected
	//according to the minimum.
	this->m_pertinentRoot = nodePtr;
	if (this->m_pertinentRoot->getNodeInfo()->userStructInfo()->m_h <
		this->m_pertinentRoot->getNodeInfo()->userStructInfo()->m_a)
		countDeletedLeaves = this->m_pertinentRoot->getNodeInfo()->userStructInfo()->m_h;
	else
		countDeletedLeaves = this->m_pertinentRoot->getNodeInfo()->userStructInfo()->m_a;

	if (countDeletedLeaves > 0)
	{
		if (this->m_pertinentRoot->getNodeInfo()->userStructInfo()->m_h <
			this->m_pertinentRoot->getNodeInfo()->userStructInfo()->m_a)
			this->m_pertinentRoot->getNodeInfo()->userStructInfo()->m_deleteType = whaType::H;
		else
			this->m_pertinentRoot->getNodeInfo()->userStructInfo()->m_deleteType = whaType::A;
	}

	findMinWHASequence(archiv,eliminatedKeys);

	return countDeletedLeaves;
}

template<class T, class Y>
void MaxSequencePQTree<T, Y>::findMinWHASequence(
	ArrayBuffer<PQNode<T, whaInfo*, Y>*> &archiv,
	SList<PQLeafKey<T, whaInfo*, Y>*> &eliminatedKeys)
{
	//a pointer to the first child of type $h$ of [[nodePtr]].
	PQNode<T, whaInfo*, Y>   *hChild1 = nullptr;

	//a pointer to the second child of type $h$ of [[nodePtr]].
	PQNode<T, whaInfo*, Y>   *hChild2 = nullptr;

	//a pointer to the child of type $a$ of [[nodePtr]].
	PQNode<T, whaInfo*, Y>   *aChild = nullptr;

	// pertinent sibling of hChild1
	PQNode<T, whaInfo*, Y>   *hChild2Sib = nullptr;

	while (!archiv.empty())
	{
		//counts the number of children of [[nodePtr]].
		int childCount = 0;

		//a pointer to a pertinent node of the $PQ$-tree that is currently examined.
		PQNode<T, whaInfo*, Y> *nodePtr = archiv.popRet();

		/*
		Check if [[nodePtr]] is a full node whose delete type is either of
		type $h$ or type $a$. Since there are no empty leaves in its
		frontier, the node must therefore keep all its pertinent leaves
		in its frontier and is depicted to be of type $b$.
		*/
		if (nodePtr->status() == PQNodeRoot::PQNodeStatus::Full &&
			(nodePtr->getNodeInfo()->userStructInfo()->m_deleteType == whaType::H ||
			nodePtr->getNodeInfo()->userStructInfo()->m_deleteType == whaType::A))
		{
			nodePtr->getNodeInfo()->userStructInfo()->m_deleteType = whaType::B;
			this->m_pertinentNodes->pushFront(nodePtr);
		}

		/*
		Check if [[nodePtr]] is a leaf whose delete type is either of type $w$ or
		$b$. In case it is of type $w$, the leaf has to be removed from the
		tree to gain reducibility of the set $S$.
		*/
		else if (nodePtr->type() == PQNodeRoot::PQNodeType::Leaf)
		{
			if (nodePtr->getNodeInfo()->userStructInfo()->m_deleteType == whaType::W)
				eliminatedKeys.pushBack(nodePtr->getKey());
			else
				this->m_pertinentNodes->pushFront(nodePtr);
		}

		/*
		Manage the case of [[nodePtr]] being either a partial $P$-node, a partial
		$Q$-node, or a full $P$- or $Q$-node, where in the latter case the
		delete type of [[nodePtr]] is of type $b$.
		*/
		else
		switch (nodePtr->getNodeInfo()->userStructInfo()->m_deleteType)
		{
			case whaType::B:
				this->m_pertinentNodes->pushFront(nodePtr);
				break;

			case whaType::W:
				markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Pertinent, whaType::W);
				nodePtr->pertChildCount(0);
				this->m_pertinentNodes->pushFront(nodePtr);
				break;

			case whaType::H:
				if (nodePtr->type() == PQNodeRoot::PQNodeType::PNode)
				{
					/*
					[[nodePtr]] is a $P$-node of type $h$. Mark all full
					children of [[nodePtr]] to be of type $b$ (by doing nothing, since
					the default is type $b$). It furthermore marks all partial children to
					be of type $w$ except for the child stored in [[hChild1]] of the
					information class of type [[whaInfo*]] of [[nodePtr]]. This child is
					marked to be of type $h$.
					*/
					markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Partial, whaType::W);
					markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Full, whaType::B);
					if (nodePtr->getNodeInfo()->userStructInfo()->m_hChild1)
					{
						hChild1 = (PQNode<T, whaInfo*, Y>*)
							nodePtr->getNodeInfo()->userStructInfo()->m_hChild1;
						hChild1->getNodeInfo()->userStructInfo()->m_deleteType = whaType::H;
						if (hChild1->getNodeInfo()->userStructInfo()->m_h <
							hChild1->getNodeInfo()->userStructInfo()->m_w)
							childCount = 1;
					}
					nodePtr->pertChildCount(nodePtr->pertChildCount() +
						childCount - partialChildren(nodePtr)->size());
				}
				else
				{
					/*
					[[nodePtr]] is a $Q$-node. Mark all pertinent children
					to be of type $w$, except for the full children between the
					[[hChild1]] and the endmost child of [[nodePtr]]. These full
					children are marked $b$ while [[hChild1]] is marked to be of type
					$h$. Setting the type of children to $b$ or $h$ is hidden in the
					function call [[setHchild]] \label{setHChild}.
					*/
					markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Pertinent, whaType::W);
					hChild1 = (PQNode<T, whaInfo*, Y>*)
						nodePtr->getNodeInfo()->userStructInfo()->m_hChild1;
					nodePtr->pertChildCount(setHchild(hChild1));
				}
				this->m_pertinentNodes->pushFront(nodePtr);
				break;

			case whaType::A:
				if (nodePtr->type() == PQNodeRoot::PQNodeType::PNode)
				{
					/*
					[[nodePtr]] is a $P$-node of type $a$.
					Distinguish two main cases, based on the existence of a
					child of [[nodePtr]] stored in [[aChild]] of the information
					class of type [[_whaInfo]] of [[nodePtr]].
					\begin{enumerate}
					\item
					If [[aChild]] is not empty, the chunk marks all full
					and partial children of [[nodePtr]] to be of type $w$ and
					marks the child [[aChild]] to be of type $a$.
					\item
					If [[aChild]] is empty, the chunk
					marks all full children of [[nodePtr]] to be of type $b$
					(by doing nothing, since the default is type $b$).
					It furthermore marks all partial children to be of type $w$
					except for the children stored in [[hChild1]] and
					[[hChild2]] of the information class of type [[whaInfo*]] of
					[[nodePtr]] which are marked to be of type $h$. Observe that
					we have to distinguish the cases where both, [[_hChild]] and
					[[hChild2]] are available, just [[_h_Child1]] is available or
					none of the nodes exist.
					\end{enumerate}
					*/
					if (nodePtr->getNodeInfo()->userStructInfo()->m_aChild)
					{
						markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Pertinent, whaType::W);
						aChild = (PQNode<T, whaInfo*, Y>*)
							nodePtr->getNodeInfo()->userStructInfo()->m_aChild;
						aChild->getNodeInfo()->userStructInfo()->m_deleteType = whaType::A;
						nodePtr->pertChildCount(1);
					}
					else
					{
						markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Full, whaType::B);
						markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Partial, whaType::W);
						if (nodePtr->getNodeInfo()->userStructInfo()->m_hChild1)
						{
							hChild1 = (PQNode<T, whaInfo*, Y>*)
								nodePtr->getNodeInfo()->userStructInfo()->m_hChild1;
							hChild1->getNodeInfo()->userStructInfo()->m_deleteType = whaType::H;
							if (hChild1->getNodeInfo()->userStructInfo()->m_h <
								hChild1->getNodeInfo()->userStructInfo()->m_w)
								childCount = 1;
						}
						if (nodePtr->getNodeInfo()->userStructInfo()->m_hChild2)
						{
							hChild2 = (PQNode<T, whaInfo*, Y>*)
								nodePtr->getNodeInfo()->userStructInfo()->m_hChild2;
							hChild2->getNodeInfo()->userStructInfo()->m_deleteType = whaType::H;
							if (hChild2->getNodeInfo()->userStructInfo()->m_h <
								hChild2->getNodeInfo()->userStructInfo()->m_w)
								childCount++;
						}
						nodePtr->pertChildCount(nodePtr->pertChildCount() + childCount -
							partialChildren(nodePtr)->size());
					}
				}
				else
				{
					/*
					[[nodePtr]] is a $Q$-node of type $a$.
					Distinguish two main cases, based on the existence of a child of
					[[nodePtr]] stored in [[aChild]] of the information class of type
					[[_whaInfo]] of [[nodePtr]].
					\begin{enumerate}
					\item
					If [[aChild]] is not empty, the chunk marks all full
					and partial children of [[nodePtr]] to be of type $w$ and marks the
					child [[aChild]] to be of type $a$.
					\item
					If [[aChild]] is empty, the chunk
					marks all full and partial children of [[nodePtr]] to be of type $w$
					except for the ones in the largest consecutive sequence of pertinent
					children. This sequence
					is depicted by the children [[hChild1]] and [[hChild2]] which may be
					either full or partial. The children between [[hChild1]] and
					[[hChild2]] are full and are marked $b$, while [[hChild1]] and
					[[hChild2]] are marked $b$ or $h$, according to their status.
					Setting the type of the nodes is hidden in calling the function
					[[setAchildren]] (see \ref{setAchildren}).
					\end{enumerate}
					*/
					if (nodePtr->getNodeInfo()->userStructInfo()->m_aChild)
					{
						aChild = (PQNode<T, whaInfo*, Y>*)
							nodePtr->getNodeInfo()->userStructInfo()->m_aChild;
						markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Pertinent, whaType::W);
						aChild->getNodeInfo()->userStructInfo()->m_deleteType = whaType::A;
						nodePtr->pertChildCount(1);
					}
					else
					{
						markPertinentChildren(nodePtr, PQNodeRoot::PQNodeStatus::Pertinent, whaType::W);
						hChild2 = (PQNode<T, whaInfo*, Y>*)
							nodePtr->getNodeInfo()->userStructInfo()->m_hChild2;
						hChild2Sib = (PQNode<T, whaInfo*, Y>*)
							nodePtr->getNodeInfo()->userStructInfo()->m_hChild2Sib;
						nodePtr->pertChildCount(setAchildren(hChild2, hChild2Sib));
					}
				}
				this->m_pertinentNodes->pushFront(nodePtr);
				break;
		}

		/*
		After successfully determining the type of the children of [[nodePtr]],
		this chunk cleans up the information needed during the
		computation at the [[nodePtr]].
		*/
		fullChildren(nodePtr)->clear();
		partialChildren(nodePtr)->clear();
		nodePtr->status(PQNodeRoot::PQNodeStatus::Empty);
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild1 = nullptr;
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild2 = nullptr;
		nodePtr->getNodeInfo()->userStructInfo()->m_aChild = nullptr;
		nodePtr->getNodeInfo()->userStructInfo()->m_w = 0;
		nodePtr->getNodeInfo()->userStructInfo()->m_h = 0;
		nodePtr->getNodeInfo()->userStructInfo()->m_a = 0;
		nodePtr->getNodeInfo()->userStructInfo()->m_deleteType = whaType::B;
	}
}

template<class T, class Y>
int MaxSequencePQTree<T, Y>::setHchild(PQNode<T, whaInfo*, Y> *hChild1)
{
	// counts the number of pertinent children corresponding to the $[w,h,a]$-numbering.
	int						pertinentChildCount = 0;

	// is [[true]] as long as children with a full label are found, [[false]] otherwise.
	bool					fullLabel           = false;

	PQNode<T,whaInfo*,Y>    *currentNode         = hChild1; // dummy
	PQNode<T,whaInfo*,Y>    *nextSibling         = nullptr;		// dummy
	PQNode<T,whaInfo*,Y>    *oldSibling          = nullptr;		// dummy

	if (hChild1 != nullptr)
		fullLabel = true;

	/*
	Trace the sequence of full children with at most one incident
	pertinent child. It marks all full nodes as $b$-node and the partial
	child, if available as $h$-child. The beginning of the sequence is
	stored in [[currentNode]] which is equal to [[h_child1]] before
	entering the while loop. Observe that this chunk cases if the while
	loop while scanning the sequence has reached the second endmost child
	of the $Q$-node (see [[if (nextSibling == 0)]]). This case may
	appear, if all children of the $Q$-node are pertinent and the second
	endmost child is the only partial child. The value [[fullLabel]] is
	set to [[false]] as soon as the end of the sequence is detected.
	*/
	while (fullLabel)
	{
		nextSibling = currentNode->getNextSib(oldSibling);
		if (!nextSibling)
			fullLabel = false;

		if (currentNode->status() == PQNodeRoot::PQNodeStatus::Full)
		{
			currentNode->getNodeInfo()->userStructInfo()->m_deleteType = whaType::B;
			pertinentChildCount++;
		}

		else
		{
			if (currentNode->status() == PQNodeRoot::PQNodeStatus::Partial)
			{
				currentNode->getNodeInfo()->userStructInfo()->m_deleteType = whaType::H;
				if ((currentNode->getNodeInfo()->userStructInfo()->m_pertLeafCount -
					currentNode->getNodeInfo()->userStructInfo()->m_h) > 0)
					pertinentChildCount++;
			}
			fullLabel = false;
		}
		oldSibling = currentNode;
		currentNode = nextSibling;
	}

	return pertinentChildCount;
}

template<class T, class Y>
int MaxSequencePQTree<T, Y>::setAchildren(
	PQNode<T, whaInfo*, Y> *hChild2,
	PQNode<T, whaInfo*, Y> *hChild2Sib)
{
	/* Variables:
	 *   - \a pertinentChildCount
	 *   - \a reachedEnd
	 *   - \a _sum denotes the number of pertinent leaves in the frontier
	 *     of the sequence.
	 *   - \a currentNode is the currently examined node of the sequence.
	 *   - \a nextSibling is a pointer needed for tracing the sequence.
	 *   - \a oldSibling is a pointer needed for tracing the sequence.
	 */
	// counts the pertinent children of the sequence.
	int                   pertinentChildCount = 0;

	PQNode<T,whaInfo*,Y>    *currentNode         = hChild2; // dummy
	PQNode<T,whaInfo*,Y>    *nextSibling         = nullptr;		// dummy
	PQNode<T,whaInfo*,Y>    *oldSibling          = nullptr;		// dummy

	//Mark [[hChild2]] either as $b$- or as $h$-node.
	if (hChild2->status() == PQNodeRoot::PQNodeStatus::Full)
		hChild2->getNodeInfo()->userStructInfo()->m_deleteType = whaType::B;
	else {
		//1. node of sequence is Empty?
		OGDF_ASSERT(hChild2->status() == PQNodeRoot::PQNodeStatus::Partial);
		hChild2->getNodeInfo()->userStructInfo()->m_deleteType = whaType::H;
	}

	if (currentNode->getNodeInfo()->userStructInfo()->m_w -
		currentNode->getNodeInfo()->userStructInfo()->m_h > 0)
		pertinentChildCount++;

	//Trace the sequence of pertinent children, marking the full children as $b$-node.
	//If a partial or empty node is detected, the end of the sequence is
	//reached and the partial node is marked as $h$-node.
	nextSibling = hChild2Sib;
	oldSibling = hChild2;

	if (nextSibling != nullptr)
	{
		currentNode = nextSibling;

		// is false]as long as the end of the sequence is not detected; true otherwise.
		bool reachedEnd = false;

		while(!reachedEnd)
		{
			if (currentNode->status() == PQNodeRoot::PQNodeStatus::Full)
			{
				currentNode->getNodeInfo()->userStructInfo()->m_deleteType = whaType::B;
				pertinentChildCount++;
			}
			else
			{
				if (currentNode->status() == PQNodeRoot::PQNodeStatus::Partial)
				{
					currentNode->getNodeInfo()->userStructInfo()->m_deleteType = whaType::H;
					if ((currentNode->getNodeInfo()->userStructInfo()->m_w -
						currentNode->getNodeInfo()->userStructInfo()->m_h) > 0)
					pertinentChildCount++;
				}
				reachedEnd = true;
			}
			if (!reachedEnd)
			{
				nextSibling = currentNode->getNextSib(oldSibling);
				if (nextSibling == nullptr)
					reachedEnd = true;
				oldSibling = currentNode;
				currentNode = nextSibling;
			}
		}
	}

	return pertinentChildCount;
}

template<class T,class Y>
void MaxSequencePQTree<T,Y>::markPertinentChildren(
	PQNode<T,whaInfo*,Y>  *nodePtr,
	PQNodeRoot::PQNodeStatus label,
	whaType deleteType)
{
	/**
	 * The function markPertinentChildren() uses just a pointer
	 * \a currentNode for tracing the pertinent children of \p nodePtr.
	 */
#if 0
	PQNode<T,whaInfo*,Y>  *currentNode = 0;
#endif

	if (label == PQNodeRoot::PQNodeStatus::Pertinent)
	{
		ListIterator<PQNode<T, whaInfo*, Y>*> it;
		for (it = partialChildren(nodePtr)->begin(); it.valid(); ++it)
			(*it)->getNodeInfo()->userStructInfo()->m_deleteType = deleteType;
		for (it = fullChildren(nodePtr)->begin(); it.valid(); ++it)
			(*it)->getNodeInfo()->userStructInfo()->m_deleteType = deleteType;
	}
	else if (label == PQNodeRoot::PQNodeStatus::Partial)
	{
		ListIterator<PQNode<T, whaInfo*, Y>*> it;
		for (it = partialChildren(nodePtr)->begin(); it.valid(); ++it)
			(*it)->getNodeInfo()->userStructInfo()->m_deleteType = deleteType;
	}

	else
	{
		ListIterator<PQNode<T, whaInfo*, Y>*> it;
		for (it = fullChildren(nodePtr)->begin(); it.valid(); ++it)
			(*it)->getNodeInfo()->userStructInfo()->m_deleteType = deleteType;
	}
}

template<class T, class Y>
void MaxSequencePQTree<T, Y>::haNumPnode(PQNode<T, whaInfo*, Y> *nodePtr)
{
	/* Variables:
	 *   - \a sumParW = sum_{i in Par(\p nodePtr)} w_i, where
	 *     Par(\p nodePtr) denotes the set of partial children of \p nodePtr.
	 *   - \a sumMax1 = max_{i in Par(\p nodePtr)}1 {(w_i - h_i)}
	 *     where Par(\p nodePtr) denotes the set of partial children of
	 *   - \p nodePtr and max 1 the first maximum.
	 *   - \a sumMax2 = max_{i in Par(\p nodePtr)}2 {(w_i - h_i)}
	 *     where Par(\p nodePtr) denotes the set of partial children of
	 *     \p nodePtr and max2 the second maximum.
	 *   - \a currentNode
	 *   - \a hChild1 is a pointer to the \a hChild1 of \p nodePtr.
	 *   - \a hChild2 is a pointer to the \a hChild2 of \p nodePtr.
	 *   - \a aChild is a pointer to the \a aChild of \p nodePtr.
	 */
	int                 sumParW   = 0;
	int                 sumMax1    = 0;
	int                 sumMax2    = 0;
	PQNode<T,whaInfo*,Y>  *currentNode = nullptr;
	PQNode<T,whaInfo*,Y>  *hChild1    = nullptr;
	PQNode<T,whaInfo*,Y>  *hChild2    = nullptr;
	PQNode<T,whaInfo*,Y>  *aChild     = nullptr;

	/*
	Computes the $h$-number
	\[ h = \sum_{i \in Par(\mbox{[[nodePtr]]})}w_i - \max_{i\in
	Par(\mbox{[[nodePtr]]})}1\left\{(w_i - h_i)\right\}\]
	of the $P$-node [[nodePtr]].
	This is done by scanning all partial children stored in the
	[[partialChildrenStack]] of [[nodePtr]] summing up the $w_i$ for every
	$i \in Par(\mbox{[[nodePtr]]})$ and detecting
	\[\max_{i\in Par(\mbox{[[nodePtr]]})}1\left\{(w_i - h_i)\right\}.\]
	Since this can be simultaneously it also computes
	\[\max_{i\in Par(\mbox{[[nodePtr]]})}2\left\{(w_i - h_i)\right\}\]
	which is needed to determine the $a$-number.
	After successfully determining the $h$-number, the [[hChild1]] and
	[[hChild2]] of [[nodePtr]] are set.
	*/

	ListIterator<PQNode<T,whaInfo*,Y>*> it;
	for (it = partialChildren(nodePtr)->begin(); it.valid(); ++it)
	{
		currentNode = (*it);
		sumParW = sumParW + currentNode->getNodeInfo()->userStructInfo()->m_w;
		int sumHelp = currentNode->getNodeInfo()->userStructInfo()->m_w -
			currentNode->getNodeInfo()->userStructInfo()->m_h;
		if (sumMax1 <= sumHelp)
		{
			sumMax2 = sumMax1;
			hChild2 = hChild1;
			sumMax1 = sumHelp;
			hChild1 = currentNode;
		}
		else if (sumMax2 <= sumHelp)
		{
			sumMax2 = sumHelp;
			hChild2 = currentNode;
		}
	}
	nodePtr->getNodeInfo()->userStructInfo()->m_hChild1 = hChild1;
	nodePtr->getNodeInfo()->userStructInfo()->m_hChild2 = hChild2;
	nodePtr->getNodeInfo()->userStructInfo()->m_h = sumParW - sumMax1;

	/*
	Compute the $a$-number of the $P$-node [[nodePtr]] where
	\[ a = \min \{ \alpha_1, \alpha_2\}\]
	such that
	\[\alpha_1 = \sum_{i \in P(\mbox{[[nodePtr]]})}w_i - \max_{i\in
	P(\mbox{[[nodePtr]]})}\left\{(w_i - h_i)\right\} \]
	which can be computed calling the function [[alpha1beta1Number]] and
	\[{\alpha}_2 \sum_{i \in Par(\mbox{[[nodePtr]]})}w_i -
	\max_{i\in Par(\mbox{[[nodePtr]]})}1\left\{(w_i - h_i)\right\}
	- \max_{i\in Par(\mbox{[[nodePtr]]})}2\left\{(w_i - h_i)\right\}\]
	This chunk uses two extra variables
	\begin{description}
	\item[[alpha1]] $ = \alpha_1$.
	\item[[alpha2]] $ = \alpha_2$.
	\end{description}
	*/
	int  alpha2 = sumParW - sumMax1 - sumMax2;
	int  alpha1 = alpha1beta1Number(nodePtr,&aChild);

	if (alpha1 <= alpha2)
	{
		nodePtr->getNodeInfo()->userStructInfo()->m_a = alpha1;
		nodePtr->getNodeInfo()->userStructInfo()->m_aChild = aChild;
	}
	else
	{
		nodePtr->getNodeInfo()->userStructInfo()->m_a = alpha2;
		nodePtr->getNodeInfo()->userStructInfo()->m_aChild = nullptr;
	}
}

template<class T,class Y>
void MaxSequencePQTree<T,Y>::haNumQnode(PQNode<T,whaInfo*,Y> *nodePtr)
{
	/* Variables:
	 *   - sumAllW = sum_{i in P(\p nodePtr)} w_i, where
	 *     P(\p nodePtr) denotes the set of pertinent children of \p nodePtr.
	 */
	int sumAllW = sumPertChild(nodePtr);

	hNumQnode(nodePtr,sumAllW);
	aNumQnode(nodePtr,sumAllW);
}

template<class T,class Y>
void MaxSequencePQTree<T,Y>::hNumQnode(
	PQNode<T,whaInfo*,Y> *nodePtr,
	int sumAllW)
{
	/* Variables:
	 *   - \a sumLeft = sum_{i in P(\p nodePtr)} w_i - sum_{i
	 *     in P_L(\p nodePtr)}(w_i - h_i), where
	 *     P_L(\p nodePtr) denotes the maximal consecutive sequence
	 *     of pertinent children on the left side of the Q-node
	 *     \p nodePtr such that only the rightmost node in
	 *     P_L(\p nodePtr) may be partial.
	 *   - \a sumRight = sum_{i in P(\p [nodePtr)}w_i - sum_{i
	 *     in P_L(\p nodePtr)}(w_i - h_i), where
	 *     P_L(\p nodePtr) denotes the maximal consecutive sequence
	 *     of pertinent children on the left side of the Q-node
	 *     \p nodePtr such that only the rightmost node in
	 *     P_L(\p nodePtr) may be partial.
	 *   - \a fullLabel
	 *   - \a aChild is a pointer to the a-child of \p nodePtr.
	 *   - \a leftChild is a pointer to the left endmost child of \p nodePtr.
	 *   - \a rightChild is a pointer to the right endmost child of \p nodePtr.
	 *   - \a holdSibling is a pointer to a child of \p nodePtr, needed
	 *     to proceed through sequences of pertinent children.
	 *   - \a checkSibling is a pointer to a currently examined child of \p nodePtr.
	 */
	int                  sumLeft     = 0;
	int                  sumRight    = 0;
	bool                  fullLabel = true;
	PQNode<T,whaInfo*,Y>   *leftChild    = nullptr;
	PQNode<T,whaInfo*,Y>   *rightChild   = nullptr;
	PQNode<T,whaInfo*,Y>   *holdSibling  = nullptr;
	PQNode<T,whaInfo*,Y>   *checkSibling = nullptr;

	//Compute the $h$-number of the $Q$-node [[nodePtr]]

	//Get endmost children of the $Q$-node [[nodePtr]].
	leftChild = nodePtr->getEndmost(0);
	rightChild = nodePtr->getEndmost(leftChild);
	OGDF_ASSERT(leftChild);
	OGDF_ASSERT(rightChild);

	/*
	Check the left
	side of the $Q$-node [[nodePtr]] for the maximal consecutive sequence
	of full nodes, including at most one partial child at the end of the sequence.

	The variable [[fullLabel]] is [[true]] as long as the [[while]]-loop
	has not detected an partial <b>or</b> empty child (see case [[if
	(leftChild->status() != Full)]]. Observe that the
	construction of the [[while]]-loop examines the last child if it is a
	partial child as well (see case [[if (leftChild->status() !=
	Empty)]] where in the computation in [[sumLeft]] we  take advantage
	of the fact, that the $h$-number of a full child is zero).
	*/
	while (fullLabel)
	{
		if (leftChild->status() != PQNodeRoot::PQNodeStatus::Full)
			fullLabel = false;
		if (leftChild->status() != PQNodeRoot::PQNodeStatus::Empty)
		{
			sumLeft = sumLeft +
				leftChild->getNodeInfo()->userStructInfo()->m_w -
				leftChild->getNodeInfo()->userStructInfo()->m_h;
			checkSibling = leftChild->getNextSib(holdSibling);
			if (checkSibling == nullptr)
				fullLabel = false;
			holdSibling = leftChild;
			leftChild = checkSibling;
		}
	}

	/*
	Check the right
	side of the $Q$-node [[nodePtr]] for the maximal consecutive sequence
	of full nodes, including at most one partial child at the end of the sequence.

	The variable [[fullLabel]] is [[true]] as long as the [[while]]-loop
	has not detected an partial <b>or</b> empty child (see case [[if
	(leftChild->status() != Full)]]. Observe that the
	construction of the [[while]]-loop examines the last child if it is a
	partial child as well (see case [[if (leftChild->status() !=
	Empty)]] where in the computation in [[sumLeft]] we  take advantage
	of the fact, that the $h$-number of a full child is zero).
	*/
	holdSibling = nullptr;
	checkSibling = nullptr;
	fullLabel = true;
	while (fullLabel)
	{
		if (rightChild->status() != PQNodeRoot::PQNodeStatus::Full)
			fullLabel = false;
		if (rightChild->status() != PQNodeRoot::PQNodeStatus::Empty)
		{
			sumRight = sumRight +
				rightChild->getNodeInfo()->userStructInfo()->m_w -
				rightChild->getNodeInfo()->userStructInfo()->m_h;

			checkSibling = rightChild->getNextSib(holdSibling);

			if (checkSibling == nullptr)
				fullLabel = false;

			holdSibling = rightChild;
			rightChild = checkSibling;
		}
	}

	/*
	After computing the number of pertinent leaves that stay in the $PQ$-tree
	when keeping either the left pertinent or the right pertinent side of
	the $Q$-node in the tree, this chunk chooses the side where the
	maximum number of leaves stay in the tree.
	Observe that we have to case the fact, that on both sides of the
	$Q$-node [[nodePtr]] no pertinent children are.
	*/
	leftChild = nodePtr->getEndmost(0);
	rightChild = nodePtr->getEndmost(leftChild);
	if (sumLeft == 0 && sumRight == 0)
	{
		nodePtr->getNodeInfo()->userStructInfo()->m_h = sumAllW;
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild1 = nullptr;
	}
	else if (sumLeft < sumRight)
	{
		nodePtr->getNodeInfo()->userStructInfo()->m_h = sumAllW - sumRight;
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild1 = rightChild;
	}
	else
	{
		nodePtr->getNodeInfo()->userStructInfo()->m_h = sumAllW - sumLeft;
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild1 = leftChild;
	}
}

template<class T,class Y>
void MaxSequencePQTree<T,Y>::aNumQnode(
	PQNode<T,whaInfo*,Y> *nodePtr,
	int sumAllW)
{
	/* Variables:
	 *   - \a beta1 = sum_{i in P(\p nodePtr} w_i - max_{i in P(\p nodePtr)}{(w_i =
	 *     a_i)}, where $P(\p nodePtr) denotes the set of all pertinent
	 *     children of the Q-node \p nodePtr. Depicts the a-number if just one
	 *     child of \p nodePtr is made a-node. Computed by calling the function alpha1beta1Number().
	 *   - \a beta2 = sum_{i in P(\p nodePtr)} w_i - max_{P_A(\p nodePtr)}{sum_{i in
	 *     P_A(\p nodePtr)}(w_i-h_i)}, where $P_A(\p nodePtr) is a maximal consecutive
	 *     sequence of pertinent children of the Q-node \p nodePtr such that all
	 *     nodes in P_A(\p nodePtr) except for the leftmost and rightmost ones are
	 *     full. Computed by this chunk.
	 *   - \a aSum depicts the number of pertinent leaves of the actual visited sequence.
	 *   - \a aHoldSum depicts the number of leaves in the actual maximum sequence.
	 *   - \a endReached is true if reached the end of the Q-node \p nodePtr and false otherwise.
	 *   - \a leftMost pointer to the leftmost end of the actual visited sequence.
	 *   - \a leftMostHold pointer to the leftmost end of the current maximum sequence.
	 *   - \a actualNode pointer to a child of the Q-node. It is the
	 *     node that is actually processed in the sequence of children.
	 *   - \a currentNode pointer to a node in a consecutive pertinent
	 *     sequence. Needed to process all nodes stored in \a sequence.
	 *   - \a lastChild is a pointer to the endmost child of the Q-node
	 *     that is opposite to the endmost child, where this chunk starts
	 *     processing the sequence of children.
	 *   - \a sequence is a SList of type PQNode<T,whaInfo*,Y>* storing
	 *     the nodes of a consecutive sequence that is actually processed.
	 */
	PQNode<T,whaInfo*,Y>   *aChild      = nullptr;
	int                     beta1       = alpha1beta1Number(nodePtr,&aChild);
	int                     beta2       = 0;
	int                     aSum        = 0;
	int                     aHoldSum    = 0;
	bool                    endReached  = 0;
	PQNode<T,whaInfo*,Y>   *leftMost      = nullptr;
	PQNode<T,whaInfo*,Y>   *leftSib       = nullptr;
	PQNode<T,whaInfo*,Y>   *leftMostHold  = nullptr;
	PQNode<T,whaInfo*,Y>   *leftSibHold   = nullptr;
	PQNode<T,whaInfo*,Y>   *actualNode    = nullptr;
	PQNode<T,whaInfo*,Y>   *currentNode    = nullptr;
	PQNode<T,whaInfo*,Y>   *lastChild     = nullptr;
	PQNode<T,whaInfo*,Y>   *holdSibling  = nullptr;
	PQNode<T,whaInfo*,Y>   *checkSibling = nullptr;
		// pointer to the second endmost child

	SList<PQNode<T,whaInfo*,Y>*> sequence;

	actualNode   = nodePtr->getEndmost(0);
	lastChild    = nodePtr->getEndmost(actualNode);

	endReached = false;
	while (!endReached)
	{
		/*
		Process the children of a $Q$-node [[nodePtr]] from one end of [[nodePtr]] to the
		other, searching for a consecutive sequence of pertinent nodes with
		the maximum number of pertinent leaves, such that all nodes of the
		pertinent sequence are full except possibly the two endmost children
		which are allowed to be partial.
		*/
		if (sequence.empty())
		{
			/*
			Currently no consecutive sequence of pertinent children
			is detected while scanning the children of the $Q$-node.
			Check the [[actualNode]] if it is the first child of
			such a sequence. If so, place [[actualNode]] on the stack [[sequence]].
			*/
			if (actualNode->status() != PQNodeRoot::PQNodeStatus::Empty)
			{
				sequence.pushFront(actualNode);
				leftMost = nullptr;
				leftSib = nullptr;
			}
		}
		else
		{
			/*
			[[actualNode]] is a sibling of a consecutive pertinent sequence that has
			been detected in an earlier step, while scanning the children of the $Q$-node.
			This chunk cases on the status of the [[actualNode]].

			In case that the status of
			the [[actualNode]] is [[Full]], [[actualNode]] is included into the
			sequence of pertinent children by pushing it onto the stack
			[[sequence]].

			If [[actualNode]] is Empty, we have reached the end of
			the actual consecutive sequence of pertinent children. In this case
			the $a$-numbers of the nodes in the sequence have to be summed up.

			If the [[actualNode]] is [[Partial]], the end of the consecutive sequence
			is reached and similar actions to the [[Empty]] have to be
			performed. However, [[actualNode]] might mark the beginning of
			another pertinent sequence. Hence it has to be stored again in [[sequence]].
			*/
			if (actualNode->status() == PQNodeRoot::PQNodeStatus::Full)
				sequence.pushFront(actualNode);

			else if (actualNode->status() == PQNodeRoot::PQNodeStatus::Empty)
			{
				/*
				If [[actualNode]] is Empty, the end of
				the actual consecutive sequence of pertinent children is reached . In
				this case, all nodes of the currently examined consecutive sequence are stored in
				[[sequence]].
				They are removed from the stack and their $a$-numbers are summed up.
				If necessary, the sequence with the largest number of full leaves in
				its frontier is updated.
				*/
				aSum = 0;

				while (!sequence.empty())
				{
					currentNode = sequence.popFrontRet();
					aSum = aSum + currentNode->getNodeInfo()->userStructInfo()->m_w
						- currentNode->getNodeInfo()->userStructInfo()->m_h;
					if (sequence.size() == 1)
						leftSib = currentNode;
				}
				leftMost = currentNode;

				if (aHoldSum < aSum)
				{
					aHoldSum = aSum;
					leftMostHold = leftMost;
					leftSibHold = leftSib;
				}

			}
			else
			{
				/*
				If the [[actualNode]] is [[Partial]], the end of the consecutive sequence
				is reached. In
				this case, all nodes of the currently examined consecutive sequence are stored in
				[[sequence]].
				They are removed from the stack and their $a$-numbers are summed up.
				If necessary, the sequence with the largest number of full leaves in
				its frontier is updated.
				However, [[actualNode]] might mark the beginning of
				another pertinent sequence. Hence it has to be stored again in [[sequence]].
				*/
				sequence.pushFront(actualNode);
				aSum = 0;
				while (!sequence.empty())
				{
					currentNode = sequence.popFrontRet();
					aSum = aSum + currentNode->getNodeInfo()->userStructInfo()->m_w -
							currentNode->getNodeInfo()->userStructInfo()->m_h;
					if (sequence.size() == 1)
						leftSib = currentNode;
				}
				if (leftSib == nullptr)
					leftSib = actualNode;
				leftMost = currentNode;

				if (aHoldSum < aSum)
				{
					aHoldSum = aSum;
					leftMostHold = leftMost;
					leftSibHold = leftSib;
				}

				sequence.pushFront(actualNode);

			}
		}

		// Get the next sibling
		if (actualNode != lastChild)
		{
			checkSibling = actualNode->getNextSib(holdSibling);
			holdSibling = actualNode;
			actualNode = checkSibling;
		}
		else
			// End of Q-node reached.
			endReached = true;
	}


	/*
	After processing
	the last child of the $Q$-node, this chunk checks, if this child was
	part of a pertinent consecutive sequence. If this is the case, the
	stack storing this seuquence was not emptied and the number of
	pertinent leaves in its frontier was not computed. Furhtermore the
	last child was not stored in [[sequence]].
	This chunk does the necessary updates for the last consecutive sequence.
	*/
	if (!sequence.empty())
	{
		aSum = 0;
		while (!sequence.empty())
		{
			currentNode = sequence.popFrontRet();
			aSum = aSum + currentNode->getNodeInfo()->userStructInfo()->m_w -
				currentNode->getNodeInfo()->userStructInfo()->m_h;
			if (sequence.size() == 1)
				leftSib = currentNode;
		}
		leftMost = currentNode;

		if (aHoldSum < aSum)
		{
			aHoldSum = aSum;
			leftMostHold = leftMost;
			leftSibHold = leftSib;
		}
	}
	/*
	After computing
	${\beta}_1$ and ${\beta}_2$, describing the number of pertinent leaves
	that have to be deleted when choosing either one node to be an
	$a$-node or a complete sequence, this chunk gets the $a$-number of the
	$Q$-node [[nodePtr]] by choosing
	\[a = \min\{{\beta}_1,{\beta}_2\]
	Also set [[aChild]] and [[hChild2]] of [[nodePtr]] according to the
	chosen minimum.
	*/
	beta2 = sumAllW - aHoldSum;
	if (beta2 < beta1)
	{
		nodePtr->getNodeInfo()->userStructInfo()->m_a = beta2;
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild2 = leftMostHold;
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild2Sib = leftSibHold;
		nodePtr->getNodeInfo()->userStructInfo()->m_aChild = nullptr;
	}
	else
	{
		nodePtr->getNodeInfo()->userStructInfo()->m_a = beta1;
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild2 = nullptr;
		nodePtr->getNodeInfo()->userStructInfo()->m_hChild2Sib = nullptr;
		nodePtr->getNodeInfo()->userStructInfo()->m_aChild = aChild;
	}
}

template<class T, class Y>
int MaxSequencePQTree<T, Y>::alpha1beta1Number(
	PQNode<T, whaInfo*, Y> *nodePtr,
	PQNode<T, whaInfo*, Y> **aChild)
{
	/* Variables:
	 *   - \a sumMaxA = max_{i in P(\p nodePtr)}{(w_i = a_i)}.
	 *   - \a sumAllW = w  = sum_{i in P(\p nodePtr)}w_i.
	 *   - \a sumHelp is a help variable.
	 *   - \a currentNode depicts a currently examined pertinent node.
	 *
	 * The function uses two while loops over the parial and the full
	 * children of \p nodePtr. It hereby computes the values \p w and
	 * max_{i in P(\p nodePtr}{(w_i = a_i)}.
	 * After finishing the while loops, the function
	 * alpha1beta1Number() returns the numbers alpha_1 = beta_1
	 * and the \p aChild.
	 */
	int                 sumMaxA   = 0;
	int                 sumAllW   = 0;
	int                 sumHelp    = 0;
	PQNode<T,whaInfo*,Y>  *currentNode = nullptr;

	ListIterator<PQNode<T,whaInfo*,Y>*> it;
	for (it = fullChildren(nodePtr)->begin(); it.valid(); ++it)
	{
		currentNode = (*it);
		sumAllW = sumAllW +
			currentNode->getNodeInfo()->userStructInfo()->m_w;
		sumHelp = currentNode->getNodeInfo()->userStructInfo()->m_w
			- currentNode->getNodeInfo()->userStructInfo()->m_a;
		if (sumMaxA < sumHelp)
		{
			sumMaxA = sumHelp;
			(*aChild) = currentNode;
		}
	}

	for (it = partialChildren(nodePtr)->begin(); it.valid(); ++it)
	{
		currentNode = (*it);
		sumAllW = sumAllW + currentNode->getNodeInfo()->userStructInfo()->m_w;
		sumHelp = currentNode->getNodeInfo()->userStructInfo()->m_w - currentNode->getNodeInfo()->userStructInfo()->m_a;
		if (sumMaxA < sumHelp)
		{
			sumMaxA = sumHelp;
			(*aChild) = currentNode;
		}
	}
	return sumAllW - sumMaxA;
}

template<class T,class Y>
int MaxSequencePQTree<T,Y>::sumPertChild(PQNode<T,whaInfo*,Y> *nodePtr)
{
	/* Variables:
	 *   - \a it depicts a currently examined pertinent node.
	 *   - \a sum = \a w  = sum_{i in P(\p nodePtr)}w_i.
	 *
	 * The function uses two for loops over the parial and the full
	 * children of \p nodePtr. It hereby computes the values $w$ stored in \a sum.
	 * After finishing the while loops, the function
	 * sumPertChild() returns the number \a w.
	 */
	int sum = 0;
	ListIterator<PQNode<T, whaInfo*, Y>*> it;
	for (it = fullChildren(nodePtr)->begin(); it.valid(); ++it)
		sum = sum + (*it)->getNodeInfo()->userStructInfo()->m_w;
	for (it = partialChildren(nodePtr)->begin(); it.valid(); ++it)
		sum = sum + (*it)->getNodeInfo()->userStructInfo()->m_w;

	return sum;
}

template<class T, class Y>
PQNode<T, whaInfo*, Y>* MaxSequencePQTree<T, Y>::GetParent(PQNode<T, whaInfo*, Y>* nodePtr)
{
	if (nodePtr->parent() == nullptr)
		return nullptr;
	else if (nodePtr->parent()->status() != PQNodeRoot::PQNodeStatus::Eliminated)
		return nodePtr->parent();
	else
	{
		PQNode<T, whaInfo*, Y> *nextNode = nodePtr;
		PQNode<T, whaInfo*, Y> *currentNode = nullptr;
		PQNode<T, whaInfo*, Y> *oldSib = nullptr;
		SListPure<PQNode<T, whaInfo*, Y>*> L;

		currentNode = nodePtr->getNextSib(nullptr);
		oldSib = nodePtr;
		L.pushFront(nodePtr);
		while (currentNode->parent()->status() == PQNodeRoot::PQNodeStatus::Eliminated)
		{
			L.pushFront(currentNode);
			nextNode = currentNode->getNextSib(oldSib);
			oldSib = currentNode;
			currentNode = nextNode;
		}
		while (!L.empty())
			L.popFrontRet()->parent(currentNode->parent());
		return currentNode->parent();
	}
}

}
