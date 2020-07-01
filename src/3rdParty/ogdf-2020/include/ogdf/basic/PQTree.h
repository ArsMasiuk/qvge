/** \file
 * \brief Declaration and implementation of the class PQTree.
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

#include <ogdf/basic/Queue.h>
#include <ogdf/basic/Array.h>

#include <ogdf/basic/pqtree/PQNode.h>
#include <ogdf/basic/pqtree/PQInternalNode.h>
#include <ogdf/basic/pqtree/PQLeaf.h>
#include <ogdf/basic/pqtree/PQLeafKey.h>
#include <ogdf/basic/pqtree/PQInternalKey.h>
#include <ogdf/basic/pqtree/PQNodeKey.h>

namespace ogdf {

template<class T,class X,class Y>
class PQTree
{
public:
	PQTree();

	/**
	 * Destructor.
	 *
	 * In order to free allocated memory, all nodes of the
	 * tree have to be deleted, hence their destructors have to be called.
	 * This is done in the function Cleanup().
	 * Furthermore all other initialized memory has to be freed which is
	 * done as well in the function Cleanup().
	 */
	virtual ~PQTree() { Cleanup(); }

	/**
	 * Adds a set of elements to the already
	 * existing set of elements of a PQ-tree.
	 * These elements have to be of type PQLeafKey
	 * and are handed to the function in an array leafKeys.
	 * The father of the new elements that has to be an existing P- or Q-node,
	 * has to be specified and is not allowed to have children.
	 *
	 * The above mentioned facts
	 * are checked by the function #addNodeToNewParent() and the process
	 * of adding a child to parent is interrupted with an error message
	 * returning 0 as soon none of the facts is fullfilled.
	 *
	 * @return 1 if it succeeded in adding the leaves to parent.
	 */
	bool addNewLeavesToTree(
		PQInternalNode<T,X,Y>        *father,
		SListPure<PQLeafKey<T,X,Y>*> &leafKeys);

	/**
	 * Cleans up all stacks, flags and pointers of a pertinent node
	 * that has been visited during the reduction process.
	 */
	void emptyNode(PQNode<T,X,Y>* nodePtr);

	/**
	 * Returns the keys stored in the leaves of the front of
	 * \p nodePtr. A specified node \p nodePtr of the PQ-tree is
	 * handed to the function and front() detects the leaves in the front
	 * of this node returning the elements represented by the leaves. These
	 * elements are stored in an array of keys named \p leafKeys.
	 * Observe that front() uses \p leafKeys[0] to store the
	 * first key.
	 */
	virtual void front(
		PQNode<T,X,Y>* nodePtr,
		SListPure<PQLeafKey<T,X,Y>*> &leafKeys);

	virtual void CleanNode(PQNode<T,X,Y>* /* nodePtr */) { }

	/**
	 * Removes the entire PQ-tree.
	 * It is called by the destructor.
	 * It scans all nodes of the tree and frees the
	 * memory used by the tree. It removes the memory
	 * allocated by the following datastructures:
	 *   - #m_root,
	 *   - #m_pseudoRoot,
	 *   - #m_pertinentNodes.
	 * It enables the client to reuse the function #Initialize().
	 */
	virtual void Cleanup();

	/**
	 * If the user wishes to use different flags in a derived class of PQTree
	 * that are not available in this implementation, he can overload this function
	 * to make a valid cleanup of the nodes.
	 * It will be called per default by the function #emptyAllPertinentNodes().
	 */
	virtual void clientDefinedEmptyNode(PQNode<T,X,Y>* nodePtr) {
		emptyNode(nodePtr);
	}

	/**
	 * Cleans up all flags that have been set in the
	 * pertinent nodes during the reduction process. All pertinent nodes have been
	 * stored in the private member stack #m_pertinentNodes during the #Bubble() phase
	 * or when processing one of the templates (see #templateL1() to #templateQ3()).
	 *
	 * This function has to be called after a reduction has been processed.
	 */
	virtual void emptyAllPertinentNodes();

	/**
	 * Initializes the PQ-tree with a set of elements.
	 * These elements have to be template classes of the class template PQLeafKey
	 * and are handed to the function in an array \p leafKeys.
	 * The function constructs the universal PQ-tree. If the
	 * \a numberOfElements > 1, the universal PQ-tree consists of one P-node
	 * as root (stored in #m_root) and all leaves gathered underneath the P-node,
	 * symbolizing all kinds of permutations. If \a numberOfElements = 1,
	 * the universal PQ-tree consists of a single PQLeaf, being the root of
	 * the tree.
	 *
	 * Observe that the first element has to be stored in
	 * \p leafKeys[0] and the last one in
	 * \p leafKeys[\a numberOfElements-1].
	 *
	 * @return 1 if the initialization of the PQ-tree was successful.
	 */
	virtual int Initialize(SListPure<PQLeafKey<T,X,Y>*> &leafKeys);

	/**
	 * Tests whether permissible permutations of the
	 * elements of U exist such that the elements of a subset S of U,
	 * stored in \p leafKeys, form a consecutive sequence. If there exists
	 * such a permutation, the PQ-tree is reduced and Reduction()
	 * returns 1.
	 *
	 * This function gets a list \p leafKeys
	 * of pointers to elements of type PQLeafKey,
	 * representing all elements of S.
	 */
	virtual bool Reduction(SListPure<PQLeafKey<T,X,Y>*> &leafKeys);

	//! Returns a pointer of the root node of the PQTree.
	PQNode<T,X,Y>* root() const {
		return m_root;
	}

	//! The function writeGML() prints the PQ-tree in the GML fileformat.
	//! @{
	void writeGML(const char *fileName);
	void writeGML(std::ostream &os);
	//! @}

protected:
	//! a pointer to the root of the $PQ$-tree.
	PQNode<T,X,Y>* m_root;

	//! a pointer to the root of the pertinent subtree.
	PQNode<T,X,Y>* m_pertinentRoot;

	//! a pointer to the virtual root of the pertinent subtree, in case that the pertinent root cannot be detected.
	PQNode<T,X,Y>* m_pseudoRoot;

	//! Stores the total number of nodes that have been allocated.
	/**
	 * Gives every node that has been used once in the
	 * PQ-tree a unique identification number.
	*/
	int m_identificationNumber;

	//! Stores the number of leaves.
	int m_numberOfLeaves;

	/**
	 * Stores all nodes that have been marked PQNodeRoot::PQNodeStatus::Full or
	 * PQNodeRoot::PQNodeStatus::Partial during a reduction. After the reduction has been
	 * finished succesfully, all pertinent nodes are reinitialized and
	 * prepared for the next reduction. This list also contains pertinent
	 * nodes that have been removed during a reduction. When detected in
	 * the stack, their memory is freed.
	 */
	List<PQNode<T,X,Y>*> *m_pertinentNodes;

	/**
	 * Realizes a function described in [Booth].
	 * It <em>bubbles</em> up from the pertinent leaves to the pertinent root
	 * in order to make sure that every pertinent node in the pertinent subtree
	 * has a valid pointer to its parent. If Bubble() does not succed in doing so,
	 * then the set of elements, stored in the \p leafKeys cannot form
	 * a consecutive sequence.
	 */
	virtual bool Bubble(SListPure<PQLeafKey<T,X,Y>*> &leafKeys);

	/**
	 * Performs the reduction of the pertinent leaves
	 * with the help of the template matchings, designed by Booth and Lueker.
	 * The reader should observe that this function can only be called after
	 * every pertinent node in the pertinent subtree has gotten a valid parent
	 * pointer. If this is not the case, the programm will be interrupted
	 * by run-time errors such as seqmentation faults. The pertinent nodes
	 * can get valid parent pointers by using the function #Bubble().
	 * If it returns 1, then it was succesful in
	 * giving each pertinent node in the pertinent subtree a valid parent pointer.
	 * If it returns 0, then some nodes do not have a valid
	 * parent pointer and the pertinent leaves are not reducable.
	 *
	 * The function Reduce() starts with the pertinent leaves and stores
	 * them in
	 * a queue \a processNodes. Every time a node is processed, its parent is
	 * checked whether all its pertinent children are already processed.
	 * If this is the case, the parent is allowed to be processed as well
	 * and stored in the queue.
	 *
	 * Processing a node means that the function Reduce() tries to apply
	 * one of the template matchings. In case that one template matching was
	 * successful, the node was reduced and Reduce() tries to reduce the
	 * next node.
	 * In case that no template matching was successfully applied, the tree is
	 * is irreducible. This causes the reduction process to be halted
	 * returning 0.
	 */
	virtual bool Reduce(SListPure<PQLeafKey<T,X,Y>*> &leafKeys);

	/**
	 * Template matching for leaves.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a PQLeaf,
	 * templateL1() considers itself responsible for the node and will
	 * apply the template matching for pertinent leaves to \p nodePtr.
	 * If the flag \p isRoot is set to 1, it signalizes
	 * templateL1() that \p nodePtr is the root of
	 * the pertinent subtree. In any other case the flag has to be 0.
	 *
	 * If templateL1() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * The function updates the \a fullChildren stack of its parent, as long as
	 * \p nodePtr is not the root of the pertinent subtree.
	 * Observe that \p nodePtr needs a valid pointer to its parent. This
	 * can be achieved by using the function Bubble() or any other
	 * appropriate, user defined function.
	 */
	virtual bool templateL1(PQNode<T,X,Y> *nodePtr, bool isRoot);

	/**
	 * Template matching for P-nodes with only full children.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a P-node with
	 * only full children,
	 * templateP1() considers itself responsible for the node and will
	 * apply the template matching for full P-nodes to \p nodePtr.
	 * If the flag \p isRoot is set to 1, it signalizes
	 * templateP1() that \p nodePtr is the root of
	 * the pertinent subtree. In any other case the flag has to be 0.
	 *
	 * If templateP1() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * If the P-node is not the root of the pertinent subtree,
	 * the \a fullChildren stack of the parent of \p nodePtr is updated.
	 * If the P-node is the root of the pertinent subtree, nothing has to
	 * be done.
	 */
	virtual bool templateP1(PQNode<T,X,Y> *nodePtr, bool isRoot);

	/**
	 * Template matching for a P-node with full \b and empty children
	 * that is the root of the pertinent subtree.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a P-node with
	 * no partial children,
	 * templateP2() considers itself responsible for the node and will
	 * apply the template matching \b P2 to \p nodePtr.
	 * Observe that the user calling this function has to make sure that
	 * \p nodePtr is partial and is the root of the pertinent subtree.
	 *
	 * If templateP2() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * The function templateP2() creates a new full P-node that
	 * will be the new root of the pertinent subtree. It then copies all full
	 * children from \p nodePtr to the new root of the pertinent subtree.
	 */
	virtual bool templateP2(PQNode<T,X,Y> **nodePtr);

	/**
	 * Template matching for a P-node with full \b and empty children that is
	 * \b not the root of the pertinent subtree.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a P-node with
	 * no partial children,
	 * templateP3() considers itself responsible for the node and will
	 * apply the template matching \b P3 to \p nodePtr.
	 * Observe that the user calling this function has to make sure that
	 * \p nodePtr is partial and is not the root of the pertinent subtree.
	 *
	 * If templateP3() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * The function templateP3() creates
	 * a new full P-node, stored in \a newPnode and copies the full children
	 * of \p nodePtr to \a newPnode. \p nodePtr keeps all empty children
	 * and will be labeled empty. A new partial Q-node will be created and stored
	 * in \a newQnode. \a newQnode is placed at the position of \p nodePtr
	 * in the tree and gets two children: \p nodePtr itself and the newly
	 * created \a newPnode.
	 */
	virtual bool templateP3(PQNode<T,X,Y> *nodePtr);

	/**
	 * Template matching for a P-node with full, empty and exactly one partial children.
	 * The P-node has to be the root of the pertinent subtree.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a P-node with
	 * one partial child,
	 * templateP4() considers itself responsible for the node and will
	 * apply the template matching \b P4 to \p nodePtr.
	 * Observe that the user calling this function has to make sure that
	 * \p nodePtr is the root of the pertinent subtree.
	 *
	 * If templateP4() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * The function templateP4() creates a new full P-node, if neccessary,
	 * and copies the full children of \p nodePtr to this P-node.
	 * The new P-node then is made endmost child of \a partialChild.
	 * The node \a partialChild is used to store the adress of the partial
	 * child of \p nodePtr.
	 * The \a partialChild itself stays child of \p nodePtr.
	 * Most of the here described action is done in the function
	 * copyFullChildrenToPartial().
	 */
	virtual bool templateP4(PQNode<T,X,Y> **nodePtr);

	/**
	 * Template matching for a P-node with full, empty children and exactly one partial
	 * child. The P-node is not allowed to be the root of the pertinent subtree.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a P-node with
	 * one partial child,
	 * templateP5() considers itself responsible for the node and will
	 * apply the template matching \b P5 to \p nodePtr.
	 * Observe that the user calling this function has to make sure that
	 * \p nodePtr is not the root of the pertinent subtree.
	 *
	 * If templateP5() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 */
	virtual bool templateP5(PQNode<T,X,Y> *nodePtr);

	/**
	 * Template matching for a P-node with full, empty and exactly two partial children.
	 * The P-node must be the root of the pertinent subtree.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a P-node with
	 * two partial children,
	 * templateP6() considers itself responsible for the node and will
	 * apply the template matching \b P6 to \p nodePtr.
	 * Observe that the user calling this function has to make sure that
	 * \p nodePtr is the root of the pertinent subtree.
	 *
	 * If templateP6() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * The function templateP6() creates, if neccessary,
	 * a new full P-node and copies all the full children of \p nodePtr
	 * to the new full P-node, whereas all empty children stay children of
	 * \p nodePtr. The new P-node will be copied to one of the partial children
	 * as endmost child of this partial node. The children of the second
	 * partial node are copied to the first one, such that the pertinent nodes
	 * form a consecutive sequence.
	 */
	virtual bool templateP6(PQNode<T,X,Y> **nodePtr);

	/**
	 * Template matching for Q-nodes with only full children.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a Q-node with
	 * only full children,
	 * templateQ1() considers itself responsible for the node and will
	 * apply the template matching for full Q-nodes to \p nodePtr.
	 * If the flag \p isRoot is set to 1, it signalizes
	 * templateQ1() that \p nodePtr is the root of
	 * the pertinent subtree. In any other case the flag has to be 0.
	 *
	 * If templateQ1() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * Different to the templateP1() for P-nodes,
	 * this function is not able to check if Q-node is full by comparing
	 * the number of children with the number of full children. The reason
	 * is the application of the #m_pseudoRoot at certain steps in the
	 * matching algorithm. This #m_pseudoRoot is used instead of the real
	 * root of the pertinent subtree in case that no parent pointer was
	 * found. But this implies that changing  the number of the children of
	 * the pertinent root is not registered by the pertinent root. Hence we
	 * are not allowed to use the \a childCount of Q-nodes.
	 */
	virtual bool templateQ1(PQNode<T,X,Y> *nodePtr, bool isRoot);

	/**
	 * Template matching for Q-nodes  with a pertinent
	 * sequence of children on one side of the Q-node.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a Q-node with
	 * a pertinent
	 * sequence of children on one side of the Q-node,
	 * templateQ2() considers itself responsible for the node and will
	 * apply the template matching \b Q2 to \p nodePtr.
	 * If the flag \p isRoot is set to 1, it signalizes
	 * templateQ2() that \p nodePtr is the root of
	 * the pertinent subtree. In any other case the flag has to be 0.
	 *
	 * If templateQ2() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * Below a short description is given of all different cases that
	 * may occure and that are handled by the function templateQ2(), \b regardless
	 * whether the Q-node \p nodePtr is root of the pertinent subtree or not.
	 * The description is somewhat trunkated and should be understood as a
	 * stenographic description of the labels of the children of \p nodePtr
	 * when running through the children from one side to the other. Of course
	 * we leave the mirror-images out.
	 *   - full, empty
	 *   - full, partial, empty
	 *   - full, partial
	 *   - partial, empty.
	 */
	virtual bool templateQ2(PQNode<T,X,Y> *nodePtr, bool isRoot);

	/*
	 * Template matching for Q-nodes with empty and/or partial children at both
	 * ends and a sequence of full and/or partial children in the middle.
	 * The Q-node must be the root of the pertinent subtree.
	 * The function requires as input any pointer to a node stored in
	 * \p nodePtr. If the node stored in \p nodePtr is a Q-node
	 * with empty and/or partial children at both ends and a sequence
	 * full or partial children in the middle,
	 * templateQ3() considers itself responsible for the node and will
	 * apply the template matching \b Q3 to \p nodePtr.
	 * Observe that the user calling this function has to make sure that
	 * \p nodePtr is the root of the pertinent subtree.
	 *
	 * If templateQ3() was responsible for \p nodePtr and the
	 * reduction was successful, the return value is 1. Otherwise
	 * the return value is 0.
	 *
	 * Below a short description is given of all different cases that
	 * may occure and are handled by the function templateQ3(), \b regardless
	 * whether the Q-node \p nodePtr is the root of the pertinent subtree or not.
	 * The description is somewhat trunkated and should be understood as a
	 * stenographic description of the labels of the children of \p nodePtr
	 * when running through the children from one side to the other. Of course
	 * we leave the mirror-images out.
	 *   - empty, full, empty
	 *   - empty, partial, full, partial, empty
	 *   - empty, partial, full, empty
	 *   - empty, partial, full, partial
	 *   - partial, full, partial
	 *   - empty, partial, partial, empty
	 *   - empty, partial, partial
	 *   - partial, partial
	 */
	virtual bool templateQ3(PQNode<T,X,Y> *nodePtr);

	/**
	 * Adds a node \p child as a child to another node specified in \p parent.
	 * The \p parent of the new node has to be an existing P- or Q-node and
	 * is \b not allowed to have children.
	 * In the case, that \p parent has children, addNewNodeToParent()
	 * returns 0 printing an error-message.
	 * In this case, use the overloaded function specifying the future
	 * siblings of \p child.
	 *
	 * @return 1 after successfully inserting \p child to \p parent, otherwise 0
	 */
	virtual bool addNodeToNewParent(
		PQNode<T,X,Y>* parent,
		PQNode<T,X,Y>* child);

	/**
	 * Adds a node \p child to the children
	 * of another node specified in \p parent.
	 * The \p parent of the new node has to be an existing P- or Q-node and
	 * is allowed to have children. In case that \p parent has children,
	 * the siblings of the new introduced child <b> must be specified</b>.
	 * If no siblings are specified, the function #addNodeToNewParent(PQNode<T,X,Y>*,PQNode<T,X,Y>*)
	 * is called by default.
	 * If the \p parent is not specified, the function assumes that
	 * \p child is added as interior child to a Q-node.
	 *
	 * The client of this function should observe the following facts:
	 *   - If \p parent is a P-node, than only one sibling is needed in order
	 *     to enter the \p child. If the client specifies two siblings in \p leftBrother
	 *     and \p rightBrother, then an arbitrary one is choosen to be a sibling.
	 *   - If \p parent is a Q-node, two siblings <b>must be specified</b> if
	 *     \p child has to become an interior child of the Q-node. If just
	 *     one sibling is specified, this implies that \p child is about to become
	 *     a new endmost child of \p parent. So either \p leftBrother or
	 *     \p rightBrother must store an existing endmost child of \p parent.
	 *   - If \p parent is a zero pointer, addNodeToNewParents() assumes
	 *     that \p child is added as interior child to a Q-node. In this
	 *     case \b both siblings of \p child <b>have to be specified</b>. Observe
	 *     however, that it is also legal to specify the parent in this case.
	 *
	 * The above mentioned facts are checked and the process
	 * of adding a child to \p parent is interrupted with an error message
	 * returning 0 as soon none of the facts is fulfilled.
	 *
	 * @return 1 if it succeeded in adding the \p child to \p parent, otherwise 0
	*/
	virtual bool addNodeToNewParent(
		PQNode<T,X,Y>* parent,
		PQNode<T,X,Y>* child,
		PQNode<T,X,Y>* leftBrother,
		PQNode<T,X,Y>* rightBrother);

	/**
	 * Checks if \p child is the only child of \p parent.
	 * If so, \p child is connected to its
	 * grandparent, as long as parent is not the root of the tree. In case
	 * that \p parent is the root of the tree and \p child is its only
	 * child, the node \p child becomes the new root of the tree. The parent then
	 * is completely removed from the tree and destroyed.
	 *
	 * Before applying the function #exchangeNodes(), the function #removeChildFromSiblings()
	 * is applied. This is useful in case
	 * the node \p parent has some ignored children and has to be reused
	 * within some extra algorithmic context.
	 *
	 * @return 1, if \p child was the only child of \p parent, otherwise 0
	 */
	virtual bool checkIfOnlyChild(
		PQNode<T,X,Y> *child,
		PQNode<T,X,Y> *parent);

	/**
	 * Marks a node as PQNodeRoot::PQNodeStatus::ToBeDeleted.
	 * This enables the function #emptyAllPertinentNodes()
	 * to remove the node and free its memory.
	 */
	virtual void destroyNode(PQNode<T,X,Y> *nodePtr) {
		nodePtr->status(PQNodeRoot::PQNodeStatus::ToBeDeleted);
	}

	/**
	 * Replaces the \p oldNode by the \p newNode in the tree.
	 * This is a function used very often in the template matchings,
	 * normally in combination with the construction of a new node which has to
	 * conquer the place of an existing node in the tree.
	 *
	 * This function can be used in all cases, so the parent of \p oldNode
	 * is allowed to be either a Q-node or a P-node and \p oldNode may be
	 * any child of its parent.
	 *
	 * The client should observe, that this function does \b not reset
	 * the pointer #m_root. If necessary, this has to be done explicitly by
	 * the client himself.
	 */
	virtual void exchangeNodes(
		PQNode<T,X,Y> *oldNode,
		PQNode<T,X,Y> *newNode);

	/**
	 * Links the two endmost children of two \b different Q-nodes
	 * via their sibling pointers together.
	 * The purpose of doing this is to combine the children of two Q-nodes
	 * as children of only one Q-node. This function does not reset the
	 * pointers to the endmost children of the Q-node. This has to be done
	 * by the client of the function.
	 */
	virtual void linkChildrenOfQnode(
		PQNode<T,X,Y> *installed,
		PQNode<T,X,Y> *newChild);

	/**
	 * Removes the node \p nodePtr from
	 * the doubly linked list of its parent. In case that \p nodePtr is endmost
	 * child of an Q-node or child of a P-node equiped with a valid
	 * reference pointer \a referenceParent to its parent (see PQNode),
	 * these pointers are considered as
	 * well and the adjacent siblings of \p nodePtr have to cover
	 * \p nodePtr's task.
	 */
	virtual void removeChildFromSiblings(PQNode<T,X,Y>* nodePtr);

	/**
	 * The objective is to remove a node \p child from the PQ-tree. To do so,
	 * the \p parent of the node \p child has to be known by the user.
	 * To indicate this, the parent has to be handed over by her.
	 *
	 * This function has to be handled with great care by the user.
	 * It is not used in any of the functions of the class template PQTree
	 * and can only be accessed by inheritance.
	 *
	 * <b>This function does not check if \p parent is the parent node of
	 * \p child</b>. This has to be guaranteed by the user. The reason for
	 * this riscfull approach lies in the details of the powerful data structure
	 * PQ-tree. In order to reach linear runtime, the internal children
	 * of a Q-node normally do not have valid parent pointers. So forcing
	 * this function to search the parent would cost in worst case
	 * linear runtime for one call of the function removeNodeFromTree().
	 * Its up to the user to do better.
	 *
	 * Calling removeNodeFromTree() with nullptr for \p parent,
	 * will always terminate this function with an ERROR-message and returning
	 * -1 as value.
	 *
	 * The return value is an integer value used to indicate how many children
	 * the \p parent after the removal of \p child still has. The client should
	 * observe that internal nodes in the PQ-tree which have just one or
	 * no children at all do not make sense. However, the function
	 * removeNodeFromTree() <b>does not check if \p parent has less than
	 * two children after the removal of \< child</b>.
	 * So in case that \p parent has less than two children, the user has to check
	 * this by herself and remove the \p parent, probably using the function
	 * checkIfOnlyChild().
	 *
	 * There are two reasons why the function removeNodeFromTree() does
	 * not check if \p parent has  less than two children after the removal
	 * of \p child:
	 *   -# The user might keep the node in the tree in order to add new nodes
	 *      as children to it.
	 *   -# Again, the parent of \p parent might not be known to \p parent,
	 *      hence removeNodeTree() would have to search, at the cost of linear time
	 *      consumption, for the parent of \p parent first before removing
	 *      \p parent from the tree.
	 *
	 * Observe that removeNodeFromTree() does not free the allocated
	 * memory of \p child. This has to be done by the user \b after calling
	 * removeNodeFromTree(). It also offers the opportunity to reuse
	 * deleted nodes. Observe that the identification number of a node
	 * #m_identificationNumber (see PQNode) cannot be changed.
	 */
	virtual int  removeNodeFromTree(
		PQNode<T,X,Y>* parent,
		PQNode<T,X,Y>* child);


	List<PQNode<T,X,Y>*>* fullChildren(PQNode<T,X,Y>* nodePtr) {
		return nodePtr->fullChildren;
	}


	List<PQNode<T,X,Y>*>*  partialChildren(PQNode<T,X,Y>* nodePtr) {
		return nodePtr->partialChildren;
	}

	virtual PQNode<T,X,Y>* clientLeftEndmost(PQNode<T,X,Y>* nodePtr) const {
		return nodePtr->m_leftEndmost;
	}

	virtual PQNode<T,X,Y>* clientRightEndmost(PQNode<T,X,Y>* nodePtr) const {
		return nodePtr->m_rightEndmost;
	}

	virtual PQNode<T,X,Y>* clientNextSib(PQNode<T,X,Y>* nodePtr,
		PQNode<T,X,Y>* other) const {
			return nodePtr->getNextSib(other);
	}

	virtual PQNode<T,X,Y>* clientSibLeft(PQNode<T,X,Y>* nodePtr) const {
		return nodePtr->m_sibLeft;
	}

	virtual PQNode<T,X,Y>* clientSibRight(PQNode<T,X,Y>* nodePtr) const {
		return nodePtr->m_sibRight;
	}

	/**
	 * If the user wishes to use different flags in a derived class of PQTree
	 * that are not available in this implementation, he can overload this function.
	 *
	 * With the help of this function it is possible to influence the layout of the nodes
	 * by using new, different lables depicting node categories in the <em>Tree Interface</em>.
	 */
	virtual int clientPrintNodeCategorie(PQNode<T,X,Y>* nodePtr);

	/**
	 * If the user wishes to use different status in a derived class of PQTree
	 * that are not available in this implementation, he can overload this function.
	 *
	 * With the help of this function it is possible to influence the information stored
	 * at nodes in the <em>Tree Interface</em> that concern the status of a node.
	 */
	virtual const char* clientPrintStatus(PQNode<T,X,Y>* nodePtr);

	/**
	 * If the user wishes to use different types in a derived class of PQTree
	 * that are not available in this implementation, he can overload this function.
	 *
	 * With the help of this function it is possible to influence the information stored
	 * at nodes in the <em>Tree Interface</em> that concern the type of a node.
	 */
	virtual const char* clientPrintType(PQNode<T,X,Y>* nodePtr);

