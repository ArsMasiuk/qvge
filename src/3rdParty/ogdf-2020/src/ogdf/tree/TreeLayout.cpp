/** \file
 * \brief Linear time layout algorithm for trees (TreeLayout)
 * based on Walker's algorithm
 *
 * \author Christoph Buchheim
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


#include <ogdf/tree/TreeLayout.h>
#include <ogdf/basic/AdjEntryArray.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {


TreeLayout::TreeLayout()
	:m_siblingDistance(20),
	 m_subtreeDistance(20),
	 m_levelDistance(50),
	 m_treeDistance(50),
	 m_orthogonalLayout(false),
	 m_orientation(Orientation::topToBottom),
	 m_selectRoot(RootSelectionType::Source)
{ }


TreeLayout::TreeLayout(const TreeLayout &tl)
	:m_siblingDistance(tl.m_siblingDistance),
	 m_subtreeDistance(tl.m_subtreeDistance),
	 m_levelDistance(tl.m_levelDistance),
	 m_treeDistance(tl.m_treeDistance),
	 m_orthogonalLayout(tl.m_orthogonalLayout),
	 m_orientation(tl.m_orientation),
	 m_selectRoot(tl.m_selectRoot)
{ }


TreeLayout &TreeLayout::operator=(const TreeLayout &tl)
{
	m_siblingDistance  = tl.m_siblingDistance;
	m_subtreeDistance  = tl.m_subtreeDistance;
	m_levelDistance    = tl.m_levelDistance;
	m_treeDistance     = tl.m_treeDistance;
	m_orthogonalLayout = tl.m_orthogonalLayout;
	m_orientation      = tl.m_orientation;
	m_selectRoot       = tl.m_selectRoot;
	return *this;
}


struct TreeLayout::TreeStructure {

	GraphAttributes &m_ga;
	NodeArray<int> m_number;         //!< Consecutive numbers for children.

	NodeArray<node> m_parent;        //!< Parent node, 0 if root.
	NodeArray<node> m_leftSibling;   //!< Left sibling, 0 if none.
	NodeArray<node> m_firstChild;    //!< Leftmost child, 0 if leaf.
	NodeArray<node> m_lastChild;	 //!< Rightmost child, 0 if leaf.
	NodeArray<node> m_thread;        //!< Thread, 0 if none.
	NodeArray<node> m_ancestor;      //!< Actual highest ancestor.

	NodeArray<double> m_preliminary; //!< Preliminary x-coordinates.
	NodeArray<double> m_modifier;    //!< Modifier of x-coordinates.
	NodeArray<double> m_change;      //!< Change of shift applied to subtrees.
	NodeArray<double> m_shift;       //!< Shift applied to subtrees.


	// initialize all node arrays and
	// compute the tree structure from the adjacency lists
	//
	// returns the root nodes in roots
	TreeStructure(const Graph &tree, GraphAttributes &GA, List<node> &roots) :
		m_ga(GA),
		m_number(tree, 0),
		m_parent(tree, nullptr),
		m_leftSibling(tree, nullptr),
		m_firstChild(tree, nullptr),
		m_lastChild(tree, nullptr),
		m_thread(tree, nullptr),
		m_ancestor(tree, nullptr),
		m_preliminary(tree, 0),
		m_modifier(tree, 0),
		m_change(tree, 0),
		m_shift(tree, 0)
	{
		// compute the tree structure

		// find the roots
		//node root = 0;
		for (node v : tree.nodes) {
			if (v->indeg() == 0)
				roots.pushBack(v);
		}

		int childCounter;
		for (node v : tree.nodes) {

			// determine
			// - the parent node of v
			// - the leftmost and rightmost child of v
			// - the numbers of the children of v
			// - the left siblings of the children of v
			// and initialize the actual ancestor of v

			m_ancestor[v] = v;
			if (isLeaf(v)) {
				if (v->indeg() == 0) { // is v a root
					m_parent[v] = nullptr;
					m_leftSibling[v] = nullptr;
				}
				else {
					m_firstChild[v] = m_lastChild[v] = nullptr;
					m_parent[v] = v->firstAdj()->theEdge()->source();
				}
			}
			else {

				// traverse the adjacency list of v
				adjEntry first;    // first leaving edge
				adjEntry stop;     // successor of last leaving edge
				first = v->firstAdj();
				if (v->indeg() == 0) { // is v a root
					stop = first;
					m_parent[v] = nullptr;
					m_leftSibling[v] = nullptr;
				}
				else {

					// search for first leaving edge
					while (first->theEdge()->source() == v)
						first = first->cyclicSucc();
					m_parent[v] = first->theEdge()->source();
					stop = first;
					first = first->cyclicSucc();
				}

				// traverse the children of v
				m_firstChild[v] = first->theEdge()->target();
				m_number[m_firstChild[v]] = childCounter = 0;
				m_leftSibling[m_firstChild[v]] = nullptr;
				adjEntry previous = first;
				while (first->cyclicSucc() != stop) {
					first = first->cyclicSucc();
					m_number[first->theEdge()->target()] = ++childCounter;
					m_leftSibling[first->theEdge()->target()]
						= previous->theEdge()->target();
					previous = first;
				}
				m_lastChild[v] = first->theEdge()->target();
			}
		}
	}

	// returns whether node v is a leaf
	bool isLeaf(node v) const
	{
		OGDF_ASSERT(v != nullptr);

		// node v is a leaf if and only if no edge leaves v
		return v->outdeg() == 0;
	}

	// returns the successor of node v on the left contour
	// returns 0 if there is none
	node nextOnLeftContour(node v) const
	{
		OGDF_ASSERT(v != nullptr);
		OGDF_ASSERT(v->graphOf() == m_firstChild.graphOf());
		OGDF_ASSERT(v->graphOf() == m_thread.graphOf());

		// if v has children, the successor of v on the left contour
		// is its leftmost child,
		// otherwise, the successor is the thread of v (may be 0)
		if (m_firstChild[v] != nullptr)
			return m_firstChild[v];
		else
			return m_thread[v];
	}

	// returns the successor of node v on the right contour
	// returns 0 if there is none
	node nextOnRightContour(node v) const
	{
		OGDF_ASSERT(v != nullptr);
		OGDF_ASSERT(v->graphOf() == m_lastChild.graphOf());
		OGDF_ASSERT(v->graphOf() == m_thread.graphOf());

		// if v has children, the successor of v on the right contour
		// is its rightmost child,
		// otherwise, the successor is the thread of v (may be 0)
		if (m_lastChild[v] != nullptr)
			return m_lastChild[v];
		else
			return m_thread[v];
	}

};


void TreeLayout::setRoot(GraphAttributes &AG, Graph &tree, SListPure<edge> &reversedEdges)
{
	NodeArray<bool> visited(tree,false);
	ArrayBuffer<node> S;

	for(node v : tree.nodes)
	{
		if(visited[v]) continue;

		// process a new connected component
		node root = nullptr;
		S.push(v);

		while(!S.empty())
		{
			node x = S.popRet();
			visited[x] = true;

			if(!root) {
				if(m_selectRoot == RootSelectionType::Source) {
					if (x->indeg() == 0)
						root = x;
				} else if (m_selectRoot == RootSelectionType::Sink) {
					if (x->outdeg() == 0)
						root = x;
				} else { // selectByCoordinate
					root = x;
				}

			} else if(m_selectRoot == RootSelectionType::ByCoord) {
				switch(m_orientation)
				{
				case Orientation::bottomToTop:
					if(AG.y(x) < AG.y(root))
						root = x;
					break;
				case Orientation::topToBottom:
					if(AG.y(x) > AG.y(root))
						root = x;
					break;
				case Orientation::leftToRight:
					if(AG.x(x) < AG.x(root))
						root = x;
					break;
				case Orientation::rightToLeft:
					if(AG.x(x) > AG.x(root))
						root = x;
					break;
				}
			}

			for(adjEntry adj : x->adjEntries) {
				node w = adj->twinNode();
				if(!visited[w])
					S.push(w);
			}
		}

		OGDF_ASSERT(root != nullptr);

		adjustEdgeDirections(tree, reversedEdges, root, nullptr);
	}
}


void TreeLayout::adjustEdgeDirections(Graph &G, SListPure<edge> &reversedEdges, node v, node parent)
{
	for (adjEntry adj : v->adjEntries) {
		node w = adj->twinNode();
		if (w == parent) continue;
		edge e = adj->theEdge();
		if (w != e->target()) {
			G.reverseEdge(e);
			reversedEdges.pushBack(e);
		}
		adjustEdgeDirections(G, reversedEdges, w, v);
	}
}


void TreeLayout::callSortByPositions(GraphAttributes &AG, Graph &tree)
{
	OGDF_ASSERT(&tree == &(AG.constGraph()));

	OGDF_ASSERT(isAcyclicUndirected(tree));

	SListPure<edge> reversedEdges;
	setRoot(AG, tree, reversedEdges);

	// stores angle of adjacency entry
	AdjEntryArray<double> angle(tree);

	GenericComparer<adjEntry, double> cmp(angle);

	for(node v : tree.nodes)
	{
		// position of node v
		double cx = AG.x(v);
		double cy = AG.y(v);

		for(adjEntry adj : v->adjEntries)
		{
			// adjacent node
			node w = adj->twinNode();

			// relative position of w to v
			double dx = AG.x(w) - cx;
			double dy = AG.y(w) - cy;

			// if v and w lie on the same point ...
			if (dx == 0 && dy == 0) {
				angle[adj] = 0;
				continue;
			}

			if(m_orientation == Orientation::leftToRight || m_orientation == Orientation::rightToLeft)
				std::swap(dx, dy);
			if(m_orientation == Orientation::topToBottom || m_orientation == Orientation::rightToLeft)
				dy = -dy;

			// compute angle of adj
			double alpha = atan2(fabs(dx),fabs(dy));

			if(dx < 0) {
				if(dy < 0)
					angle[adj] = alpha;
				else
					angle[adj] = Math::pi - alpha;
			} else {
				if (dy > 0)
					angle[adj] = Math::pi + alpha;
				else
					angle[adj] = 2*Math::pi - alpha;
			}
		}

		// get list of all adjacency entries at v
		SListPure<adjEntry> entries;
		v->allAdjEntries(entries);

		// sort entries according to angle
		entries.quicksort(cmp);

		// sort entries accordingly in tree
		tree.sort(v,entries);
	}

	// adjacency lists are now sorted, so we can apply the usual call
	call(AG);

	// restore temporarily removed edges again
	undoReverseEdges(AG, tree, reversedEdges);
}


void TreeLayout::call(GraphAttributes &AG)
{
	const Graph &tree = AG.constGraph();
	if(tree.numberOfNodes() == 0) return;

	OGDF_ASSERT(isArborescenceForest(tree));
	OGDF_ASSERT(m_siblingDistance > 0);
	OGDF_ASSERT(m_subtreeDistance > 0);
	OGDF_ASSERT(m_levelDistance > 0);

	// compute the tree structure
	List<node> roots;
	TreeStructure ts(tree, AG, roots);

	if(m_orientation == Orientation::topToBottom || m_orientation == Orientation::bottomToTop)
	{
		double minX = 0, maxX = 0;
		for(ListConstIterator<node> it = roots.begin(); it.valid(); ++it)
		{
			node root = *it;

			// compute x-coordinates
			firstWalk(ts, root, true);
			secondWalkX(ts, root, -ts.m_preliminary[root]);

			// compute y-coordinates
			computeYCoordinatesAndEdgeShapes(root,AG);

			if(it != roots.begin())
			{
				findMinX(AG,root,minX);

				double shift = maxX + m_treeDistance - minX;

				shiftTreeX(AG,root,shift);
			}

			findMaxX(AG,root,maxX);
		}

		// The computed layout draws a tree downwards. If we want to draw the
		// tree upwards, we simply invert all y-coordinates.
		if(m_orientation == Orientation::bottomToTop)
		{
			for(node v : tree.nodes)
				AG.y(v) = -AG.y(v);

			for(edge e : tree.edges) {
				for(DPoint &p: AG.bends(e))
					p.m_y = -p.m_y;
			}
		}

	} else {
		ListConstIterator<node> it;
		double minY = 0, maxY = 0;
		for(it = roots.begin(); it.valid(); ++it)
		{
			node root = *it;

			// compute y-coordinates
			firstWalk(ts, root, false);
			secondWalkY(ts, root, -ts.m_preliminary[root]);

			// compute y-coordinates
			computeXCoordinatesAndEdgeShapes(root,AG);

			if(it != roots.begin())
			{
				findMinY(AG,root,minY);

				double shift = maxY + m_treeDistance - minY;

				shiftTreeY(AG,root,shift);
			}

			findMaxY(AG,root,maxY);
		}

		// The computed layout draws a tree upwards. If we want to draw the
		// tree downwards, we simply invert all y-coordinates.
		if(m_orientation == Orientation::rightToLeft)
		{
			for(node v : tree.nodes)
				AG.x(v) = -AG.x(v);

			for(edge e : tree.edges) {
				for(DPoint &p: AG.bends(e))
					p.m_x = -p.m_x;
			}
		}

	}
}

void TreeLayout::undoReverseEdges(GraphAttributes &AG, Graph &tree, SListPure<edge> &reversedEdges)
{
#if 0
	if(m_pGraph) {
#endif
		while(!reversedEdges.empty()) {
			edge e = reversedEdges.popFrontRet();
			tree.reverseEdge(e);
			AG.bends(e).reverse();
		}

#if 0
		m_pGraph = nullptr;
	}
#endif
}

void TreeLayout::findMinX(GraphAttributes &AG, node root, double &minX)
{
	ArrayBuffer<node> S;
	S.push(root);

	while(!S.empty())
	{
		node v = S.popRet();

		double left = AG.x(v) - AG.width(v)/2;
		if(left < minX) minX = left;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->target();
			if(w != v) S.push(w);
		}
	}
}

void TreeLayout::findMinY(GraphAttributes &AG, node root, double &minY)
{
	ArrayBuffer<node> S;
	S.push(root);

	while(!S.empty())
	{
		node v = S.popRet();

		double left = AG.y(v) - AG.height(v)/2;
		if(left < minY) minY = left;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->target();
			if(w != v) S.push(w);
		}
	}
}


void TreeLayout::shiftTreeX(GraphAttributes &AG, node root, double shift)
{
	ArrayBuffer<node> S;
	S.push(root);
	while(!S.empty())
	{
		node v = S.popRet();

		AG.x(v) += shift;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->target();
			if(w != v) {
				ListIterator<DPoint> itP;
				for(itP = AG.bends(e).begin(); itP.valid(); ++itP)
					(*itP).m_x += shift;
				S.push(w);
			}
		}
	}
}

void TreeLayout::shiftTreeY(GraphAttributes &AG, node root, double shift)
{
	ArrayBuffer<node> S;
	S.push(root);
	while(!S.empty())
	{
		node v = S.popRet();

		AG.y(v) += shift;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->target();
			if(w != v) {
				ListIterator<DPoint> itP;
				for(itP = AG.bends(e).begin(); itP.valid(); ++itP)
					(*itP).m_y += shift;
				S.push(w);
			}
		}
	}
}


void TreeLayout::findMaxX(GraphAttributes &AG, node root, double &maxX)
{
	ArrayBuffer<node> S;
	S.push(root);
	while(!S.empty())
	{
		node v = S.popRet();

		double right = AG.x(v) + AG.width(v)/2;
		if(right > maxX) maxX = right;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->target();
			if(w != v) S.push(w);
		}
	}
}

void TreeLayout::findMaxY(GraphAttributes &AG, node root, double &maxY)
{
	ArrayBuffer<node> S;
	S.push(root);
	while(!S.empty())
	{
		node v = S.popRet();

		double right = AG.y(v) + AG.height(v)/2;
		if(right > maxY) maxY = right;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->target();
			if(w != v) S.push(w);
		}
	}
}


void TreeLayout::firstWalk(TreeStructure &ts, node subtree, bool upDown)
{
	OGDF_ASSERT(subtree != nullptr);
	OGDF_ASSERT(subtree->graphOf() == ts.m_leftSibling.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_preliminary.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_firstChild.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_lastChild.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_modifier.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_change.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_shift.graphOf());

	// compute a preliminary x-coordinate for subtree
	if(ts.isLeaf(subtree)) {

		// place subtree close to the left sibling
		node leftSibling = ts.m_leftSibling[subtree];
		if(leftSibling != nullptr) {
			if(upDown) {
				ts.m_preliminary[subtree] = ts.m_preliminary[leftSibling]
					+ (ts.m_ga.width(subtree) + ts.m_ga.width(leftSibling)) / 2
					+ m_siblingDistance;
			} else {
				ts.m_preliminary[subtree] = ts.m_preliminary[leftSibling]
					+ (ts.m_ga.height(subtree) + ts.m_ga.height(leftSibling)) / 2
					+ m_siblingDistance;
			}
		}
		else ts.m_preliminary[subtree] = 0;
	}
	else {
		node defaultAncestor = ts.m_firstChild[subtree];

		// collect the children of subtree
		List<node> children;
		node v = ts.m_lastChild[subtree];
		do {
			children.pushFront(v);
			v = ts.m_leftSibling[v];
		} while(v != nullptr);

		ListIterator<node> it;

		// apply firstwalk and apportion to the children
		for (it = children.begin(); it.valid(); it = it.succ()) {
			firstWalk(ts, *it, upDown);
			apportion(ts, *it, defaultAncestor, upDown);
		}

		// shift the small subtrees
		double shift = 0;
		double change = 0;
		children.reverse();
		for(it = children.begin(); it.valid(); it = it.succ()) {
			ts.m_preliminary[*it] += shift;
			ts.m_modifier[*it] += shift;
			change += ts.m_change[*it];
			shift += ts.m_shift[*it] + change;
		}

		// place the parent node
		double midpoint = (ts.m_preliminary[children.front()] + ts.m_preliminary[children.back()]) / 2;
		node leftSibling = ts.m_leftSibling[subtree];
		if(leftSibling != nullptr) {
			if(upDown) {
				ts.m_preliminary[subtree] = ts.m_preliminary[leftSibling]
					+ (ts.m_ga.width(subtree) + ts.m_ga.width(leftSibling)) / 2
					+ m_siblingDistance;
			} else {
				ts.m_preliminary[subtree] = ts.m_preliminary[leftSibling]
					+ (ts.m_ga.height(subtree) + ts.m_ga.height(leftSibling)) / 2
					+ m_siblingDistance;
			}
			ts.m_modifier[subtree] =
				ts.m_preliminary[subtree] - midpoint;
		}
		else ts.m_preliminary[subtree] = midpoint;
	}
}

void TreeLayout::apportion(
	TreeStructure &ts,
	node subtree,
	node &defaultAncestor,
	bool upDown)
{
	OGDF_ASSERT(subtree != nullptr);
	OGDF_ASSERT(subtree->graphOf() == defaultAncestor->graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_leftSibling.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_firstChild.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_modifier.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_ancestor.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_change.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_shift.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_thread.graphOf());

	if(ts.m_leftSibling[subtree] == nullptr) return;

	// check distance to the left of the subtree
	// and traverse left/right inside/outside contour

	double leftModSumOut = 0;  // sum of modifiers on left outside contour
	double leftModSumIn = 0;   // sum of modifiers on left inside contour
	double rightModSumIn = 0;  // sum of modifiers on right inside contour
	double rightModSumOut = 0; // sum of modifiers on right outside contour

	double moveDistance;
	int numberOfSubtrees;
	node leftAncestor,rightAncestor;

	// start the traversal at the actual level
	node leftContourOut  = ts.m_firstChild[ts.m_parent[subtree]];
	node leftContourIn   = ts.m_leftSibling[subtree];
	node rightContourIn  = subtree;
	node rightContourOut = subtree;
	bool stop = false;
	do {

		// add modifiers
		leftModSumOut  += ts.m_modifier[leftContourOut];
		leftModSumIn   += ts.m_modifier[leftContourIn];
		rightModSumIn  += ts.m_modifier[rightContourIn];
		rightModSumOut += ts.m_modifier[rightContourOut];

		// actualize ancestor for right contour
		ts.m_ancestor[rightContourOut] = subtree;

		if(ts.nextOnLeftContour(leftContourOut) != nullptr && ts.nextOnRightContour(rightContourOut) != nullptr)
		{
			// continue traversal
			leftContourOut  = ts.nextOnLeftContour(leftContourOut);
			leftContourIn   = ts.nextOnRightContour(leftContourIn);
			rightContourIn  = ts.nextOnLeftContour(rightContourIn);
			rightContourOut = ts.nextOnRightContour(rightContourOut);

			// check if subtree has to be moved
			if(upDown) {
				moveDistance = ts.m_preliminary[leftContourIn] + leftModSumIn
					+ (ts.m_ga.width(leftContourIn) + ts.m_ga.width(rightContourIn)) / 2
					+ m_subtreeDistance
					- ts.m_preliminary[rightContourIn] - rightModSumIn;
			} else {
				moveDistance = ts.m_preliminary[leftContourIn] + leftModSumIn
					+ (ts.m_ga.height(leftContourIn) + ts.m_ga.height(rightContourIn)) / 2
					+ m_subtreeDistance
					- ts.m_preliminary[rightContourIn] - rightModSumIn;
			}
			if(moveDistance > 0) {

				// compute highest different ancestors of leftContourIn
				// and rightContourIn
				if(ts.m_parent[ts.m_ancestor[leftContourIn]] == ts.m_parent[subtree])
					leftAncestor = ts.m_ancestor[leftContourIn];
				else leftAncestor = defaultAncestor;
				rightAncestor = subtree;

				// compute the number of small subtrees in between (plus 1)
				numberOfSubtrees =
					ts.m_number[rightAncestor] - ts.m_number[leftAncestor];

				// compute the shifts and changes of shift
				ts.m_change[rightAncestor] -= moveDistance / numberOfSubtrees;
				ts.m_shift[rightAncestor] += moveDistance;
				ts.m_change[leftAncestor] += moveDistance / numberOfSubtrees;

				// move subtree to the right by moveDistance
				ts.m_preliminary[rightAncestor] += moveDistance;
				ts.m_modifier[rightAncestor] += moveDistance;
				rightModSumIn += moveDistance;
				rightModSumOut += moveDistance;
			}
		}
		else stop = true;
	} while(!stop);

	// adjust threads
	if(ts.nextOnRightContour(rightContourOut) == nullptr && ts.nextOnRightContour(leftContourIn) != nullptr)
	{
		// right subtree smaller than left subforest
		ts.m_thread[rightContourOut] = ts.nextOnRightContour(leftContourIn);
		ts.m_modifier[rightContourOut] += leftModSumIn - rightModSumOut;
	}

	if(ts.nextOnLeftContour(leftContourOut) == nullptr && ts.nextOnLeftContour(rightContourIn) != nullptr)
	{
		// left subforest smaller than right subtree
		ts.m_thread[leftContourOut] = ts.nextOnLeftContour(rightContourIn);
		ts.m_modifier[leftContourOut] += rightModSumIn - leftModSumOut;
		defaultAncestor = subtree;
	}
}


void TreeLayout::secondWalkX(
	TreeStructure &ts,
	node subtree,
	double modifierSum)
{
	OGDF_ASSERT(subtree != nullptr);
	OGDF_ASSERT(subtree->graphOf() == ts.m_preliminary.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_modifier.graphOf());

	// compute final x-coordinates for the subtree
	// by recursively aggregating modifiers
	ts.m_ga.x(subtree) = ts.m_preliminary[subtree] + modifierSum;
	modifierSum += ts.m_modifier[subtree];
	for(adjEntry adj : subtree->adjEntries) {
		edge e = adj->theEdge();
		if (e->target() != subtree)
			secondWalkX(ts, e->target(), modifierSum);
	}
}


void TreeLayout::secondWalkY(
	TreeStructure &ts,
	node subtree,
	double modifierSum)
{
	OGDF_ASSERT(subtree != nullptr);
	OGDF_ASSERT(subtree->graphOf() == ts.m_preliminary.graphOf());
	OGDF_ASSERT(subtree->graphOf() == ts.m_modifier.graphOf());

	// compute final y-coordinates for the subtree
	// by recursively aggregating modifiers
	ts.m_ga.y(subtree) = ts.m_preliminary[subtree] + modifierSum;
	modifierSum += ts.m_modifier[subtree];
	for(adjEntry adj : subtree->adjEntries) {
		node t = adj->theEdge()->target();
		if (t != subtree)
			secondWalkY(ts, t, modifierSum);
	}
}


void TreeLayout::computeYCoordinatesAndEdgeShapes(node root, GraphAttributes &AG)
{
	OGDF_ASSERT(root != nullptr);

	// compute y-coordinates and edge shapes
	List<node> oldLevel;   // the nodes of the old level
	List<node> newLevel;   // the nodes of the new level
	ListIterator<node> it;
	double yCoordinate;    // the y-coordinate for the new level
	double edgeCoordinate; // the y-coordinate for edge bends
	double newHeight;      // the maximal node height on the new level

	// traverse the tree level by level
	newLevel.pushBack(root);
	AG.y(root) = yCoordinate = 0;
	newHeight = AG.height(root);
	while(!newLevel.empty()) {
		double oldHeight = newHeight; // the maximal node height on the old level
		newHeight = 0;
		oldLevel.conc(newLevel);
		while(!oldLevel.empty()) {
			node v = oldLevel.popFrontRet();
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				if(e->target() != v) {
					node w = adj->theEdge()->target();
					newLevel.pushBack(w);

					// compute the shape of edge e
					DPolyline &edgeBends = AG.bends(e);
					edgeBends.clear();
					if(m_orthogonalLayout) {
						edgeCoordinate =
							yCoordinate + (oldHeight + m_levelDistance) / 2;
						edgeBends.pushBack(DPoint(AG.x(v),edgeCoordinate));
						edgeBends.pushBack(DPoint(AG.x(w),edgeCoordinate));
					}

					// compute the maximal node height on the new level
					if(AG.height(e->target()) > newHeight)
						newHeight = AG.height(e->target());
				}
			}
		}

		// assign y-coordinate to the nodes of the new level
		yCoordinate += (oldHeight + newHeight) / 2 + m_levelDistance;
		for(it = newLevel.begin(); it.valid(); it = it.succ())
			AG.y(*it) = yCoordinate;
	}
}

void TreeLayout::computeXCoordinatesAndEdgeShapes(node root, GraphAttributes &AG)
{
	OGDF_ASSERT(root != nullptr);

	// compute y-coordinates and edge shapes
	List<node> oldLevel;   // the nodes of the old level
	List<node> newLevel;   // the nodes of the new level
	ListIterator<node> it;
	double xCoordinate;    // the x-coordinate for the new level
	double edgeCoordinate; // the x-coordinate for edge bends
	double newWidth;       // the maximal node width on the new level

	// traverse the tree level by level
	newLevel.pushBack(root);
	AG.x(root) = xCoordinate = 0;
	newWidth = AG.width(root);
	while(!newLevel.empty()) {
		double oldWidth = newWidth; // the maximal node width on the old level
		newWidth = 0;
		oldLevel.conc(newLevel);
		while(!oldLevel.empty()) {
			node v = oldLevel.popFrontRet();
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				if(e->target() != v) {
					node w = e->target();
					newLevel.pushBack(w);

					// compute the shape of edge e
					DPolyline &edgeBends = AG.bends(e);
					edgeBends.clear();
					if(m_orthogonalLayout) {
						edgeCoordinate =
							xCoordinate + (oldWidth + m_levelDistance) / 2;
						edgeBends.pushBack(DPoint(edgeCoordinate,AG.y(v)));
						edgeBends.pushBack(DPoint(edgeCoordinate,AG.y(w)));
					}

					// compute the maximal node width on the new level
					if(AG.width(e->target()) > newWidth)
						newWidth = AG.width(e->target());
				}
			}
		}

		// assign x-coordinate to the nodes of the new level
		xCoordinate += (oldWidth + newWidth) / 2 + m_levelDistance;
		for(it = newLevel.begin(); it.valid(); it = it.succ())
			AG.x(*it) = xCoordinate;
	}
}

}