private:
	/**
	 * Checks whether all full children of a Q-node
	 * \p nodePtr form a consecutive sequence. If the full nodes do so,
	 * the procedure returns 1 as a result, otherwise 0.
	 *
	 * The pointer \p firstFull denotes just an arbirtary full child. Starting
	 * from this position, checkChain sweeps through the consecutive
	 * sequence, halting as soon as a nonfull child is detected.
	 * The two pointers \p seqStart and \p seqEnd are set within this function.
	 * They denote the first and last node of the consecutive sequence.
	 *
	 * The client should observe that it is not possible to avoid the use
	 * of such a function. According to the procedure Bubble() children of
	 * Q-nodes get unblocked as soon as they are adjacent to any pertinent
	 * sibling. This includes that chains of more than two partial children
	 * are regarded as unblocked as well.
	 * Such chains are of course not reducible and therefore
	 * have to be detected by the function checkChain().
	 *
	 * The function is used by the function #templateQ2() and #templateQ3().
	 */
	bool checkChain(
		PQNode<T,X,Y> *nodePtr,
		PQNode<T,X,Y> *firstFull,
		PQNode<T,X,Y> **seqStart,
		PQNode<T,X,Y> **seqEnd);

	/**
	 * Copies all full children of \p nodePtr to a new P-node
	 * The node \p nodePtr has to be a P-node. The new P-node
	 * is added to \p partialChild as an endmost child of \p partialChild.
	 * The node \p partialChild has to be a Q-node and the new P-node is added
	 * to the side of \p partialChild where the pertinent children are.
	 *
	 * The new P-node is allocated by this function and referenced by the
	 * variable \p newNode.
	 *
	 * The function copyFullChildrenToPartial() is used by the functions
	 * #templateP4(), #templateP5(), and #templateP6().
	 */
	void copyFullChildrenToPartial(
		PQNode<T,X,Y> *nodePtr,
		PQNode<T,X,Y> *partialChild);

	/**
	 * Copies the full children of a P-node that are stored in the stack
	 * \p fullNodes to a new P-node.
	 *
	 * This new P-node is created by the function and stored in \a newNode
	 * if there is more than one full child.
	 * If there is just one full child, it is not necessary to construct a new
	 * P-node and the full child is stored in \a newNode.
	 * The \a newNode is the return value of the procedure.
	 *
	 * This function is used by
	 * #templateP2(), #templateP3() and #copyFullChildrenToPartial().
	 */
	PQNode<T,X,Y>* createNodeAndCopyFullChildren(List<PQNode<T,X,Y>*> *fullNodes);

	/**
	 * Of every partial node that is found in the sequence of children of
	 * \p nodePtr, all children are removed from that partial node and included
	 * as children of \p nodePtr, occupying the place of the partial node in
	 * the sequence of children of \p nodePtr. Thereby, removeBlock() takes
	 * care, that the newly included full children of \p nodePtr form
	 * a consecutive sequence with the already existing pertinent children of
	 * \p nodePtr. The partial node itself is deleted afterwards.
	 *
	 * This function is used by the functions #templateQ2() and #templateQ3().
	 *
	 * The node \p nodePtr is expected to be a
	 * Q-node with no, one or at most two partial children, such that
	 * the partial and full children of \p nodePtr form a legal consecutive
	 * sequence, hence can be reduced.
	 */
	void removeBlock(PQNode<T,X,Y> *nodePtr, bool isRoot);

	/**
	 * Sorts the exceptions before frontExcept() scans the frontier.
	 * It is only called by the function frontExcept().
	 */
	void sortExceptions(int Exceptions[], int arraySize);
};

template<class T,class X,class Y>
bool PQTree<T,X,Y>::addNewLeavesToTree(
	PQInternalNode<T,X,Y> *father,
	SListPure<PQLeafKey<T,X,Y>*> &leafKeys)
{
	if (!leafKeys.empty())
	{
		OGDF_ASSERT(!father->m_childCount);
		// Father has children. Brothers expected

		/// Enter the first element as PQLeaf to the [[parent]].
		SListIterator<PQLeafKey<T,X,Y>*> it = leafKeys.begin();
		PQLeafKey<T,X,Y>* newKey = *it; //leafKeys[0];

		PQNode<T,X,Y>* aktualSon = new PQLeaf<T,X,Y>(m_identificationNumber++,PQNodeRoot::PQNodeStatus::Empty,newKey);
		PQNode<T,X,Y>* firstSon = aktualSon;
		firstSon->m_parent = father;
		firstSon->m_parentType = father->type();
		father->m_childCount++;
		PQNode<T,X,Y>* oldSon = firstSon;

		/// Enter all other elements as leaves to [[parent]].
		for (++it; it.valid(); ++it)
		{
			newKey = *it; //leafKeys[i];
			aktualSon = new PQLeaf<T,X,Y>(m_identificationNumber++, PQNodeRoot::PQNodeStatus::Empty,newKey);
			aktualSon->m_parent = father;
			aktualSon->m_parentType = father->type();
			father->m_childCount++;
			oldSon->m_sibRight = aktualSon;
			aktualSon->m_sibLeft = oldSon;
			oldSon = aktualSon;
		}
		if (father->type() == PQNodeRoot::PQNodeType::PNode)
		/// Set the reference pointers if [[parent]] is a $P$-node.
		{
			firstSon->m_sibLeft = oldSon;
			oldSon->m_sibRight = firstSon;
			father->m_referenceChild = firstSon;
			firstSon->m_referenceParent = father;
		}
		else if (father->type() == PQNodeRoot::PQNodeType::QNode)
		/// Set the endmost children if [[parent is a $Q$-node.
		{
			father->m_leftEndmost = firstSon;
			father->m_rightEndmost = oldSon;
		}
		return true;
	}

	return false;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::addNodeToNewParent(
	PQNode<T,X,Y>* parent,
	PQNode<T,X,Y>* child)
{
	OGDF_ASSERT(parent->type() == PQNodeRoot::PQNodeType::PNode);
	OGDF_ASSERT(parent->type() == PQNodeRoot::PQNodeType::QNode);
		//parent type not valid.

	if (child != nullptr)
	{
		OGDF_ASSERT(parent->m_childCount == 0);
			//when adding new nodes: Brothers expected.
		child->m_parent = parent;
		child->m_parentType = parent->type();
		parent->m_childCount++;

		/*
		Set the reference pointers in case that [[parent]] is a $P$-node.
		If [[parent]] is a $Q$-node, this chunk sets the endmost children
		of [[parent]]. Since [[child]] is the only child of [[parent]]
		both endmost pointers are set to [[child]].
		*/
		if (parent->type() == PQNodeRoot::PQNodeType::PNode)
		{
			child->m_sibLeft = child;
			child->m_sibRight = child;
			parent->m_referenceChild = child;
			child->m_referenceParent = parent;
		}
		else if (parent->type() == PQNodeRoot::PQNodeType::QNode)
		{
			parent->m_leftEndmost = child;
			parent->m_rightEndmost = child;
		}

		return true;
	}

	return false;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::addNodeToNewParent(
	PQNode<T,X,Y>* parent,
	PQNode<T,X,Y>* child,
	PQNode<T,X,Y>* leftBrother,
	PQNode<T,X,Y>* rightBrother)
{

	if (parent != nullptr)
	{
		OGDF_ASSERT(parent->type() == PQNodeRoot::PQNodeType::PNode || parent->type() == PQNodeRoot::PQNodeType::QNode);
		//parent type not valid
		if (leftBrother == nullptr && rightBrother == nullptr)
			return addNodeToNewParent(parent,child);
		else if (child != nullptr)
		{
			child->m_parent = parent;
			child->m_parentType = parent->type();
			parent->m_childCount++;

			if (parent->type() == PQNodeRoot::PQNodeType::PNode)
			{
				/*
				The parent is a $P$-node with children.
				Either [[leftBrother]] or [[rightBrother]] stores
				a pointer to an existing child of [[parent]] and [[parent]]
				is a $P$-node. In case that two brothers are stored, an
				arbitrary one is choosen to be the next sibling of [[child]].
				This brother is stored in [[brother]]. The pointer [[sister]]
				denotes a pointer to an arbitrary sibling of [[brother]].
				*/
				PQNode<T,X,Y>* brother = (leftBrother != nullptr) ? leftBrother : rightBrother;
				PQNode<T,X,Y>* sister = brother->m_sibRight;
				child->m_sibLeft = brother;
				child->m_sibRight = sister;
				brother->m_sibRight = child;
				sister->m_sibLeft = child;
				return true;
			}

			else if (leftBrother == nullptr)
			{
				/*
				The parent is a $Q$-node with children.
				The [[leftBrother]] is a [[0]]-pointer while the
				[[rightBrother]] denotes an existing child of [[parent]].
				The node [[rightBrother]] <b>must be</b> one of the two endmost
				children of [[parent]]. If this is not the case, the chunk
				detects this, halts the procedure [[addNewLeavesToTree]]
				printing an error message and returning [[0]].
				If [[rightBrother]] is endmost child of [[parent]], then
				this chunk adds [[child]] at the one end where
				[[rightBrother]] hides. The node [[child]] is then made the
				new endmost child of [[parent]] on the corresponding side.
				*/
				if (rightBrother == parent->m_leftEndmost)
				{
					parent->m_leftEndmost = child;
					child->m_sibRight = rightBrother;
					rightBrother->putSibling(child,PQNodeRoot::SibDirection::Left);
					return true;
				}

				// missing second brother?
				OGDF_ASSERT(rightBrother == parent->m_rightEndmost);
				parent->m_rightEndmost = child;
				child->m_sibLeft = rightBrother;
				rightBrother->putSibling(child,PQNodeRoot::SibDirection::Left);
				return true;
			}

			else if (rightBrother == nullptr)
			{
				/*
				The parent is a $Q$-node with children.
				The [[rightBrother]] is a [[0]]-pointer while the
				[[leftBrother]] denotes an existing child of [[parent]]. The
				node [[leftBrother]] <b>must be</b> one of the two endmost
				children of [[parent]]. If this is not the case, the chunk
				detects this, halts the procedure [[addNodeToNewParent]]
				printing an error message and returning [[0]].
				If [[leftBrother]] is endmost child of [[parent]], then this
				chunk adds [[child]] at the one end where [[leftBrother]]
				hides. The node [[child]] is then made new endmost child of
				[[parent]] on the corresponding side.
				*/
				if (leftBrother == parent->m_rightEndmost)
				{
					parent->m_rightEndmost = child;
					child->m_sibLeft = leftBrother;
					leftBrother->putSibling(child,PQNodeRoot::SibDirection::Right);
					return true;
				}

				// missing second brother?
				OGDF_ASSERT(leftBrother == parent->m_leftEndmost);
				parent->m_leftEndmost = child;
				child->m_sibRight = leftBrother;
				leftBrother->putSibling(child,PQNodeRoot::SibDirection::Right);
				return true;
			}

			else
			{
				/*
				The parent is a $Q$-node with children.
				Both the [[rightBrother]] and the [[leftBrother]] denote
				existing children of [[parent]]. In this case, [[leftBrother]]
				and [[rightBrother]] must be immideate siblings. If this is
				not the case, this will be detected during the function call
				[[changeSiblings]] of the class [[PQNode.h]] (see
				\ref{PQNode.changeSiblings}) in the first two lines of this
				chunk. If the chunk recognizes the failure of
				[[changeSiblings]] it halts the procedure
				[[addNewLeavesToTree]], printing an error message and
				returning [[0]].
				If the two brothers are immediate siblings, this chunk
				adds [[child]] between the two brothers as interior child of
				the $Q$-node [[parent]].
				*/
#ifdef OGDF_DEBUG
				bool ok =
#endif
					rightBrother->changeSiblings(leftBrother,child) && leftBrother->changeSiblings(rightBrother,child);

				// brothers are not siblings?
				OGDF_ASSERT(ok);

				if (leftBrother->m_sibRight == child)
				{
					child->m_sibLeft = leftBrother;
					child->m_sibRight = rightBrother;
				}
				else
				{
					child->m_sibLeft = rightBrother;
					child->m_sibRight = leftBrother;
				}
				return true;
			}
		}
		else
			return false;
	}
	else if (leftBrother != nullptr && rightBrother != nullptr)
	{
		/*
		The parent is a $Q$-node with children.
		Both the [[rightBrother]] and the [[leftBrother]] denote
		existing children of [[parent]]. In this case, [[leftBrother]]
		and [[rightBrother]] must be immideate siblings. If this is
		not the case, this will be detected during the function call
		[[changeSiblings]] of the class [[PQNode.h]] (see
		\ref{PQNode.changeSiblings}) in the first two lines of this
		chunk. If the chunk recognizes the failure of
		[[changeSiblings]] it halts the procedure
		[[addNewLeavesToTree]], printing an error message and
		returning [[0]].
		If the two brothers are immediate siblings, this chunk
		adds [[child]] between the two brothers as interior child of
		the $Q$-node [[parent]].
		*/
#ifdef OGDF_DEBUG
		bool ok =
#endif
			rightBrother->changeSiblings(leftBrother,child) && leftBrother->changeSiblings(rightBrother,child);

		// brothers are not siblings?
		OGDF_ASSERT(ok);

		if (leftBrother->m_sibRight == child)
		{
			child->m_sibLeft = leftBrother;
			child->m_sibRight = rightBrother;
		}
		else
		{
			child->m_sibLeft = rightBrother;
			child->m_sibRight = leftBrother;
		}
		return true;
	}

	return true;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::Bubble(SListPure<PQLeafKey<T,X,Y>*> &leafKeys)
{
	/* Variables:
	 *   - \a blockcount is the number of blocks of blocked nodes during
	 *     the bubbling up phase.
	 *   - \a numBlocked is the number of blocked nodes during the
	 *     bubbling up phase.
	 *   - \a blockedSiblings counts the number of blocked siblings that
	 *      are adjacent to \a checkNode. A node has 0, 1 or 2 blocked siblings.
	 *      A child of a P-node has no blocked siblings. Endmost children of
	 *      Q-nodes have at most 1 blocked sibling. The interior children of
	 *      a Q-Node have at most 2 blocked siblings.
	 *   - \a checkLeaf is a pointer used for finding the pertinent leaves.
	 *   - \a checkNode is a pointer to the actual node.
	 *   - \a checkSib is a pointer used to examin the siblings of [[checkNode]].
	 *   - \a offTheTop is a variable which is either 0 (that is its
	 *     initial value) or 1 in case that the root of the tree has been
	 *     process during the first phase.
	 *   - \a parent is a pointer to the parent of \a checkNode, if \a checkNode
	 *     has a valid parent pointer.
	 *   - \a processNodes is a first-in first-out list that is used for
	 *     sequencing the order in which the nodes are processed.
	 *   - \a blockedNodes is a stack storing all nodes that have been
	 *     once blocked. In case that the [[m_pseudoRoot]] has to be
	 *     introduced, the stack contains the blocked nodes.
	 */
	Queue<PQNode<T,X,Y>*> processNodes;

	/*
	Enter the [[Full]] leaves into the queue [[processNodes]].
	In a first step the pertinent leaves have to be identified in the tree
	and entered on to the queue [[processNodes]]. The identification of
	the leaves can be done with the help of a pointer stored in every
	[[PQLeafKey]] (see \ref{PQLeafKey}) in constant time for every element.
	*/
	SListIterator<PQLeafKey<T,X,Y>*> it;
	for (it  = leafKeys.begin(); it.valid(); ++it)
	{
		PQNode<T,X,Y>* checkLeaf = (*it)->nodePointer(); //leafKeys[i]->nodePointer();
		checkLeaf->mark(PQNodeRoot::PQNodeMark::Queued);
		processNodes.append(checkLeaf);
		m_pertinentNodes->pushFront(checkLeaf);
	}

	int blockCount       = 0;
	int numBlocked       = 0;
	int offTheTop        = 0;
	PQNode<T,X,Y>* checkSib = nullptr;
	ArrayBuffer<PQNode<T,X,Y>*> blockedNodes;

	while ((processNodes.size() + blockCount + offTheTop) > 1)
	{
		if (processNodes.size() == 0)
			/*
			No consecutive sequence possible.
			The queue [[processNodes]] does not contain any nodes for
			processing and the sum of [[blockCount]] and [[offTheTop]] is
			greater than 1. If the queue is empty, the root of the pertinent
			subtree was already processed. Nevertheless, there are blocked
			nodes since [[offTheTop]] is either be [[0]] or [[1]], hence
			[[blockCount]] must be at least [[1]]. Such blocked nodes cannot
			form a consecutive sequence with all nodes in the set
			[[leafKeys]]. Observe that this chunk finishes the function
			[[Bubble]]. Hence every memory allocated by the function [[Bubble]]
			has to be deleted here as well.
			*/
			return false;
		/*
		If there are still nodes to be processed in which case the queue
		[[processNodes]] is not empty, we get the next node from the queue.
		By default this node has to be marked as blocked.
		*/
		PQNode<T,X,Y>* checkNode = processNodes.pop();
		blockedNodes.push(checkNode);
		checkNode->mark(PQNodeRoot::PQNodeMark::Blocked);
		int blockedSiblings = 0;

		/*
		Check if node is adjacent to an unblocked node.
		After getting the node [[checkNode]] from the queue, its siblings are
		checked, whether they are unblocked. If they are, then they have a
		valid pointer to their parent and the parent pointer of [[checkNode]]
		is updated.
		*/
		if (checkNode->m_parentType != PQNodeRoot::PQNodeType::PNode && checkNode != m_root)
			// checkNode is son of a QNode.
			// Check if it is blocked.
		{
			if (clientSibLeft(checkNode) == nullptr)
				// checkNode is endmost child of
				// a QNode. It has a valid pointer
				// to its parent.
			{
				checkNode->mark(PQNodeRoot::PQNodeMark::Unblocked);
				if (clientSibRight(checkNode) &&
					clientSibRight(checkNode)->mark() == PQNodeRoot::PQNodeMark::Blocked)
					blockedSiblings++;
			}

			else if (clientSibRight(checkNode) == nullptr)
				// checkNode is endmost child of
				// a QNode. It has a valid pointer
				// to its parent.
			{
				checkNode->mark(PQNodeRoot::PQNodeMark::Unblocked);
				if (clientSibLeft(checkNode) &&
					clientSibLeft(checkNode)->mark() == PQNodeRoot::PQNodeMark::Blocked)
					blockedSiblings++;
			}


			else
				// checkNode is not endmost child of
				// a QNode. It has not a valid
				// pointer to its parent.
			{
				if (clientSibLeft(checkNode)->mark() == PQNodeRoot::PQNodeMark::Unblocked)
					// checkNode is adjacent to an
					// unblocked node. Take its parent.
				{
					checkNode->mark(PQNodeRoot::PQNodeMark::Unblocked);
					checkNode->m_parent = clientSibLeft(checkNode)->m_parent;
				}
				else if (clientSibLeft(checkNode)->mark() == PQNodeRoot::PQNodeMark::Blocked)
					blockedSiblings++;

				if (clientSibRight(checkNode)->mark() == PQNodeRoot::PQNodeMark::Unblocked)
					// checkNode is adjacent to an
					// unblocked node. Take its parent.
				{
					checkNode->mark(PQNodeRoot::PQNodeMark::Unblocked);
					checkNode->m_parent = clientSibRight(checkNode)->m_parent;
				}
				else if (clientSibRight(checkNode)->mark() == PQNodeRoot::PQNodeMark::Blocked)
					blockedSiblings++;
			}
		}

		else
			// checkNode is son of a PNode
			// and children of P_NODEs
			// cannot be blocked.
			checkNode->mark(PQNodeRoot::PQNodeMark::Unblocked);

		if (checkNode->mark() == PQNodeRoot::PQNodeMark::Unblocked)
		{
			PQNode<T,X,Y>* parent = checkNode->m_parent;

			/*
			Get maximal consecutive set of blocked siblings.
			This chunk belongs to the procedure [[bubble]].
			The node [[checkNode]] is [[Unblocked]].
			If the parent of [[checkNode]] is a $Q$-Node, then we check the
			siblings [[checkSib]] of [[checkNode]] whether they are
			[[Blocked]]. If they are blocked,  they have to be marked
			[[Unblocked]] since they are adjacent to the [[Unblocked]] node
			[[checkNode]]. We then have to proceed with the siblings of
			[[checkSib]] in order to find [[Blocked]] nodes
			adjacent to [[checkSib]]. This is repeated until no [[Blocked]]
			nodes are found any more.

			Observe that while running through the children of the $Q$-Node
			(referred by the pointer [[parent]]), their parent pointers,
			as well as the [[pertChildCount]] of [[parent]] are updated.
			Furthermore we reduce simultaneously the count [[numBlocked]].
			*/
			if (blockedSiblings > 0)
			{
				if (clientSibLeft(checkNode) != nullptr)
				{
					checkSib = clientSibLeft(checkNode);
					PQNode<T,X,Y>* oldSib = checkNode;
					while (checkSib->mark() == PQNodeRoot::PQNodeMark::Blocked)
					{
						checkSib->mark(PQNodeRoot::PQNodeMark::Unblocked);
						checkSib->m_parent = parent;
						numBlocked--;
						parent->m_pertChildCount++;
						PQNode<T,X,Y>* holdSib = clientNextSib(checkSib,oldSib);
						oldSib = checkSib;
						checkSib = holdSib;
						// Blocked node as endmost child of a QNode.
					}
				}

				if (clientSibRight(checkNode) != nullptr)
				{
					checkSib = clientSibRight(checkNode);
					PQNode<T,X,Y>* oldSib = checkNode;
					while (checkSib->mark() == PQNodeRoot::PQNodeMark::Blocked)
					{
						checkSib->mark(PQNodeRoot::PQNodeMark::Unblocked);
						checkSib->m_parent = parent;
						numBlocked--;
						parent->m_pertChildCount++;
						PQNode<T,X,Y>* holdSib = clientNextSib(checkSib,oldSib);
						oldSib = checkSib;
						checkSib = holdSib;
						// Blocked node as endmost child of a QNode.
					}
				}
			}

			/*
			Process parent of [[checkNode]]
			After processing the siblings of the [[Unblocked]] [[checkNode]]
			the parent has to be processed. If [[checkNode]] is the root
			of the tree we do nothing except setting the flag [[offTheTop]].
			If it is not the root and [[parent]] has not been placed onto the
			queue [[processNodes]], the [[parent]] is placed on to
			[[processNodes]].

			Observe that the number [[blockCount]] is updated. Since
			[[checkNode]] was [[Unblocked]] all pertinent nodes adjacent
			to that node became [[Unblocked]] as well. Therefore the number
			of blocks is reduced by the number of [[Blocked]] siblings of
			[[checkNode]].
			*/
			if (parent == nullptr)
				// checkNode is root of the tree.
				offTheTop = 1;
			else
				// checkNode is not the root.
			{
				parent->m_pertChildCount++;
				if (parent->mark() == PQNodeRoot::PQNodeMark::Unmarked)
				{
					processNodes.append(parent);
					m_pertinentNodes->pushFront(parent);
					parent->mark(PQNodeRoot::PQNodeMark::Queued);
				}
			}

			blockCount -= blockedSiblings;
			blockedSiblings = 0;
		} else {
			/*
			Process blocked [[checkNode]]
			Since [[checkNode]] is [[Blocked]], we cannot continue
			processing at this point in the Tree. We have to wait until
			this node becomes unblocked. So only the variables
			[[blockCount]] and [[numBlocked]] are updated.
			*/
			blockCount += 1 - blockedSiblings;
			numBlocked++;
		}
	}

	if (blockCount == 1)
	{
		/*
		If [[blockCount]] $= 1$ enter [[m_pseudoRoot]] to the tree
		In case that the root of the pertinent subtree is a $Q$-node
		with empty children on both sides and the pertinent children
		in the middle, it is possible that the $PQ$-tree is reducible.
		But since the sequence of pertinent children of the $Q$-node is
		blocked, the procedure is not able to find the parent of its
		pertinent children. This is due to the fact that the interior
		children of a $Q$-node do not have a valid parent pointer.

		So the root of the pertinent subtree is not known, hence cannot be
		entered into the processing queue used in the function call [[Reduce]]
		(see \ref{Reduce}). To solve this problem a special node only designed
		for this cases is used: [[m_pseudoRoot]]. It simulates the root of the
		pertinent subtree. This works out well, since for this node the only
		possible template maching is [[templateQ3]] (see \ref{templateQ3}),
		where no pointers to the endmost children of a $Q$-node are used.
		*/
		while (!blockedNodes.empty())
		{
			PQNode<T,X,Y>* checkNode = blockedNodes.popRet();
			if (checkNode->mark() == PQNodeRoot::PQNodeMark::Blocked)
			{
				checkNode->mark(PQNodeRoot::PQNodeMark::Unblocked);
				checkNode->m_parent = m_pseudoRoot;
				m_pseudoRoot->m_pertChildCount++;
				OGDF_ASSERT(!checkNode->endmostChild());
					//Blocked node as endmost child of a QNode.
			}
		}
	}

	return true;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::checkChain(
	PQNode<T,X,Y> *nodePtr,
	PQNode<T,X,Y> *firstFull,
	PQNode<T,X,Y> **seqStart,
	PQNode<T,X,Y> **seqEnd)
{
	/* Variables:
	 *   - \a fullCount counts the number of children that are
	 *     discovered by the function checkChain(). This is necessary, since
	 *     checkChain() is used by two template matching functions
	 *     templateQ2() and templateQ3() where in the latter case the
	 *     pointer \a firstFull may point to any full child in the front of the Q-node
	 *     \a nodePtr.
	 *   - \a notFull is set 1 when an empty child is encountered.
	 *   - \a checkNode is the actual node that is examined.
	 *   - \a leftNext is the next node that has to be examined on the
	 *     left side of \a firstFull.
	 *   - \a leftOld is the node that has been examined right before
	 *     \a checkNode on the left side of \a firstFull.
	 *   - \a rightNext is the next node that has to be examined on the
	 *     right side of \a firstFull. Not needed when checkChain() was
	 *     called by templateQ2().
	 *   - \a rightOld is the node that has been examined right before
	 *     \a checkNode on the right side of \a firstFull.
	 *     Not needed when checkChain() was called by templateQ2().
	*/
	bool notFull = false;
	int fullCount = nodePtr->fullChildren->size();
	fullCount--;                    // firstFull does have a Full label.

	/*
	Start at the [[firstFull]] child sweeping through the full children on
	the left side of [[firstfull]]. It stops as soon as a nonfull child is
	detected. The last found full child is stored in [[seqEnd]].
	*/
	PQNode<T,X,Y> *leftNext = clientSibLeft(firstFull);
	(*seqEnd) = firstFull;
	if (leftNext != nullptr)
	{
		if (leftNext->status() == PQNodeRoot::PQNodeStatus::Full) {
			fullCount--;

			PQNode<T,X,Y> *leftOld = firstFull;
			PQNode<T,X,Y> *checkNode = leftNext;

			while (fullCount > 0 && !notFull)
				// There are still full children to be
				// counted, and no empty child has been
				// encountered on this side.
			{
				leftNext = clientNextSib(checkNode,leftOld);
				if (leftNext != nullptr && leftNext->status() == PQNodeRoot::PQNodeStatus::Full)
					fullCount--;
				else
					notFull = true;
				leftOld = checkNode;
				checkNode = leftNext;
			}

			if (checkNode != nullptr && checkNode->status() == PQNodeRoot::PQNodeStatus::Full)
				(*seqEnd)  = checkNode;

			else {
				//searching consecutive sequence in Q2 or Q3.
				OGDF_ASSERT(leftOld != nullptr);
				OGDF_ASSERT(leftOld->status() == PQNodeRoot::PQNodeStatus::Full);
				(*seqEnd) = leftOld;
			}

		} else {
			(*seqEnd) = firstFull;
		}
	}

	/*
	Start at the [[firstFull]] child sweeping through the full children on
	the right side of [[firstfull]]. It stops as soon as a nonfull child is
	detected.
	*/
	notFull = false;
	PQNode<T,X,Y> *rightNext = clientSibRight(firstFull);
	(*seqStart) = firstFull;
	if(rightNext != nullptr)
	{
		if (rightNext->status() == PQNodeRoot::PQNodeStatus::Full) {
			fullCount--;

			PQNode<T,X,Y> *rightOld = firstFull;
			PQNode<T,X,Y> *checkNode = rightNext;

			while (fullCount > 0 && !notFull)
				// There are still full children to be
				// counted, and no empty child has been
				// encountered on this side.
			{
				rightNext = clientNextSib(checkNode,rightOld);
				if (rightNext != nullptr && rightNext->status() == PQNodeRoot::PQNodeStatus::Full)
					fullCount--;
				else
					notFull = true;
				rightOld = checkNode;
				checkNode = rightNext;
			}
			if (checkNode != nullptr && checkNode->status() == PQNodeRoot::PQNodeStatus::Full)
				(*seqStart) = checkNode;

			else {
				OGDF_ASSERT(rightOld != nullptr);
				OGDF_ASSERT(rightOld->status() == PQNodeRoot::PQNodeStatus::Full);
				(*seqStart) = rightOld;
				//searching consecutive seqeuence in Q2 or Q3.
			}

		} else {
			(*seqStart) = firstFull;
		}
	}

	if (firstFull == (*seqEnd))
	{
		PQNode<T,X,Y> *checkNode = (*seqEnd);
		(*seqEnd) = (*seqStart);
		(*seqStart) = checkNode;
	}

	if (fullCount == 0)
		// All full children occupy a consecutive
		// sequence.
		return true;
	else
		return false;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::checkIfOnlyChild(
	PQNode<T,X,Y> *child,
	PQNode<T,X,Y> *parent)
{
	if ((parent->type() == PQNodeRoot::PQNodeType::PNode && parent->m_childCount == 1)
		|| (parent->type() == PQNodeRoot::PQNodeType::QNode && parent->m_leftEndmost == child
			&& parent->m_rightEndmost == child))
	{
		removeChildFromSiblings(child);
		child->m_parent = parent->m_parent;
		if (parent->m_parent != nullptr)         // parent is not the root.
			exchangeNodes(parent,child);
		else
		{
			exchangeNodes(parent,child);
			m_root = child;
		}
		destroyNode(parent);
		return true;
	}
	else
		return false;
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::Cleanup()
{
	/* In order to free the allocated memory, all nodes of the
	 * tree have to be deleted, hence there destructors are called.
	 * In order to achieve this, we start at the root of the tree and go down the
	 * tree to the leaves for reaching every node. When a node is processed,
	 * (besides the #m_root, this will always be the node \a checkNode)
	 * the pointers of all its children are stored in a queue \a helpqueue and
	 * then the processed node is deleted.
	 *
	 * The use of a queue \a helpqueue is a must, since the nodes do not
	 * have pointers to all of their children, as the children mostly do
	 * not have a pointer to their parent.
	 *
	 * It might look weird at the first glance that the function Cleanup()
	 * calls the function emptyAllPertinentNodes(), but if some nodes were removed
	 * during a reduction, they were stored in the stack #m_pertinentNodes.
	 * These nodes have to be deleted as well
	 * which is provided by the function emptyAllPertinentNodes().
	 */
	PQNode<T,X,Y>*  nextSon   = nullptr;
	PQNode<T,X,Y>*  lastSon   = nullptr;

	Queue<PQNode<T,X,Y>*> helpqueue;

	if (m_root != nullptr)
	{
		emptyAllPertinentNodes();
		PQNode<T,X,Y>*  oldSib = nullptr;

		/*
		Process the [[m_root]] of the [[PQTree]]. Before deleting [[m_root]],
		pointers to all its children are stored in the queue [[helpqueue]].
		*/
		if (m_root->type() == PQNodeRoot::PQNodeType::PNode)
		{
			if (m_root->m_referenceChild != nullptr)
			{
				PQNode<T,X,Y>* firstSon = m_root->m_referenceChild;
				helpqueue.append(firstSon);

				if (firstSon->m_sibRight != nullptr)
					nextSon = firstSon->m_sibRight;
				while (nextSon != firstSon)
				{
					helpqueue.append(nextSon);
					nextSon = nextSon->m_sibRight;
				}
			}
		}
		else if (m_root->type() == PQNodeRoot::PQNodeType::QNode)
		{
			PQNode<T,X,Y>* firstSon = m_root->m_leftEndmost;
			helpqueue.append(firstSon);

			lastSon = m_root->m_rightEndmost;
			helpqueue.append(lastSon);

			nextSon = lastSon->getNextSib(oldSib);
			oldSib = lastSon;
			while (nextSon != firstSon)
			{
				helpqueue.append(nextSon);
				PQNode<T,X,Y>* holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
			}
		}


		CleanNode(m_root);
		delete m_root;

		while (!helpqueue.empty())
		{
			PQNode<T,X,Y>* checkNode = helpqueue.pop();

			/*
			Process an arbitrary node [[checkNode]] of the [[PQTree]].
			Before deleting [[checkNode]],
			pointers to all its children are stored in the queue [[helpqueue]].
			*/
			if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
			{
				if (checkNode->m_referenceChild != nullptr)
				{
					PQNode<T,X,Y>* firstSon = checkNode->m_referenceChild;
					helpqueue.append(firstSon);

					if (firstSon->m_sibRight != nullptr)
						nextSon = firstSon->m_sibRight;
					while (nextSon != firstSon)
					{
						helpqueue.append(nextSon);
						nextSon = nextSon->m_sibRight;
					}
				}
			}
			else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
			{
				oldSib = nullptr;

				PQNode<T,X,Y>*firstSon = checkNode->m_leftEndmost;
				helpqueue.append(firstSon);

				lastSon = checkNode->m_rightEndmost;
				helpqueue.append(lastSon);

				nextSon = lastSon->getNextSib(oldSib);
				oldSib = lastSon;
				while (nextSon != firstSon)
				{
					helpqueue.append(nextSon);
					PQNode<T,X,Y>* holdSib = nextSon->getNextSib(oldSib);
					oldSib = nextSon;
					nextSon = holdSib;
				}
			}

			CleanNode(checkNode);
			delete checkNode;
		}
	}

	CleanNode(m_pseudoRoot);
	delete m_pseudoRoot;

	delete m_pertinentNodes;

	m_root = nullptr;
	m_pertinentRoot = nullptr;
	m_pseudoRoot = nullptr;
	m_pertinentNodes = nullptr;

	m_numberOfLeaves = 0;
	m_identificationNumber = 0;
}

template<class T,class X,class Y>
int PQTree<T,X,Y>::clientPrintNodeCategorie(PQNode<T,X,Y>* nodePtr)
{
	return (nodePtr != nullptr) ? 1 : 0;
		// 1 is the standard node categrie in the Tree Interface.
}

template<class T,class X,class Y>
const char* PQTree<T,X,Y>::clientPrintStatus(PQNode<T,X,Y>* nodePtr)
{
	return (nodePtr != nullptr) ? "ERROR" : "ERROR: clientPrintStatus: NO NODE ACCESSED";
}

template<class T,class X,class Y>
const char*  PQTree<T,X,Y>::clientPrintType(PQNode<T,X,Y>* nodePtr)
{
	return (nodePtr != nullptr) ? "ERROR" : "ERROR: clientPrintType: NO NODE ACCESSED";
}

template<class T,class X,class Y> PQTree<T,X,Y>::PQTree()
{
	m_root = nullptr;
	m_pertinentRoot = nullptr;
	m_pseudoRoot = nullptr;

	m_numberOfLeaves = 0;
	m_identificationNumber = 0;

	m_pertinentNodes = nullptr;
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::copyFullChildrenToPartial(
	PQNode<T,X,Y> *nodePtr,
	PQNode<T,X,Y> *partialChild)
{
	if (nodePtr->fullChildren->size() > 0)
		// There are some full children to
		// be copied.
	{
		nodePtr->m_childCount = nodePtr->m_childCount -
			nodePtr->fullChildren->size();

		PQNode<T,X,Y> *newNode = createNodeAndCopyFullChildren(nodePtr->fullChildren);

		// Introduce newNode as endmost
		// child of the partial Q-node.
		partialChild->m_childCount++;
		partialChild->fullChildren->pushFront(newNode);

		if (clientLeftEndmost(partialChild)->status() == PQNodeRoot::PQNodeStatus::Full)
		{
			PQNode<T,X,Y> *checkNode = partialChild->m_leftEndmost;
			partialChild->m_leftEndmost = newNode;
			linkChildrenOfQnode(checkNode,newNode);

		}
		else {
			// ERROR: Endmostchild not found?
			OGDF_ASSERT(clientRightEndmost(partialChild)->status() == PQNodeRoot::PQNodeStatus::Full);

			PQNode<T,X,Y> *checkNode = partialChild->m_rightEndmost;
			partialChild->m_rightEndmost = newNode;
			linkChildrenOfQnode(checkNode,newNode);
		}

		newNode->m_parent = partialChild;
		newNode->m_parentType = PQNodeRoot::PQNodeType::QNode;
	}
}

template<class T,class X,class Y>
PQNode<T,X,Y>* PQTree<T,X,Y>::createNodeAndCopyFullChildren(
								List<PQNode<T,X,Y>*> *fullNodes)
{
	/* Variables:
	 *   - \a newNode stores the adress of the new allocated P-node or
	 *     the adress of the only full child.
	 *   - \a oldSon is a variable used for adding the full nodes as
	 *     children to the new P-node.
	 *   - \a firstSon stores the adress of the first detected full
	 *     child. It is needed  for adding the full nodes as
	 *     children to the new P-node.
	 *   - \a checkSon is a variable used for adding the full nodes as
	 *     children to the new P-node.
	 *   - \a newPQnode is used for proper allocation of the new P-node.
	 */
	PQNode<T,X,Y>  *newNode    = nullptr;

	if (fullNodes->size() == 1)
	{
		/*
		There is just one full child. So no new $P$-node is created. The
		full child is copied as child to the [[partialChild]].
		*/
		newNode = fullNodes->popFrontRet();
		removeChildFromSiblings(newNode);
	}

	else
	{
		/*
		This chunk belongs to the function [[createNodeAndCopyFullChildren]].
		There is more than one full child, so a new $P$-node is created.
		This chunk first allocates the memory for the new $P$-node that will
		be stored in [[newNode]]. Then it pops the nodes out of the stack
		[[fullNodes]] and introduces them as sons of [[newNode]].
		Popping the nodes out of the stack implies at the same time
		that they are removed from the [[fullChildren]] stack of the
		$P$-node of their parent.
		*/
		newNode = new PQInternalNode<T,X,Y>(m_identificationNumber++, PQNodeRoot::PQNodeType::PNode, PQNodeRoot::PQNodeStatus::Full);
		m_pertinentNodes->pushFront(newNode);
		newNode->m_pertChildCount = fullNodes->size();
		newNode->m_childCount = fullNodes->size();

		/*
		The first node is copied separately, since we need the pointer to it
		for setting the pointers to the siblings of the next full nodes.
		*/
		PQNode<T,X,Y> *firstSon = fullNodes->popFrontRet();
		removeChildFromSiblings(firstSon);
		newNode->fullChildren->pushFront(firstSon);
		firstSon->m_parent = newNode;
		firstSon->m_parentType = newNode->type();
		PQNode<T,X,Y> *oldSon = firstSon;


		/*
		All remaining nodes that are stored in the stack [[fullNodes]] are
		introduced as children of the new $P$-node [[newNode]]. Observe
		that the children of a $P$-node must form a doubly linked list.
		Hence the last node and the [[firstSon]] must be linked via their
		siblings pointers.
		*/
		while (!fullNodes->empty())
		{
			PQNode<T,X,Y> *checkSon = fullNodes->popFrontRet();
			removeChildFromSiblings(checkSon);
			newNode->fullChildren->pushFront(checkSon);
			oldSon->m_sibRight = checkSon;
			checkSon->m_sibLeft = oldSon;
			checkSon->m_parent = newNode;
			checkSon->m_parentType = newNode->type();
			oldSon = checkSon;
		}
		firstSon->m_sibLeft = oldSon;
		oldSon->m_sibRight = firstSon;
		newNode->m_referenceChild = firstSon;
		firstSon->m_referenceParent = newNode;
	}

	return newNode;
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::emptyAllPertinentNodes()
{
	while(!m_pertinentNodes->empty())
	{
		PQNode<T, X, Y> *nodePtr = m_pertinentNodes->popFrontRet();
		switch (nodePtr->status())
		{
			case PQNodeRoot::PQNodeStatus::ToBeDeleted:
				if (nodePtr == m_root)
					m_root = nullptr;
				CleanNode(nodePtr);
				OGDF_ASSERT(nodePtr);
				delete nodePtr;
				break;

			case PQNodeRoot::PQNodeStatus::Full:
				emptyNode(nodePtr);
				break;

			case PQNodeRoot::PQNodeStatus::Partial:
				emptyNode(nodePtr);
				break;

			default:
				clientDefinedEmptyNode(nodePtr);
				break;
		}
	}
	m_pseudoRoot->m_pertChildCount = 0;
	m_pseudoRoot->m_pertLeafCount = 0;
	m_pseudoRoot->fullChildren->clear();
	m_pseudoRoot->partialChildren->clear();
	m_pseudoRoot->status(PQNodeRoot::PQNodeStatus::Empty);
	m_pseudoRoot->mark(PQNodeRoot::PQNodeMark::Unmarked);
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::emptyNode(PQNode<T,X,Y> *nodePtr)
{
	nodePtr->status(PQNodeRoot::PQNodeStatus::Empty);
	nodePtr->m_pertChildCount = 0;
	nodePtr->m_pertLeafCount = 0;
	nodePtr->fullChildren->clear();
	nodePtr->partialChildren->clear();
	nodePtr->mark(PQNodeRoot::PQNodeMark::Unmarked);
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::exchangeNodes(
	PQNode<T,X,Y> *oldNode,
	PQNode<T,X,Y> *newNode)
{
	if (oldNode->m_referenceParent != nullptr)
	{
		/*
		The node [[oldNode]] is connected to its parent
		via the reference pointer of the doubly linked list. The [[newNode]]
		will be the new reference child and is linked via the reference
		pointers to the $P$-node.
		*/
		oldNode->m_referenceParent->m_referenceChild = newNode;
		newNode->m_referenceParent = oldNode->m_referenceParent;
		oldNode->m_referenceParent = nullptr;
	}

	else if (oldNode->endmostChild())
	{
		/*
		The [[oldNode]] is endmost child of a Q-node. So its parent
		contains an extra pointer to [[oldNode]]. Link the parent of
		[[oldNode]] to [[newNode]] via this pointer and make [[newNode]]
		endmost child of its new parent.
		*/
		if (oldNode->m_parent->m_leftEndmost == oldNode)
			oldNode->m_parent->m_leftEndmost = newNode;
		else if (oldNode->m_parent->m_rightEndmost == oldNode)
			oldNode->m_parent->m_rightEndmost = newNode;
	}

	if (oldNode->m_sibLeft == oldNode && oldNode->m_sibRight == oldNode) {
		/*
		Two possible cases are occured.
		\begin{enumerate}
		\item [[oldNode]] is the only child of a $P$-node. In order to
		implement the doubly linked list of children of the $P$-node, the sibling
		pointers of [[newNode]] are set to [[newNode]].
		\item [[oldNode]] is the [[m_root]] of the $PQ$-tree. Since
		by our definition of the $PQ$-tree the sibling pointers of the
		[[m_root]] point to the root itself, (i.e. to make sure that
		checking for the endmost child property is also valid for the root)
		the sibling pointers of [[newNode]] are set to [[newNode]] as well.
		\end{enumerate}
		*/
		oldNode->m_sibLeft = nullptr;
		oldNode->m_sibRight = nullptr;
		if (oldNode->m_parent == nullptr)
		{
			newNode->m_sibLeft = newNode;
			newNode->m_sibRight = newNode;
		}
		else
		{
			newNode->m_sibLeft = newNode;
			newNode->m_sibRight = newNode;
		}
	}
	else
	{
		OGDF_ASSERT(!(oldNode->m_sibLeft == oldNode));
		//sibling pointers of old node are not compatible
		OGDF_ASSERT(!(oldNode->m_sibRight == oldNode));
		//sibling pointers of old node are not compatible.
	}
	/*
	Manage the exchange of [[oldNode]] and [[newNode]] according to
	[[oldNode]]'s siblings. The chunk checks both siblings of
	[[oldNode]] and resets the sibling pointers of [[oldNode]]'s siblings
	as well as the sibling pointers of [[newNode]].
	*/
	if (oldNode->m_sibLeft != nullptr)
	{
		if (oldNode->m_sibLeft->m_sibRight == oldNode)
			oldNode->m_sibLeft->m_sibRight = newNode;
		else {
			// Sibling was not connected to child?
			OGDF_ASSERT(oldNode->m_sibLeft->m_sibLeft == oldNode);
			oldNode->m_sibLeft->m_sibLeft = newNode;
		}
		newNode->m_sibLeft = oldNode->m_sibLeft;
		oldNode->m_sibLeft = nullptr;
	}

	if (oldNode->m_sibRight != nullptr)
	{
		if (oldNode->m_sibRight->m_sibLeft == oldNode)
			oldNode->m_sibRight->m_sibLeft = newNode;
		else {
			// Sibling was not connected to child?
			OGDF_ASSERT(oldNode->m_sibRight->m_sibRight == oldNode);
			oldNode->m_sibRight->m_sibRight = newNode;
		}
		newNode->m_sibRight = oldNode->m_sibRight;
		oldNode->m_sibRight = nullptr;
	}

	newNode->m_parentType = oldNode->m_parentType;
	newNode->m_parent = oldNode->m_parent;
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::front(
	PQNode<T,X,Y>* nodePtr,
	SListPure<PQLeafKey<T,X,Y>*> &leafKeys)
{
	Queue<PQNode<T,X,Y>*> helpqueue;
	helpqueue.append(nodePtr);

	while (!helpqueue.empty())
	{
		PQNode<T,X,Y>* checkNode = helpqueue.pop();

		if (checkNode->type() == PQNodeRoot::PQNodeType::Leaf)
			leafKeys.pushBack(checkNode->getKey());
		else
		{
			PQNode<T,X,Y>*  firstSon  = nullptr;
			PQNode<T,X,Y>*  nextSon   = nullptr;
			PQNode<T,X,Y>*  oldSib    = nullptr;
			PQNode<T,X,Y>*  holdSib   = nullptr;

			if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
			{
				OGDF_ASSERT(checkNode->m_referenceChild);
				firstSon = checkNode->m_referenceChild;
			}
			else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
			{
				OGDF_ASSERT(checkNode->m_leftEndmost);
				firstSon = checkNode->m_leftEndmost;
			}
			helpqueue.append(firstSon);
			nextSon = firstSon->getNextSib(oldSib);
			oldSib = firstSon;
			while (nextSon && nextSon != firstSon)
			{
				helpqueue.append(nextSon);
				holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
			}
		}
	}
}

template<class T,class X,class Y>
int PQTree<T,X,Y>::Initialize(SListPure<PQLeafKey<T,X,Y>*> &leafKeys)
{
	m_pertinentNodes = new List<PQNode<T,X,Y>*>;

	if (!leafKeys.empty())
	{
		PQInternalNode<T,X,Y> *newNode2 = new PQInternalNode<T,X,Y>(-1,PQNodeRoot::PQNodeType::QNode,PQNodeRoot::PQNodeStatus::Partial);
		m_pseudoRoot = newNode2;

		if (leafKeys.begin() != leafKeys.end()) // at least two elements
		{
			PQInternalNode<T,X,Y> *newNode = new PQInternalNode<T,X,Y>(m_identificationNumber++,PQNodeRoot::PQNodeType::PNode,PQNodeRoot::PQNodeStatus::Empty);
			m_root = newNode;
			m_root->m_sibLeft = m_root;
			m_root->m_sibRight = m_root;
			return addNewLeavesToTree(newNode,leafKeys);
		}
		PQLeaf<T,X,Y> *newLeaf = new PQLeaf<T,X,Y>(m_identificationNumber++,PQNodeRoot::PQNodeStatus::Empty,*leafKeys.begin());
		m_root = newLeaf;
		m_root->m_sibLeft = m_root;
		m_root->m_sibRight = m_root;
		return 1;
	}

	return 0;
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::linkChildrenOfQnode(
	PQNode<T,X,Y> *installed,
	PQNode<T,X,Y> *newChild)
{
	if (installed != nullptr && newChild != nullptr)
	{
		if (installed->m_sibLeft == nullptr)
		{
			installed->m_sibLeft = newChild;
			if (newChild->m_sibRight == nullptr)
				newChild->m_sibRight = installed;
			else
				newChild->m_sibLeft = installed;
		}
		else {
			// endmost child with 2 siblings encountered?
			OGDF_ASSERT(installed->m_sibRight == nullptr);

			installed->m_sibRight = newChild;
			if (newChild->m_sibLeft == nullptr)
				newChild->m_sibLeft = installed;
			else
				newChild->m_sibRight = installed;
		}
	}
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::writeGML(const char *fileName)
{
	std::ofstream os(fileName);
	writeGML(os);
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::writeGML(std::ostream &os)
{
	Array<int> id(0,m_identificationNumber,0);
	int nextId = 0;

	SListPure<PQNode<T,X,Y>*> helpQueue;
	SListPure<PQNode<T,X,Y>*> secondTrace;

	os.setf(std::ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::PQTree::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	PQNode<T,X,Y>*  checkNode = m_root;
	PQNode<T,X,Y>*  firstSon  = 0;
	PQNode<T,X,Y>*  nextSon   = 0;
	PQNode<T,X,Y>*  lastSon   = 0;
	PQNode<T,X,Y>*  oldSib    = 0;
	PQNode<T,X,Y>*  holdSib   = 0;

	if (checkNode->type() != PQNodeRoot::PQNodeType::Leaf)
		secondTrace.pushBack(checkNode);

	while (checkNode)
	{
		os << "  node [\n";

		os << "    id " << (id[checkNode->m_identificationNumber] = nextId++) << "\n";

		os << "    label \"" << checkNode->m_identificationNumber;
		if (checkNode->getKey() != 0)
			checkNode->getKey()->print(os);
		os << "\"\n";

		os << "    graphics [\n";
		if (m_root->status() == PQNodeRoot::PQNodeStatus::Full)
		{
			if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
				os << "      fill \"#FF0000\"\n";
			else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
				os << "      fill \"#0000A0\"\n";
			else if (checkNode->type() == PQNodeRoot::PQNodeType::Leaf)
				os << "fill \"#FFFFE6\"\n";
		}
		else if (m_root->status() == PQNodeRoot::PQNodeStatus::Empty)
		{
			if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
				os << "      fill \"#FF0000\"\n";
			else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
				os << "      fill \"#0000A0\"\n";
			else if (checkNode->type() == PQNodeRoot::PQNodeType::Leaf)
				os << "      fill \"#00FF00\"\n";
		}
		else if (m_root->status() == PQNodeRoot::PQNodeStatus::Partial)
		{
			if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
				os << "      fill \"#FF0000\"\n";
			else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
				os << "      fill \"#0000A0\"\n";
			else if (checkNode->type() == PQNodeRoot::PQNodeType::Leaf)
				os << "      fill \"#FFFFE6\"\n";
		}
		else if (m_root->status() == PQNodeRoot::PQNodeStatus::Pertinent)
		{
			if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
				os << "      fill \"#FF0000\"\n";
			else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
				os << "      fill \"#0000A0\"\n";
			else if (checkNode->type() == PQNodeRoot::PQNodeType::Leaf)
				os << "      fill \"#FFFFE6\"\n";
		}

		os << "    ]\n"; // graphics
		os << "  ]\n"; // node

		if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
		{
			if (checkNode->m_referenceChild != 0)
			{
				firstSon = checkNode->m_referenceChild;
				helpQueue.pushBack(firstSon);

				if (firstSon->m_sibRight != 0)
				nextSon = firstSon->m_sibRight;
				while (nextSon != firstSon)
				{
					helpQueue.pushBack(nextSon);
					nextSon = nextSon->m_sibRight;
				}
			}
		}
		else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
		{
			oldSib = 0;
			holdSib = 0;

			firstSon = checkNode->m_leftEndmost;
			helpQueue.pushBack(firstSon);

			lastSon = checkNode->m_rightEndmost;
			if ( firstSon != lastSon)
			{
				helpQueue.pushBack(lastSon);
				nextSon = lastSon->getNextSib(oldSib);
				oldSib = lastSon;
				while (nextSon != firstSon)
				{
					helpQueue.pushBack(nextSon);
					holdSib = nextSon->getNextSib(oldSib);
					oldSib = nextSon;
					nextSon = holdSib;
				}
			}
		}
		if (!helpQueue.empty())
		{
			checkNode = helpQueue.popFrontRet();
			if (checkNode->type() != PQNodeRoot::PQNodeType::Leaf)
				secondTrace.pushBack(checkNode);
		}
		else
			checkNode = 0;
	}


	SListIterator<PQNode<T,X,Y>*> it;

	for (it = secondTrace.begin(); it.valid(); ++it)
	{
		checkNode = *it;
		if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
		{
			if (checkNode->m_referenceChild != 0)
			{
				firstSon = checkNode->m_referenceChild;
				os << "  edge [\n";
				os << "    source " << id[checkNode->m_identificationNumber] << "\n";
				os << "    target " << id[firstSon->m_identificationNumber] << "\n";
				os << "  ]\n"; // edge

				if (firstSon->m_sibRight != 0)
				nextSon = firstSon->m_sibRight;
				while (nextSon != firstSon)
				{
					os << "  edge [\n";
					os << "    source " << id[checkNode->m_identificationNumber] << "\n";
					os << "    target " << id[nextSon->m_identificationNumber] << "\n";
					os << "  ]\n"; // edge
					nextSon = nextSon->m_sibRight;
				}
			}
		}
		else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
		{
			oldSib = 0;
			holdSib = 0;

			firstSon = checkNode->m_leftEndmost;
			lastSon = checkNode->m_rightEndmost;

			os << "  edge [\n";
			os << "    source " << id[checkNode->m_identificationNumber] << "\n";
			os << "    target " << id[lastSon->m_identificationNumber] << "\n";
			os << "  ]\n"; // edge
			if ( firstSon != lastSon)
			{
				nextSon = lastSon->getNextSib(oldSib);
				os << "  edge [\n";
				os << "    source " << id[checkNode->m_identificationNumber] << "\n";
				os << "    target " << id[nextSon->m_identificationNumber] << "\n";
				os << "  ]\n"; // edge

				oldSib = lastSon;
				while (nextSon != firstSon)
				{
					holdSib = nextSon->getNextSib(oldSib);
					oldSib = nextSon;
					nextSon = holdSib;
					os << "  edge [\n";
					os << "    source " << id[checkNode->m_identificationNumber] << "\n";
					os << "    target " << id[nextSon->m_identificationNumber] << "\n";
					os << "  ]\n"; // edge
				}
			}
		}
	}
	os << "]\n"; // graph
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::Reduce(SListPure<PQLeafKey<T,X,Y>*> &leafKeys)
{
	/* Variables:
	 *   - \a checkLeaf is a pointer to a various PQLeaf of the set of
	 *     elements that has to be reduced.
	 *   - \a checkNode is a pointer to a various node of the pertinent
	 *     subtree.
	 *   - \a pertLeafCount counts the number of pertinent leaves in the
	 *     PQ-tree. Since Reduce() takes care that every node knows the
	 *     number of pertinent leaves in its frontier, the root of the
	 *     pertinent subtree can be identified with the help of \a pertLeafCount.
	 *   - \a processNodes is a queue storing nodes of the pertinent
	 *     subtree that are considered to be reduced next. A node may be
	 *     reduced (and therefore is pushed on to \a processNodes) as soon as
	 *     all its pertinent children have been reduced.
	 */
	int                   pertLeafCount = 0;
	Queue<PQNode<T,X,Y>*> processNodes;

	/*
	 * In a first step the pertinent leaves have to be identified in the tree
	 * and are entered on to the queue [[processNodes]]. The identification of
	 * the leaves can be done with the help of a pointer stored in every
	 * [[PQLeafKey]] in constant time for every element.
	 */
	SListIterator<PQLeafKey<T,X,Y>*> it;
	for (it = leafKeys.begin(); it.valid(); ++it)
	{
		PQNode<T,X,Y>* checkLeaf = (*it)->nodePointer();
		checkLeaf->status(PQNodeRoot::PQNodeStatus::Full);
		checkLeaf->m_pertLeafCount = 1;
		processNodes.append(checkLeaf);
		pertLeafCount++;
	}

	PQNode<T,X,Y>* checkNode = processNodes.top();
	while (checkNode != nullptr && processNodes.size() > 0)
	{
		checkNode = processNodes.pop();

		if (checkNode->m_pertLeafCount < pertLeafCount)
		{
			/*
			 * Application of the template matchings to a pointer [[checkNode]]
			 * storing the adress of a node that is <b>not the root</b> of the
			 * pertinent subtree. Before applying the template matchings to
			 * [[checkNode]], some values of the parent of [[checkNode]] are
			 * updated. The number of the parents pertinent children stored in
			 * [[pertChildCount]] is count down by one. In case that
			 * [[checkNode->m_parent->m_pertChildCount == 0]], we know that all
			 * pertinent children of the parent have been processed. Since the
			 * parent then is allowed to be processed as well,
			 * [[checkNode->m_parent]] is stored in the queue [[processNodes]].
			 */
			checkNode->m_parent->m_pertLeafCount =
				checkNode->m_parent->m_pertLeafCount
				+ checkNode->m_pertLeafCount;

			checkNode->m_parent->m_pertChildCount--;
			if (!checkNode->m_parent->m_pertChildCount)
				processNodes.append(checkNode->m_parent);
			if (!templateL1(checkNode, 0)
			 && !templateP1(checkNode, 0)
			 && !templateP3(checkNode)
			 && !templateP5(checkNode)
			 && !templateQ1(checkNode, 0)
			 && !templateQ2(checkNode, 0)) {
				checkNode= nullptr;
			}
		}
		else
		{
			/*
			 * application of the template matchings to a pointer [[checkNode]]
			 * that stores the adress of a node that <b>is the root</b> of the
			 * pertinent subtree. In a case that a template matching was
			 * successfully applied, the pointer [[checkNode]] stores after the
			 * application the adress of the root of pertinent subtree. This
			 * includes nodes that have been newly introduced as root of the
			 * pertinent subtree during the application. If no template matching
			 * was successfully applied [[checkNode]] is a [[0]] pointer.
			 */
			if (!templateL1(checkNode, 1)
			 && !templateP1(checkNode, 1)
			 && !templateP2(&checkNode)
			 && !templateP4(&checkNode)
			 && !templateP6(&checkNode)
			 && !templateQ1(checkNode, 1)
			 && !templateQ2(checkNode, 1)
			 && !templateQ3(checkNode)) {
				checkNode = nullptr;
			}
		}
	}

	m_pertinentRoot = checkNode;
	return m_pertinentRoot != nullptr;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::Reduction(SListPure<PQLeafKey<T,X,Y>*> &leafKeys)
{
	/* Reduction() calls the procedure Bubble() and if Bubble() was
	 * successful, Reduction() calls the function Reduce().
	 */
	bool success = Bubble(leafKeys);

	if (!success)
		return false;
	else
		return Reduce(leafKeys);
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::removeBlock(PQNode<T,X,Y> *nodePtr,bool isRoot)
{
	/*
	For every
	partial child we keep a set of pointers. Since there are at most
	two partial children, we initialize two sets. Every set contains
	besides a pointer to the partial child four pointers to its endmost
	children, sorted according to their full or empty labels and pointers
	to the immediate siblings of the partial child, also sorted according
	to their full or empty labels.
	*/
	///Pointer to the first partial child
	PQNode<T,X,Y> *partial_1 = nullptr;

	/*
	Pointer to the full endmost child (more
	precisely: to the endmost child appearing on the full side) of
	[[partial_1]]. In case that ignored nodes are used, this [[endfull_1]]
	may store the adress of an ignored node.
	*/
	PQNode<T,X,Y> *endfull_1 = nullptr;

	/*
	Pointer to the empty endmost child  (more
	precisely: to the endmost child appearing on the empty side) of
	[[partial_1]]. In case that ignored nodes are used, this [[endempty_1]]
	may store the adress of an ignored node.
	*/
	PQNode<T,X,Y> *endempty_1 = nullptr;

	/*
	Pointer to the first <i>non ignored</i> node
	with full status. [[realfull_1]] is identical to [[endfull_1]] if no
	ignored nodes appear at the full end of the first partial child.
	*/
	PQNode<T,X,Y> *realfull_1 = nullptr;

	/*
	Pointer to the first <i>non ignored</i> node
	with empty status. [[realempty_1]] is identical to [[endempty_1]] if no
	ignored nodes appear at the empty end of the first partial child.
	*/
	PQNode<T,X,Y> *realempty_1 = nullptr;

	// Pointer to the second partial child.
	PQNode<T,X,Y> *partial_2 = nullptr;

	/*
	Pointer to the full endmost child (more
	precisely: to the endmost child appearing on the full side) of
	[[partial_2]]. In case that ignored nodes are used, this [[endfull_2]]
	may store the adress of an ignored node.
	*/
	PQNode<T,X,Y> *endfull_2 = nullptr;

	/*
	Pointer to the empty endmost child  (more
	precisely: to the endmost child appearing on the empty side) of
	[[partial_2]]. In case that ignored nodes are used, this [[endempty_2]]
	may store the adress of an ignored node.
	*/
	PQNode<T,X,Y> *endempty_2 = nullptr;

	/*
	Pointer to the first <i>non ignored</i> node
	with empty status. [[realempty_2]] is identical to [[endempty_2]] if no
	ignored nodes appear at the empty end of the first partial child.
	*/
	PQNode<T,X,Y> *realempty_2 = nullptr;

	/*
	Pointer to a full sibling of
	[[partial_1]], if it exists. In case that ignored nodes are used
	this [[sibfull_1]] stores the direct sibling of [[partial_1]]
	on the side where the full siblings are. Hence [[sibfull_1]] may
	store an ignored node.
	*/
	PQNode<T,X,Y> *sibfull_1 = nullptr;

	/*
	Pointer to a partial sibling of
	[[partial_1]], if it exists. In case that ignored nodes are used
	this [[sibpartial_1]] stores the direct sibling of [[partial_1]]
	on the side where a partial sibling is. Hence [[sibpartial_1]] may
	store an ignored node.
	*/
	PQNode<T,X,Y> *sibpartial_1 = nullptr;

	/*
	Pointer to an empty sibling of
	[[parial_1]], if it exists. In case that ignored nodes are used
	this [[sibempty_1]] stores the direct sibling of [[partial_1]]
	on the side where the empty siblings are. Hence [[sibempty_1]] may
	store an ignored node.
	*/
	PQNode<T,X,Y> *sibempty_1 = nullptr;

	/*
	Pointer only used in case that [[partial_1]] has
	no non-ignored siblings on one side. [[partial_1]] then is endmost child
	of [[nodePtr]], but ignored children may appear between [[partial_1]]
	and the end of sequence of children of [[nodePtr]]. The
	[[nonstatussib_1]] then stores the adress of the endmost ignored child.
	Observe that it is not valid for a $Q$-node to have only one
	non-ignored child and several ignored children. Hence this situation
	is only allowed to appear <b>once</b> on one side of [[partial_1]].
	Every other situation results in an error message.
	*/
	PQNode<T,X,Y> *nonstatussib_1 = nullptr;

	/*
	Pointer to a full sibling of
	[[partial_2]], if it exists. In case that ignored nodes are used
	this [[sibfull_2]] stores the direct sibling of [[partial_2]]
	on the side where the full siblings are. Hence [[sibfull_2]] may
	store an ignored node.
	*/
	PQNode<T,X,Y> *sibfull_2 = nullptr;

	/*
	Pointer to a partial sibling of
	[[partial_2]], if it exists. In case that ignored nodes are used
	this [[sibpartial_2]] stores the direct sibling of [[partial_2]]
	on the side where a partial sibling is. Hence [[sibpartial_2]] may
	store an ignored node.
	*/
	PQNode<T,X,Y> *sibpartial_2 = nullptr;

	/*
	Pointer to an empty sibling of
	[[parial_2]], if it exists. In case that ignored nodes are used
	this [[sibempty_2]] stores the direct sibling of [[partial_2]]
	on the side where the empty siblings are. Hence [[sibempty_2]] may
	store an ignored node.
	*/
	PQNode<T,X,Y> *sibempty_2 = nullptr;

	/*
	Pointer	only used in case that [[partial_2]] has
	no non-ignored siblings on one side. [[partial_2]] then is endmost child
	of [[nodePtr]], but ignored children may appear between [[partial_2]]
	and the end of sequence of children of [[nodePtr]]. The
	[[nonstatussib_2]] then stores the adress of the endmost ignored child.
	Observe that it is not valid for a $Q$-node to have only one
	non-ignored child and several ignored children. Hence this situation
	is only allowed to appear <b>once</b> on one side of [[partial_2]].
	Every other situation results in an error message.
	*/
	PQNode<T,X,Y> *nonstatussib_2 = nullptr;

	PQNode<T,X,Y> *helpptr        = nullptr;

	PQNode<T,X,Y> *checkVarLeft   = nullptr;

	PQNode<T,X,Y> *checkVarRight  = nullptr;

	/*
	[[endmostcheck]] is [[1]], if [[partial_1]] is the endmost
	child of [[nodePtr]]. Per default, [[endmostcheck]] is [[0]].
	*/
	int endmostcheck = 0;


	nodePtr->status(PQNodeRoot::PQNodeStatus::Partial);
	if (!isRoot)
		nodePtr->m_parent->partialChildren->pushFront(nodePtr);

	if (!nodePtr->partialChildren->empty()) { // Get a partial child.
		partial_1 = nodePtr->partialChildren->popFrontRet();

		// Get the full and empty
		// endmost children of the
		// partial child [[partial_1]].
		checkVarLeft = clientLeftEndmost(partial_1);
		checkVarRight = clientRightEndmost(partial_1);
		if (checkVarLeft->status() == PQNodeRoot::PQNodeStatus::Full)
		{
			endfull_1  = partial_1->m_leftEndmost;
			realfull_1 = checkVarLeft;
		}
		else {
			// partial child with no full endmost child detected?
			OGDF_ASSERT(checkVarRight->status() == PQNodeRoot::PQNodeStatus::Full);

			endfull_1  = partial_1->m_rightEndmost;
			realfull_1 = checkVarRight;
		}

		if (checkVarLeft->status() == PQNodeRoot::PQNodeStatus::Empty)
		{
			endempty_1  = partial_1->m_leftEndmost;
			realempty_1 = checkVarLeft;
		}
		else {
			// partial child with no empty endmost child detected?
			OGDF_ASSERT(checkVarRight->status() == PQNodeRoot::PQNodeStatus::Empty);

			endempty_1  = partial_1->m_rightEndmost;
			realempty_1 = checkVarRight;
		}

		// Get the immediate
		// siblings of the partial
		// child [[partial_1]].
		if (clientSibLeft(partial_1) != nullptr)
		{
			if (clientSibLeft(partial_1)->status() == PQNodeRoot::PQNodeStatus::Full)
				sibfull_1 = partial_1->m_sibLeft;
			else if (clientSibLeft(partial_1)->status() == PQNodeRoot::PQNodeStatus::Empty)
				sibempty_1 = partial_1->m_sibLeft;
			else if (clientSibLeft(partial_1)->status() == PQNodeRoot::PQNodeStatus::Partial)
				sibpartial_1 = partial_1->m_sibLeft;
		}
		else
			nonstatussib_1 = partial_1->m_sibLeft;

		if (clientSibRight(partial_1) != nullptr)
		{
			if (clientSibRight(partial_1)->status() == PQNodeRoot::PQNodeStatus::Full)
				sibfull_1 = partial_1->m_sibRight;
			else if (clientSibRight(partial_1)->status() == PQNodeRoot::PQNodeStatus::Empty)
				sibempty_1 = partial_1->m_sibRight;
			else if (clientSibRight(partial_1)->status() == PQNodeRoot::PQNodeStatus::Partial)
				sibpartial_1 = partial_1->m_sibRight;
		}
		else {
			// partial child detected with no siblings of valid status?
			OGDF_ASSERT(nonstatussib_1 == nullptr);
			nonstatussib_1 = partial_1->m_sibRight;
		}
	}


	if (!nodePtr->partialChildren->empty()) { // There is a second partial child.
		partial_2 = nodePtr->partialChildren->popFrontRet();
		// Get the full and empty endmost
		// children of the partial
		// child [[partial_2]].

		checkVarLeft = clientLeftEndmost(partial_2);
		checkVarRight = clientRightEndmost(partial_2);
		if (checkVarLeft->status() == PQNodeRoot::PQNodeStatus::Full)
		{
			endfull_2 = partial_2->m_leftEndmost;
		}
		else {
			// partial child with no full endmost child detected?
			OGDF_ASSERT(checkVarRight->status() == PQNodeRoot::PQNodeStatus::Full);

			endfull_2 = partial_2->m_rightEndmost;
		}

		if (checkVarLeft->status() == PQNodeRoot::PQNodeStatus::Empty)
		{
			endempty_2 = partial_2->m_leftEndmost;
			realempty_2 = checkVarLeft;
		}
		else {
			// partial child with no empty endmost child detected?
			OGDF_ASSERT(checkVarRight->status() == PQNodeRoot::PQNodeStatus::Empty);

			endempty_2 = partial_2->m_rightEndmost;
			realempty_2 = checkVarRight;
		}
		// Get the immediate siblings
		// of the partial child
		// [[partial_2]].
		if (clientSibLeft(partial_2) != nullptr)
		{
			if (clientSibLeft(partial_2)->status() == PQNodeRoot::PQNodeStatus::Full)
				sibfull_2 = partial_2->m_sibLeft;
			else if (clientSibLeft(partial_2)->status() == PQNodeRoot::PQNodeStatus::Empty)
				sibempty_2 = partial_2->m_sibLeft;
			else if (clientSibLeft(partial_2)->status() == PQNodeRoot::PQNodeStatus::Partial)
				sibpartial_2 = partial_2->m_sibLeft;
		}
		else
			nonstatussib_2 = partial_2->m_sibLeft;


		if (clientSibRight(partial_2) != nullptr)
		{
			if (clientSibRight(partial_2)->status() == PQNodeRoot::PQNodeStatus::Full)
				sibfull_2 = partial_2->m_sibRight;
			else if (clientSibRight(partial_2)->status() == PQNodeRoot::PQNodeStatus::Empty)
				sibempty_2 = partial_2->m_sibRight;
			else if (clientSibRight(partial_2)->status() == PQNodeRoot::PQNodeStatus::Partial)
				sibpartial_2 = partial_2->m_sibRight;
		}
		else {
			OGDF_ASSERT(nonstatussib_2 == nullptr);
			nonstatussib_2 = partial_2->m_sibRight;
		}
	}


	if (partial_1 != nullptr && partial_2 != nullptr)

	/*
	Connect the endmost
	children of the partial children [[partial_1]] and [[partial_2]] correctly
	with their new siblings. In doing this, all children of the partial
	children become children of [[nodePtr]]. The reader should observe that
	the parent pointers of the interior children of [[partial_1]] and
	[[partial_2]] are not updated in order to hit the linear time complexity.

	When including the children of the partial children to the children
	of [[nodePtr]], it is taken care that all full children
	form a consecutive sequence afterwards. If neccessary the pointers to the
	endmost children of [[nodePtr]] are updated.
	*/
	{
		if (sibfull_1 != nullptr && sibfull_2 != nullptr)
			// There are full children  between
			// the 2 partial nodes.
			// Connect the full children
			// between the 2 partial children
			// with the full endmost children
			// of the 2 partial nodes.
		{
			sibfull_1->changeSiblings(partial_1,endfull_1);
			endfull_1->putSibling(sibfull_1);
			sibfull_2->changeSiblings(partial_2,endfull_2);
			endfull_2->putSibling(sibfull_2);
		}

		else if (sibpartial_1 != nullptr && sibpartial_2 != nullptr)
			// There are no full children between
			// the 2 partial nodes. Connect the
			// full endmost children of the
			// partial nodes as siblings.
		{
			if (partial_1 == sibpartial_2 && partial_2 == sibpartial_1)
				// Regular Case.
			{
				endfull_1->putSibling(endfull_2);
				endfull_2->putSibling(endfull_1);
			}
				// Only ignored children between
				// partial_1 and partial_2.
			else
			{
				endfull_1->putSibling(sibpartial_1);
				sibpartial_1->changeSiblings(partial_1,endfull_1);
				endfull_2->putSibling(sibpartial_2);
				sibpartial_2->changeSiblings(partial_2,endfull_2);
			}

		}
			// Include the children of the
			// partial children with their
			// full nodes inbetween into
			// the sequence of the children of
			// Q-node nodePtr.
		if (sibempty_1 == nullptr)
			// partial_1 is endmost child of
			// nodePtr. Make the empty endmost
			// child of partial_1 be the new
			// endmost child of nodePtr.
		{
			if (nonstatussib_1 == nullptr)
				// Regular case.
			{
				nodePtr->changeEndmost(partial_1,endempty_1);
			}
			else
				// Only ignored children between
				// partial_1 and one end of nodePtr.
			{
				nonstatussib_1->changeSiblings(partial_1,endempty_1);
				endempty_1->putSibling(nonstatussib_1);
			}
			endempty_1->m_parent = nodePtr;
			realempty_1->m_parent = nodePtr;
		}

		else
			// partial_1 is not endmost child.
		{
			sibempty_1->changeSiblings(partial_1,endempty_1);
			endempty_1->putSibling(sibempty_1);
		}


		if (sibempty_2 == nullptr)
			// partial_2 is endmost child of
			// nodePtr. Make the empty endmost
			// child of partial_2 be the new
			// endmost child of nodePtr.
		{
			if (nonstatussib_2 == nullptr)
				// Regular case.
			{
				nodePtr->changeEndmost(partial_2,endempty_2);
			}
			else
				// Only ignored children between
				// partial_1 and one end of
				// nodePtr.
			{
				nonstatussib_2->changeSiblings(partial_2,endempty_2);
				endempty_2->putSibling(nonstatussib_2);
			}
			endempty_2->m_parent = nodePtr;
			realempty_2->m_parent = nodePtr;
		}

		else
			// partial_2 is not endmost child.
		{
			sibempty_2->changeSiblings(partial_2,endempty_2);
			endempty_2->putSibling(sibempty_2);
		}

			// Copy the full children of
			// partial_1 and partial_2 to
			// nodePtr.
		while (!partial_2->fullChildren->empty())
		{
			helpptr = partial_2->fullChildren->popFrontRet();
			nodePtr->fullChildren->pushFront(helpptr);
		}
		nodePtr->m_childCount = nodePtr->m_childCount +partial_2->m_childCount - 1;

		destroyNode(partial_2);

		while (!partial_1->fullChildren->empty())
		{
			helpptr = partial_1->fullChildren->popFrontRet();
			nodePtr->fullChildren->pushFront(helpptr);
		}
		nodePtr->m_childCount = nodePtr->m_childCount +partial_1->m_childCount - 1;

		destroyNode(partial_1);
	}


	else if (partial_1 != nullptr)

	/*
	Connect the endmost
	children of the partial child [[partial_1]] correctly
	with their new siblings. In doing this, all children of the partial
	child become children of [[nodePtr]]. The reader should observe that
	the parent pointers of the interior children of [[partial_1]]
	are not updated in order to hit the linear time complexity.

	When including the children of [[partial_1]] to the children
	of [[nodePtr]], it is taken care that all full children
	form a consecutive sequence afterwards. If necessary the pointers to the
	endmost children of [[nodePtr]] are updated.
	*/
	{
		if ((clientLeftEndmost(nodePtr) == partial_1) ||
			(clientRightEndmost(nodePtr) == partial_1))
				// partial_1 is endmost child.
			endmostcheck = 1;

		if (sibfull_1 != nullptr)
			// There are full children on one
			// side of the partial node.
			// Connect the full children with
			// the full endmost child of
			// partial_1.
		{
			sibfull_1->changeSiblings(partial_1,endfull_1);
			endfull_1->putSibling(sibfull_1);
		}

		else if (!endmostcheck)
			// There are not any full children
			// and partial_1 is not endmost.
			// So get the 2nd empty sibling
			// of partial_1 and connect it
			// to the full endmost child
			// of partial_1.
		{
			if (partial_1->m_sibLeft != sibempty_1)
				sibempty_2 = partial_1->m_sibLeft;
			else
				sibempty_2 = partial_1->m_sibRight;

			sibempty_2->changeSiblings(partial_1,endfull_1);
			endfull_1->putSibling(sibempty_2);
		}

		else
			// partial_1 is endmost child
			// and there are no full children.
			// Make the full endmost child of
			// partial_1 be the endmostchild
			// of nodePtr.
		{

			if (nonstatussib_1 == nullptr)
				// Regular case.
			{
				nodePtr->changeEndmost(partial_1,endfull_1);
			}
			else
				// Only ignored children between
				// partial_1 and one end of
				// nodePtr.
			{
				nonstatussib_1->changeSiblings(partial_1,endfull_1);
				endfull_1->putSibling(nonstatussib_1);
			}
			endfull_1->m_parent = nodePtr;
			realfull_1->m_parent = nodePtr;

		}

		if (sibempty_1 == nullptr)
			// There are no empty children.
			// partial_1 is endmost child of
			// nodePtr. Make the empty endmost
			// child of partial_1 be the new
			// endmost child of nodePtr.
		{
			if (nonstatussib_1 == nullptr)
				// Regular case.
			{
				nodePtr->changeEndmost(partial_1,endempty_1);
			}
			else
				// Only ignored children between
				// partial_1 and one end of
				// nodePtr.
			{
				nonstatussib_1->changeSiblings(partial_1,endempty_1);
				endempty_1->putSibling(nonstatussib_1);
			}
			endempty_1->m_parent = nodePtr;
			realempty_1->m_parent = nodePtr;
		}

		else
			// There are empty children. So
			// connect the empty endmost child
			// of partial_1 with sibempty_1.
		{
			sibempty_1->changeSiblings(partial_1,endempty_1);
			endempty_1->putSibling(sibempty_1);
		}

		while (!partial_1->fullChildren->empty())
		{
			helpptr = partial_1->fullChildren->popFrontRet();
			nodePtr->fullChildren->pushFront(helpptr);
		}

		nodePtr->m_childCount = nodePtr->m_childCount + partial_1->m_childCount - 1;
		destroyNode(partial_1);

	}
	// else nodePtr does not have partial children. Then nothing is to do.
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::removeChildFromSiblings(PQNode<T,X,Y>* nodePtr)
{
	if (nodePtr->m_referenceParent != nullptr)
	{
		/*
		Checksif [[nodePtr]] is connected with its parent via the reference
		pointers (see \ref{PQNode}). If so, the next sibling of [[nodePtr]]
		will be the the new reference child.
		*/
		nodePtr->m_referenceParent->m_referenceChild = nodePtr->m_sibRight;
		nodePtr->m_sibRight->m_referenceParent = nodePtr->m_referenceParent;
		if (nodePtr->m_referenceParent->m_referenceChild == nodePtr)
			nodePtr->m_referenceParent->m_referenceChild = nullptr;
		nodePtr->m_referenceParent = nullptr;
	}
	else if (nodePtr->endmostChild())
	{
		/*
		Check if [[nodePtr]] is the endmost child of a $Q$-node.
		If so, the next sibling of [[nodePtr]] will be the the new endmost
		child of the $Q$-node. Observe that the sibling then gets a valid
		pointer to its parent.
		*/
		PQNode<T,X,Y> *sibling = nodePtr->getNextSib(nullptr);
		if (nodePtr->m_parent->m_leftEndmost == nodePtr)
			nodePtr->m_parent->m_leftEndmost = sibling;
		else if (nodePtr->m_parent->m_rightEndmost == nodePtr)
			nodePtr->m_parent->m_rightEndmost = sibling;
		if (sibling != nullptr)
			sibling->m_parent = nodePtr->m_parent;
	}

	/*
	Remove [[nodePtr]] from its immediate siblings and links the
	siblings via the [[sibRight]] and [[sibLeft]] pointers.
	*/
	if (nodePtr->m_sibRight != nullptr && nodePtr->m_sibRight != nodePtr)
	{
		if (nodePtr->m_sibRight->m_sibLeft == nodePtr)
			nodePtr->m_sibRight->m_sibLeft = nodePtr->m_sibLeft;
		else {
			// Sibling was not connected to child?
			OGDF_ASSERT(nodePtr->m_sibRight->m_sibRight == nodePtr);
			nodePtr->m_sibRight->m_sibRight = nodePtr->m_sibLeft;
		}
	}
	if (nodePtr->m_sibLeft != nullptr && nodePtr->m_sibLeft != nodePtr)
	{
		if (nodePtr->m_sibLeft->m_sibRight == nodePtr)
			nodePtr->m_sibLeft->m_sibRight = nodePtr->m_sibRight;
		else {
			// Sibling was not connected to child?
			OGDF_ASSERT(nodePtr->m_sibLeft->m_sibLeft == nodePtr);
			nodePtr->m_sibLeft->m_sibLeft = nodePtr->m_sibRight;
		}
	}

	nodePtr->m_sibRight = nullptr;
	nodePtr->m_sibLeft = nullptr;
}

template<class T,class X,class Y>
int PQTree<T,X,Y>::removeNodeFromTree(PQNode<T,X,Y>* parent,PQNode<T,X,Y>* child)
{
	if (parent !=nullptr)
	{
		removeChildFromSiblings(child);
		parent->m_childCount--;
		if (child->status() == PQNodeRoot::PQNodeStatus::Full || child->status() == PQNodeRoot::PQNodeStatus::Partial)
			parent->m_pertChildCount--;
		return parent->m_childCount;
	}
	else
	{
		//parent is invalid 0-pointer.
		return -1;
	}
}

template<class T,class X,class Y>
void PQTree<T,X,Y>::sortExceptions(int Exceptions[],int arraySize)
{
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (int i = 0; i < (arraySize-1); i++)
		{
			if (Exceptions[i] > Exceptions[i+1])
			{
				std::swap(Exceptions[i], Exceptions[i+1]);
				changed = true;
			}
		}
	}
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateL1(PQNode<T,X,Y> *nodePtr, bool isRoot)
{
	if (nodePtr->type() == PQNodeRoot::PQNodeType::Leaf && nodePtr->status() == PQNodeRoot::PQNodeStatus::Full)
	{
		if (!isRoot)
			nodePtr->m_parent->fullChildren->pushFront(nodePtr);
		return true;
	}

	return false;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateP1(PQNode<T,X,Y> *nodePtr, bool isRoot)
{
	if (nodePtr->type() != PQNodeRoot::PQNodeType::PNode ||
		nodePtr->fullChildren->size() != nodePtr->m_childCount)
		return false;
	else
	{
		nodePtr->status(PQNodeRoot::PQNodeStatus::Full);
		if (!isRoot)
			nodePtr->m_parent->fullChildren->pushFront(nodePtr);

		return true;
	}
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateP2(PQNode<T,X,Y> **nodePtr)
{
	if ((*nodePtr)->type() != PQNodeRoot::PQNodeType::PNode ||
		(*nodePtr)->partialChildren->size() > 0)
		return false;

	(*nodePtr)->m_childCount =
		(*nodePtr)->m_childCount - (*nodePtr)->fullChildren->size() + 1;
			// Gather all full children of nodePtr
			// as children of the new P-node.
			// Delete them from nodePtr.

	PQNode<T,X,Y> *newNode = createNodeAndCopyFullChildren((*nodePtr)->fullChildren);
		// Correct parent-pointer and
		// sibling-pointers of the new P-node.

	newNode->m_parent = (*nodePtr);
	newNode->m_sibRight = (*nodePtr)->m_referenceChild->m_sibRight;
	newNode->m_sibLeft = newNode->m_sibRight->m_sibLeft;
	newNode->m_sibLeft->m_sibRight = newNode;
	newNode->m_sibRight->m_sibLeft = newNode;
	newNode->m_parentType = PQNodeRoot::PQNodeType::PNode;
		// The new P-node now is the root of
		// the pertinent subtree.
	(*nodePtr) = newNode;

	return true;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateP3(PQNode<T,X,Y> *nodePtr)
{
	/* Variables:
	 *   - \p newPnode is the pointer to the new P-node, or, in case
	 *     that \p nodePtr has only one full child, is the pointer of this child.
	 *   - \p newQnode is the pointer to the new Q-node.
	 *   - \p newNode is used for the proper allocation of the new
	 *     Q-node.
	 *   - \p emptyNode is a pointer to any empty child of \p nodePtr.
	 */
	if (nodePtr->type() != PQNodeRoot::PQNodeType::PNode || nodePtr->partialChildren->size() > 0)
		return false;

	/*
	Create a new partial $Q$-node stored in [[newQnode]].
	It replaces [[nodePtr]] by the Q-node [[newQnode]] in the $PQ$-tree
	and makes [[nodePtr]] endmost child of [[newQnode]].
	This is done by updating parent-pointers and sibling-pointers.
	*/
	PQInternalNode<T,X,Y> *newNode = new PQInternalNode<T,X,Y>(m_identificationNumber++, PQNodeRoot::PQNodeType::QNode, PQNodeRoot::PQNodeStatus::Partial);
	PQNode<T,X,Y>         *newQnode = newNode;
	m_pertinentNodes->pushFront(newQnode);

	exchangeNodes(nodePtr,newQnode);
	nodePtr->m_parent = newQnode;
	nodePtr->m_parentType = PQNodeRoot::PQNodeType::QNode;

	newQnode->m_leftEndmost = (nodePtr);
	newQnode->m_childCount = 1;

	/*
	Create a new full $P$-node stored in [[newPnode]].
	It copies the full children of [[nodePtr]] to [[newPnode]].
	The new $P$-node will then be included into the tree as child of
	the new $Q$-node [[newQnode]].
	*/
	if (nodePtr->fullChildren->size() > 0)
	{
		nodePtr->m_childCount = nodePtr->m_childCount -
			nodePtr->fullChildren->size();

		PQNode<T,X,Y> *newPnode = createNodeAndCopyFullChildren(nodePtr->fullChildren);
		newPnode->m_parentType = PQNodeRoot::PQNodeType::QNode;

		// Update newQnode.
		newQnode->m_childCount++;
		newQnode->fullChildren->pushFront(newPnode);
		// Update sibling pointers.
		nodePtr->m_sibRight = newPnode;
		newPnode->m_sibLeft = nodePtr;
		newQnode->m_rightEndmost = newPnode;
		newPnode->m_parent = newQnode;
	}

	// Check if nodePtr contains
	// only one son. If so, nodePtr
	// will be deleted from the tree.
	PQNode<T,X,Y> *emptyNode = nodePtr->m_referenceChild;
	checkIfOnlyChild(emptyNode,nodePtr);
	// Update partialchildren stack of
	// the parent of the new Q-node.
	newQnode->m_parent->partialChildren->pushFront(newQnode);

	return true;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateP4(PQNode<T,X,Y> **nodePtr)
{
	if ((*nodePtr)->type() != PQNodeRoot::PQNodeType::PNode   ||
		(*nodePtr)->partialChildren->size() != 1)
		return false;

	PQNode<T,X,Y> *partialChild = (*nodePtr)->partialChildren->popFrontRet();
	copyFullChildrenToPartial(*nodePtr,partialChild);
		// If nodePtr does not have any
		// empty children, then it has to
		// be deleted and the partial node
		// is occupying its place in the tree.
	checkIfOnlyChild(partialChild,*nodePtr);
		// The partial child now is
		// root of the pertinent subtree.
	*nodePtr = partialChild;

	return true;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateP5(PQNode<T,X,Y> *nodePtr)
{
	/* Variables:
	 *   - \a partialChild is a pointer to the partial child of
	 *     \a nodePtr.
	 *   - \a checkNode is a pointer to the endmost empty child of
	 *     \a partialChild.
	 *   - \a emptyNode is a pointer to the empty node that is copied as
	 *     endmost child to \a partialChild.
	 *   - \a emptyChildCount stores the number of empty children of
	 *     \a nodePtr.
	 *
	 * If neccessary, the function templateP5() creates a new full P-node
	 * and copies all full children of \p nodePtr to this new full P-node.
	 * All empty children of \p nodePtr stay empty children of \p nodePtr.
	 *
	 * The new full P-node and \p nodePtr will be the new endmost children of
	 * \a partialChild. The \a partialChild then occupies the position
	 * of \p nodePtr in the PQ-tree.
	 */
	if ((nodePtr->type() != PQNodeRoot::PQNodeType::PNode) ||
		(nodePtr->partialChildren->size() != 1))
		return false;

	/*
	Remove [[partialChild]] from the children of [[nodePtr]]. The node
	[[partialChild]] then occupies the position of [[nodePtr]] in the
	$PQ$-tree which is done in the function call [[exchangeNodes]]
	(\ref{exchangeNodes}). The chunk then removes all full children from
	[[nodePtr]] and adds them as children of a new $P$-node as endmost
	child of [[partialChild]]. This is done in the function call
	[[copyFullChildrenToPartial]] (\ref{copyFullChildrenToPartial}).
	When this chunk has finished, [[nodePtr]] has only empty children.
	*/
	int emptyChildCount = nodePtr->m_childCount -
						nodePtr->fullChildren->size() - 1;
	PQNode<T,X,Y> *partialChild = nodePtr->partialChildren->popFrontRet();
	nodePtr->m_parent->partialChildren->pushFront(partialChild);
	removeChildFromSiblings(partialChild);
	exchangeNodes(nodePtr,partialChild);
	copyFullChildrenToPartial(nodePtr,partialChild);

	if (emptyChildCount > 0)
	{
		/*
		Check if [[nodePtr]] has just one empty child. If so, the child
		is stored in [[emptyNode]] in order to be added to the empty
		side of the partial $Q$-node [[partialChild]]. If [[nodePtr]]
		has more than one empty child, [[nodePtr]] is stored in
		[[emptyNode]] in order to be added to the empty
		side of the partial $Q$-node [[partialChild]].
		*/
		PQNode<T,X,Y> *emptyNode;
		if (emptyChildCount == 1)
		{
			emptyNode = nodePtr->m_referenceChild;
			removeChildFromSiblings(emptyNode);

		} else {
			emptyNode = nodePtr;
			emptyNode->m_childCount = emptyChildCount;
		}

		/*
		Check at which side of [[partialChild]]
		the empty children hide. [[emptyNode]] stores the empty node
		that is added to the empty side of [[partialChild]].
		*/
		PQNode<T,X,Y> *checkNode;
		if (clientLeftEndmost(partialChild)->status() == PQNodeRoot::PQNodeStatus::Empty)
		{
			checkNode = partialChild->m_leftEndmost;
			partialChild->m_leftEndmost = emptyNode;
		}
		else {
			// Endmostchild not found?
			OGDF_ASSERT(clientRightEndmost(partialChild)->status() == PQNodeRoot::PQNodeStatus::Empty);

			checkNode = partialChild->m_rightEndmost;
			partialChild->m_rightEndmost = emptyNode;
		}

		linkChildrenOfQnode(checkNode,emptyNode);
		emptyNode->m_parent = partialChild;
		emptyNode->m_parentType = PQNodeRoot::PQNodeType::QNode;
		partialChild->m_childCount++;
	}
		// If nodePtr did not have any empty
		// children it has to be deleted.
	if (emptyChildCount <= 1)
		destroyNode(nodePtr);

	return true;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateP6(PQNode<T,X,Y> **nodePtr)
{
	/* Variables:
	 *   - \a partial_1 is a pointer to the first partial child of
	 *     \p nodePtr.
	 *   - \a partial_2 is a pointer to the second partial child of
	 *     \p nodePtr.
	 *   - \a fullEnd_1 is a pointer to a full endmost child of \a partial_1.
	 *   - \a fullEnd_2 is a pointer to a full endmost child of \a partial_2.
	 *   - \a emptyEnd_2 is a pointer to the empty endmost child (more
	 *     precisely: to the endmost child appearing on the empty side) of
	 *     \a partial_2. In case that <em>ignored nodes</em> are used, this
	 *     \a emptyEnd_2 may store the adress of an ignored node.
	 *   - \a realEmptyEnd_2 is a pointer to the first <em>non ignored</em>
	 *     node with empty status on the empty side of \a partial_2. In case
	 *     that no ignored nodes are used, \a realEmpty_2 is identical to
	 *     \a endEmpty_2.
	 */
#if 0
	PQNode<T,X,Y>  *partial_1       = 0;
	PQNode<T,X,Y>  *partial_2       = 0;
	PQNode<T,X,Y>  *fullEnd_1       = 0;
	PQNode<T,X,Y>  *fullEnd_2       = 0;
	PQNode<T,X,Y>  *emptyEnd_2      = 0;
	PQNode<T,X,Y>  *realEmptyEnd_2  = 0;
#endif

	if ((*nodePtr)->type() != PQNodeRoot::PQNodeType::PNode   ||
		(*nodePtr)->partialChildren->size() != 2)
		return false;

	/*
	Get the partial children of [[nodePtr]] and removes the second
	partial child stored in [[partial_2]] from the children of
	[[nodePtr]]. If there are any full children of [[nodePtr]], the
	chunk removes them from the children of [[nodePtr]] and copies them
	as children to a new $P$-node. This new $P$-node is then made
	endmost child of [[partial_1]].
	*/
	PQNode<T,X,Y> *partial_1 = (*nodePtr)->partialChildren->popFrontRet();
	PQNode<T,X,Y> *partial_2 = (*nodePtr)->partialChildren->popFrontRet();

	removeChildFromSiblings(partial_2);
	(*nodePtr)->m_childCount--;
	copyFullChildrenToPartial(*nodePtr,partial_1);

	/*
	Check the endmost children of the two partial children of [[nodePtr]]
	and stores them at approriate places, remembering what kind of type
	the endmost children are.
	*/
	PQNode<T,X,Y> *fullEnd_1;
	if (clientLeftEndmost(partial_1)->status() == PQNodeRoot::PQNodeStatus::Full)
		fullEnd_1 = partial_1->m_leftEndmost;
	else {
		// partial child with no Full endmost child detected?
		OGDF_ASSERT(clientRightEndmost(partial_1)->status() == PQNodeRoot::PQNodeStatus::Full);
		fullEnd_1 = partial_1->m_rightEndmost;
	}

	PQNode<T,X,Y> *fullEnd_2      = nullptr;
	PQNode<T,X,Y> *emptyEnd_2     = nullptr;
	PQNode<T,X,Y> *realEmptyEnd_2 = nullptr;
	if (clientLeftEndmost(partial_2)->status() == PQNodeRoot::PQNodeStatus::Full)
		fullEnd_2 = partial_2->m_leftEndmost;
	else {
		// partial child with no Full or Empty endmost child detected?
		OGDF_ASSERT(clientLeftEndmost(partial_2)->status() == PQNodeRoot::PQNodeStatus::Empty);

		emptyEnd_2 = partial_2->m_leftEndmost;
		realEmptyEnd_2 = clientLeftEndmost(partial_2);
	}

	if (clientRightEndmost(partial_2)->status() == PQNodeRoot::PQNodeStatus::Full)
		fullEnd_2 = partial_2->m_rightEndmost;
	else {
		// partial child with no Full or Empty endmost child detected?
		OGDF_ASSERT(clientRightEndmost(partial_2)->status() == PQNodeRoot::PQNodeStatus::Empty);

		emptyEnd_2 = partial_2->m_rightEndmost;
		realEmptyEnd_2 = clientRightEndmost(partial_2);
	}

	OGDF_ASSERT(fullEnd_2 != emptyEnd_2);
	//partial child with same type of endmost child detected

	/*
	The children of [[partial_2]] are removed from their parent and
	added as children to [[partial_1]]. This is done by resetting the
	sibling pointers of the two endmost children of [[partial_1]] and
	[[partial_2]] and the endmost child pointers of [[partial_1]].
	Observe that the parent pointers are not updated. The node
	[[partial_2]] is deleted.
	*/
	while (!partial_2->fullChildren->empty())
		partial_1->fullChildren->pushFront(partial_2->fullChildren->popFrontRet());
	linkChildrenOfQnode(fullEnd_1,fullEnd_2);
	if (partial_1->m_leftEndmost == fullEnd_1)
		partial_1->m_leftEndmost = emptyEnd_2;
	else
		partial_1->m_rightEndmost = emptyEnd_2;

	emptyEnd_2->m_parent = partial_1;
	emptyEnd_2->m_parentType = PQNodeRoot::PQNodeType::QNode;

	realEmptyEnd_2->m_parent = partial_1;
	realEmptyEnd_2->m_parentType = PQNodeRoot::PQNodeType::QNode;

	partial_1->m_childCount = partial_1->m_childCount +
		partial_2->m_childCount;
	destroyNode(partial_2);

		// If nodePtr does not have any
		// empty children, then it has to
		// be deleted and the partial node
		// is occupying its place in the tree.
	checkIfOnlyChild(partial_1,*nodePtr);
		// partial_1 is now root of the
		// pertinent subtree.
	*nodePtr = partial_1;

	return true;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateQ1(PQNode<T,X,Y> *nodePtr, bool isRoot)
{
	if (nodePtr->type() == PQNodeRoot::PQNodeType::QNode &&
		nodePtr != m_pseudoRoot &&
		clientLeftEndmost(nodePtr)->status() == PQNodeRoot::PQNodeStatus::Full &&
		clientRightEndmost(nodePtr)->status() == PQNodeRoot::PQNodeStatus::Full)
	{
		PQNode<T,X,Y>* seqStart = nullptr;
		PQNode<T,X,Y>* seqEnd = nullptr;
		if (checkChain(nodePtr,clientLeftEndmost(nodePtr),&seqStart,&seqEnd))
		{
			nodePtr->status(PQNodeRoot::PQNodeStatus::Full);
			if (!isRoot)
				nodePtr->m_parent->fullChildren->pushFront(nodePtr);
			return true;
		}
	}

	return false;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateQ2(PQNode<T,X,Y> *nodePtr,bool isRoot)
{
	/* Variables:
	 *   - \a fullNode is a pointer to the full endmost child of \p nodePtr.
	 *   - \a sequenceBegin is a pointer to the first node of the
	 *     sequence of full children. Identical to the node fullNode and
	 *     mainly needed by the function checkChain().
	 *   - \a sequenceEnd is a pointer to the last node of the sequence
	 *     of full children. Is set by the function checkChain().
	 *   - \a partialChild is a pointer to the partial child of \p nodePtr.
	 *   - \a sequenceCons is 1 if all full children of
	 *     \p nodePtr form a consecutive sequence with one full child beeing
	 *     an endmost child of \p nodePtr \b and the partial child is
	 *     adjacent to the sequence.
	 *
	 * templateQ2() first checks if one of the above mentioned cases
	 * occures and
	 * then applies the necessary template matching.
	 * No special action has to be performed for the full nodes. If there exists
	 * a partial child that will be stored in \a partialChild, its children
	 * are made children of \p nodePtr. So to say, \a partialChild is lifted
	 * up to the Q-node \p nodePtr and the occurance of the children
	 * of \a partialChild is fixed within the children of \p nodePtr.
	 * (Remember that a partial child is also a Q-node).
	 */
	if (nodePtr->type() != PQNodeRoot::PQNodeType::QNode ||
		nodePtr->partialChildren->size() > 1)
		return false;

	bool sequenceCons = false;
	if (nodePtr->fullChildren->size() > 0)
	{
		/*
		Get a full endmost child of
		the $Q$-node [[nodePtr]] if there exists one.
		*/
		PQNode<T,X,Y> *fullNode = nullptr;
		if (nodePtr->m_leftEndmost != nullptr)
		{
			fullNode = clientLeftEndmost(nodePtr);
			if (fullNode->status() != PQNodeRoot::PQNodeStatus::Full)
				fullNode = nullptr;
		}
		if (nodePtr->m_rightEndmost != nullptr  && fullNode == nullptr)
		{
			fullNode = clientRightEndmost(nodePtr);
			if (fullNode->status() != PQNodeRoot::PQNodeStatus::Full)
				fullNode = nullptr;
		}

		/*
		In case that a full endmost child of [[nodePtr]] exists, this
		child has been stored in [[fullNode]] and the chunk checks by
		calling the function [[checkChain]] (\ref{checkChain}), if all
		full children of [[nodePtr]] form a consecutive sequence.
		In case that the full children
		of [[nodePtr]] form a consecutive sequence the
		return value of [[checkChain]] is [[1]]. If a partial child
		stored in [[partialChild]] exists, the chunk checks if
		[[partialChild]] is adjacent to the sequence of full children.
		If the latter case is [[1]], the flag [[sequenceCons]] is
		set to [[1]] and the function [[templateQ2]] is allowed to
		reduce the pertient children of [[nodePtr]].
		*/
		PQNode<T,X,Y> *sequenceBegin = nullptr;
		PQNode<T,X,Y> *sequenceEnd   = nullptr;
		if (fullNode != nullptr)
			sequenceCons = checkChain(nodePtr,fullNode,&sequenceBegin,&sequenceEnd);

		if (sequenceCons && nodePtr->partialChildren->size() == 1)
		{
			PQNode<T,X,Y> *partialChild = nodePtr->partialChildren->front();
			sequenceCons = false;

			if (clientSibLeft(sequenceEnd) == partialChild ||
				clientSibRight(sequenceEnd) == partialChild)
				sequenceCons = true;
		}
	}
	else
	{
		if (!nodePtr->partialChildren->empty())
		{
			/*
			If the $Q$-node [[nodePtr]] has no full children but one
			partial child this chunk checks, if the partial child is
			endmost child of the [[nodePtr]]. If this is not the case,
			[[nodePtr]] cannot be reduced by the template matching
			<b>Q2</b>.
			*/
#if 0
			nodePtr->partialChildren->startAtBottom();
			partialChild = nodePtr->partialChildren->readNext();
#endif
			PQNode<T,X,Y> *partialChild = nodePtr->partialChildren->front();
			if ((clientLeftEndmost(nodePtr) == partialChild) ||
				(clientRightEndmost(nodePtr) == partialChild))
				sequenceCons = true;
		}
	}

	if (sequenceCons)
		removeBlock(nodePtr,isRoot);

	return sequenceCons;
}

template<class T,class X,class Y>
bool PQTree<T,X,Y>::templateQ3(PQNode<T,X,Y> *nodePtr)
{
	/* Variables:
	 *   - \a fullChild is a pointer to an arbitrary full child of \p nodePtr.
	 *   - \a fullStart is a pointer to the first full child of a
	 *     consecutive sequence of full children.
	 *   - \a fullEnd is a pointer to the last full child of a
	 *     consecutive sequence of full children.
	 *   - \a partial_1 is a pointer to the first partial child of \p nodePtr.
	 *   - \a partial_2 is a pointer to the second partial child of \p nodePtr.
	 *   - \a conssequence is 1 if the pertinent children of
	 *     \p nodePtr form a consecutive sequence with at most one partial
	 *     child at every end of the sequence.
	 *   - \a found is a help variable.
	 *
	 * templateQ3() first checks if one of the above mentioned cases
	 * occures and then applies the neccessary template matching.
	 * No special action has to be performed for the full nodes. If there exist
	 * one or two partial children which will be stored in \a partial_1
	 * or \a partial_2, their children
	 * are made children of \p nodePtr. So to say, \a partial_1 and
	 * partial_2 are lifted up to the Q-node \p nodePtr
	 * and the occurance of their children
	 * is fixed within the children of \p nodePtr.
	 */
	if (nodePtr->type() != PQNodeRoot::PQNodeType::QNode || nodePtr->partialChildren->size() >= 3)
		return false;

	bool conssequence = false;
	bool found        = false;

	/*
	Check ifthe
	pertinent children of [[nodePtr]] form a consecutive sequence. We
	differ between two cases:
	\begin{enumerate}
	\item There exist full children of [[nodePtr]]. First check with
	the function [[checkChain]] (\ref{checkChain}) if the full children
	form a consecutive sequence. In case that the check was
	successful, check if each partial child is adjacent to a full child.
	If both checks were successful, the pertient children form a
	consecutive sequence.
	\item There do not exist full children. Check if the partial
	children (there are at most two of them) form a consecutive sequence.
	If the test was successful, the pertinent children form a
	consecutive sequence.
	*/
	if (!nodePtr->fullChildren->empty())
	{
		/*
		A consecutive
		sequence of full children has been detected, containing all full
		children of [[nodePtr]]. The chunk checks if each partial child
		of [[nodePtr]] is adjacent to a full child. Observe that the
		function [[templateQ3]] only reaches this chunk when [[nodePtr]]
		has less than three partial children.
		*/
		PQNode<T,X,Y> *fullChild = nodePtr->fullChildren->front();
		PQNode<T,X,Y> *fullStart = nullptr;
		PQNode<T,X,Y> *fullEnd   = nullptr;
		conssequence = checkChain(nodePtr,fullChild,&fullStart,&fullEnd);
		if (conssequence)
		{
			ListIterator<PQNode<T,X,Y>*> it;
			for (it = nodePtr->partialChildren->begin(); it.valid(); ++it)
			{
				PQNode<T,X,Y> *partial_1 = *it;
				found = false;
				if ((clientSibLeft(fullStart) == partial_1)  ||
					(clientSibRight(fullStart) == partial_1) ||
					(clientSibLeft(fullEnd) == partial_1)    ||
					(clientSibRight(fullEnd) == partial_1)     )
					found = true;
				if (!found)
					conssequence = found;
			}
		}
	}

	else if (nodePtr->partialChildren->size() == 2)
	{
		/*
		In case that the node [[nodePtr]] does not have any full children,
		this chunk checks if the partial children are adjacent.
		*/
		PQNode<T,X,Y> *partial_1 = nodePtr->partialChildren->front();
		PQNode<T,X,Y> *partial_2 = nodePtr->partialChildren->back();
		if ((clientSibLeft(partial_1) == partial_2)  ||
			(clientSibRight(partial_1) == partial_2)   )
			found = true;
		conssequence = found;
	}

	if (conssequence)
		removeBlock(nodePtr,true);

	return conssequence;
}

}
