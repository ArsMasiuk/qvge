/** \file
 * \brief Implementation of the third phase of the Sugiyama
 * algorithm
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

#include <ogdf/layered/FastHierarchyLayout.h>

namespace ogdf {

#define forallnodes for(actNode=0;actNode<n;actNode++)
#define foralllayers for(actLayer=0;actLayer<k;actLayer++)
#define forallnodesonlayer \
	for(actNode=first[actLayer];actNode<first[actLayer+1];actNode++)

#define ALLOW .00001

/**
 * \brief Stores a pair of an integer and a double.
 *
 * This class is used by the class kList.
 */
class withKey
{
public:
	int element;
	double key;

	friend std::ostream& operator<<(std::ostream& out,const withKey& wk) {
		out<<wk.element<<"("<<wk.key<<")";
		return out;
	}
	friend std::istream& operator>>(std::istream& in,withKey& wk) {
		in>>wk.element>>wk.key;
		return in;
	}
};

OGDF_DECLARE_COMPARER(cmpWithKey, withKey, int, x.key);

/**
 * \brief Class kList extends the class List by functions needed in the FastHierarchLayout algorithm.
 *
 * Especially, it computes the median of a list and reduces it.
 */
class kList : public List<withKey> {
public:
	bool pop(int& e,double& k) {
		if(empty()) return 0;
		withKey wk=popFrontRet();
		e=wk.element;
		k=wk.key;
		return 1;
	}

	double peek() {
		return front().key;
	}

	void add(int e,double k) {
		withKey wK;
		wK.element=e;
		wK.key=k;
		pushBack(wK);
	}

	double median() const {
		int sz = size();
		if(sz == 0)
			return 0;
		ListConstIterator<withKey> it = get(sz/2);
		double k = (*it).key;
		if (sz == 2*(int) (sz/2))
			k = (k + (*it.pred()).key) / 2;
		return k;
	}


	//! Scans the list for pairs of elements with the same double key.
	/**
	 * Replaces them by one element. If integer key is 0, it removes element from list.
	 * \pre List must be sorted by key (in any direction)
	 */
	void reduce(kList& newList) {
		if (empty()) return;
		withKey oldWK,newWK;
		newWK = oldWK = popFrontRet();

		while (!empty()) {
			oldWK = popFrontRet();
			if (oldWK.key > newWK.key + ALLOW ||
			    oldWK.key < newWK.key - ALLOW) {
				if (newWK.element) {
					newList.pushBack(newWK);
				}
				newWK = oldWK;
			} else {
				newWK.element += oldWK.element;
			}
		}
		if (newWK.element) {
			newList.pushBack(newWK);
		}
	}
};



// ** Member functions of FastHierarchyLayout **

FastHierarchyLayout::FastHierarchyLayout()
{
	m_minNodeDist    = LayoutStandards::defaultNodeSeparation();
	m_minLayerDist   = 1.5 * LayoutStandards::defaultNodeSeparation();
	m_fixedLayerDist = false;
}


FastHierarchyLayout::FastHierarchyLayout(const FastHierarchyLayout &fhl)
{
	m_minNodeDist    = fhl.m_minNodeDist;
	m_minLayerDist   = fhl.m_minLayerDist;
	m_fixedLayerDist = fhl.m_fixedLayerDist;
}


FastHierarchyLayout &FastHierarchyLayout::operator=(const FastHierarchyLayout &fhl)
{
	m_minNodeDist    = fhl.m_minNodeDist;
	m_minLayerDist   = fhl.m_minLayerDist;
	m_fixedLayerDist = fhl.m_fixedLayerDist;

	return *this;
}


void FastHierarchyLayout::doCall(const HierarchyLevelsBase &levels,
	GraphAttributes &AGC)
{
	const Hierarchy &H  = levels.hierarchy();
	const GraphCopy &GC = H;

	if (GC.numberOfNodes() == 0) {
		return;
	}

	int actLayer = 0;
	int actNode = 0;
	List<int> *newEdge;

	// initialize class variables

	n		= GC.numberOfNodes();
	m		= GC.numberOfEdges();
	k		= levels.size();
	x		= new double[n];
	breadth		= new double[n];
	layer		= new int[n];
	adj[0]		= new List<int>[n];
	adj[1]		= new List<int>[n];
	virt		= new bool[n];
	longEdge	= new List<int>*[n];
	height		= new double[k];
	y			= new double[k];
	first		= new int[k+1];

	// CG
	forallnodes {
		longEdge[actNode] = nullptr;
	}

	// Compute first.
	first[0]	= 0;
	foralllayers {
		first[actLayer+1] = first[actLayer]+levels[actLayer].size();
		height[actLayer] = 0;
	}

	// Compute
	//    breadth, height, virt, longEdge for nonvirtual nodes
	for(node v1 : GC.nodes) {
		int n1 = first[H.rank(v1)] + levels.pos(v1); // Number nodes top down and from left to right
		virt[n1] = H.isLongEdgeDummy(v1);
		breadth[n1] = 0;
		layer[n1] = H.rank(v1);
		if(!virt[n1]) {
			breadth[n1] = getWidth(AGC, levels, v1);
			incrTo(height[layer[n1]], getHeight(AGC, levels, v1));
			newEdge = new List<int>;
			newEdge->pushBack(n1);
			longEdge[n1] = newEdge;
		}
	}

	// compute long Edge for virtual nodes
	for(edge e : GC.edges)
	{
		edge e1 = GC.original(e);
		//if(GC.chain(e1).size() > 1) {
		if(e1 && GC.chain(e1).size() > 1 && e == GC.chain(e1).front())
		{
			newEdge = new List<int>;
			for (edge e2 : GC.chain(e1)) {
				node v1 = e2->target();
				int n1 = first[H.rank(v1)] + levels.pos(v1); // Number nodes top down and from left to right
				newEdge->pushBack(n1);
			}
			newEdge->popBack(); // last node is nonvirtual and must be removed from list.

			// CG: avoid assigning a redirected edge to a dummy node twice
			if(newEdge->size() == 1 && longEdge[newEdge->front()] != nullptr) {
				delete newEdge;

			} else {
				// for every node in the the list, assign the longEdge (stored in
				// newEdge) to the node.
				for (int n1 : *newEdge) {
					longEdge[n1] = newEdge;
				}
			}
		}
	}

	// Compute adjacencylists adj[0] and adj[1] for every node.
	for(edge e1 : GC.edges) {
		node v1 = e1->source();
		node v2 = e1->target();
		int n1 = first[H.rank(v1)] + levels.pos(v1);
		int n2 = first[H.rank(v2)] + levels.pos(v2);
		adj[0][n2].pushBack(n1);
		adj[1][n1].pushBack(n2);
	}

	// Sort the adjacencylists adj[0] and adj[1] for every node according to
	// the internal numbering.
	forallnodes {
		adj[0][actNode].quicksort();
		adj[1][actNode].quicksort();
	}


	// Compute the layout
	findPlacement();

	// Copy coordinates into AGC
	for(node v1 : GC.nodes) {
		int n1 = first[H.rank(v1)] + levels.pos(v1);
		AGC.x(v1) = x[n1];
		if(GC.isDummy(v1) && !H.isLongEdgeDummy(v1))
			AGC.y(v1) = 0.5*(y[layer[n1]-1]+y[layer[n1]]);
		else
			AGC.y(v1) = y[layer[n1]];
	}

	// Cleanup

	List<int> *toDelete;
	forallnodes {
		if(longEdge[actNode] != nullptr) {
			toDelete = longEdge[actNode];
			for (int n1 : *toDelete) {
				longEdge[n1] = nullptr;
			}
			delete toDelete;
		}
	}
	delete[] y;
	delete[] first;
	delete[] height;
	delete[] x;
	delete[] breadth;
	delete[] layer;
	delete[] adj[0];
	delete[] adj[1];
	delete[] virt;
	delete[] longEdge;
}


void FastHierarchyLayout::sortLongEdges(int actNode,
	int dir,
	double *pos,
	bool& exD,
	double &dist,
	int *block,
	bool *marked)
{
	if(marked[actNode])
		// if node was already placed.
		return;

	bool exB=0;
	double best=0;

	// Mark the long edge. Thus all virtual nodes on the long edge will be
	// regarded as placed.
	for (int next : *longEdge[actNode]) {
		marked[next] = 1;
	}

	// Traverse the long Edge.
	// If for a virtual node there exists a left direct sibling w and w belongs
	// to the same block then call sortLongEdges recursively for w. Store
	// leftmost (rightmost) position in best.
	for (int next : *longEdge[actNode]) {
		if( sameLayer(next - dir, next) && block[next - dir] == block[next]) {
			sortLongEdges(next - dir, dir, pos, exD, dist, block, marked);
			if(!exB ||
				dir * (best - pos[next - dir]) <
				dir * (totalB[next] - totalB[next - dir]))
			{
				exB = 1;
				best = pos[next - dir] + totalB[next] - totalB[next - dir];
			}
		}
	}
	// Traverse long edge
	// Set postion of every virtual node on edge to best. Test for every node
	// if the direct left (right) sibling belongs to a different block.
	for (int next : *longEdge[actNode]) {
		pos[next]=best;
		if( sameLayer(next + dir,next) &&
			block[next + dir] != block[next] &&
			(!exD ||
			dir*(totalB[next + dir] - totalB[next] - pos[next + dir]
		+ pos[next]) > dist))
		{
			dist = dir*(totalB[next + dir] - totalB[next]
			- pos[next + dir] + pos[next]);
			exD = 1;
		}
	}
}


bool FastHierarchyLayout::placeSingleNode(int leftBnd,
	int rightBnd,
	int actNode,
	double& best,
	int d)
{
	kList neighbours;

	for (int next : adj[d][actNode]) {
		neighbours.add(0,x[next]);
	}
	if(neighbours.empty())
		return 0;
	best = neighbours.median();

	// if median outside boundaries, get free position as close as possible
	// to boundary.
	if(leftBnd != -1)
		incrTo(best,x[leftBnd] + mDist[actNode] - mDist[leftBnd]);
	if(rightBnd != -1)
		decrTo(best,x[rightBnd] + mDist[actNode] - mDist[rightBnd]);
	return 1;
}


void FastHierarchyLayout::placeNodes(int leftBnd,
	int rightBnd,
	int left,
	int right,
	int d)
{
	if(left == right)
		// The sequence consists of a single node.
		placeSingleNode(leftBnd,rightBnd,left,x[left],d);

	else if(left < right) {
		// Introduce variables to handle the two subsequences analogously.

		int mdl[2];
		mdl[0] = (right + left)/2; // rightmost node of left subsequence.
		mdl[1] = mdl[0] + 1;       // leftmost node of right subsequence.

		int bnd[2];         // technically for left and right boundary.
		bnd[0] = leftBnd;
		bnd[1] = rightBnd;

		int res[2];  // res[0] startresistance to push mdl[0] to the left
		// res[1] startresistance to push mdl[1] to the right

		int actNode; // auxiliary variables for nodes.
		int resChange; // auxiliary variables for the resistance

		int dir; // variable to distinguish left and right subsequence.
		// -1 for the left, 1 for the right subsequence.

		double mD; // minimal distance between mdl[0] and mdl[1]
		mD = mDist[mdl[1]] - mDist[mdl[0]];


		kList bends[2]; // bends[0] stores the changes of resistance against
		// pushing mdl[0] to the left
		// bends[1] stores the changes of resistance against
		// pushing mdl[1] to the right
		// Change of resistance is of type withKey: containg
		// the position in a double variable where the
		// resitance changes and an integer storing the amount
		// of incresing the resistance.

		kList bds;      // auxiliary variable for constructing bends.
		double newBend; // a position of a resistance change.

		double diff,diff1,diff2; // auxiliary variables


		//recursive call for the left and the right subsequence
		placeNodes(leftBnd,rightBnd,left,mdl[0],d);
		placeNodes(leftBnd,rightBnd,mdl[1],right,d);


		// Scan the left (i =0) and then the right subsequence (i = 1) to
		// compute the bends[i]; for technical details see report.
		for(int i = 0;i < 2;i++) {
			dir = i ? 1: -1; // set direction
			res[i]=0;
			for(actNode = mdl[i];actNode >= left && actNode <= right;actNode +=dir) {
				resChange = 0;
				for (int next : adj[d][actNode]) {
					if(dir*(x[next] - x[actNode]) < ALLOW)
						resChange++;
					else {
						resChange--;
						newBend = x[next] + mDist[mdl[i]] - mDist[actNode];
						if(dir*(x[mdl[i]] - newBend) > -ALLOW) {
							res[i]++;
						} else if((bnd[i] == -1 || dir*(newBend - x[bnd[i]]
						+ mDist[bnd[i]] - mDist[mdl[i]]) <ALLOW)
							&& dir*(newBend-x[mdl[1-i]])<mD-ALLOW)
						{
							bds.add(2,newBend);
						}
					}
				}
				newBend = x[actNode] + mDist[mdl[i]] - mDist[actNode];
				if(dir*(x[mdl[i]] - newBend) >- ALLOW) {
					res[i] += resChange;
				} else if((bnd[i] == -1 ||
					dir *
					(newBend - x[bnd[i]] + mDist[bnd[i]] - mDist[mdl[i]])
					< ALLOW) && dir*(newBend - x[mdl[1 - i]]) < mD - ALLOW)
				{
					bds.add(resChange,newBend);
				}
			}
			if(bnd[i] != -1)
				bds.add(m,x[bnd[i]] - mDist[bnd[i]] + mDist[mdl[i]]);
			cmpWithKey cmp;
			bds.quicksort(cmp);
			bds.reduce(bends[i]);
		}
		if(!bends[0].empty())
			bends[0].reverse();



		// Move mdl[0] and mdl[1] such that the  nodes respect the minimal node
		// distance mD.
		while(x[mdl[1]] - x[mdl[0]] < mD-ALLOW) {
			// as long as distance too small

			int resCh0, resCh1;
			resCh0=resCh1=0;
			if(res[0] < res[1]) { // if smaller resistance to the left.
				// Go to the next resistance change and update resCh0 and x[mdl[0]].
				if(!bends[0].pop(resCh0,x[mdl[0]]) || x[mdl[1]] - x[mdl[0]] > mD + ALLOW)
					x[mdl[0]] = x[mdl[1]] - mD;
			}
			else if(res[1] < res[0]) { // if smaller resistance to the right.
				// Go to the next resistance change and update resCh1 and x[mdl[1]].
				if(!bends[1].pop(resCh1,x[mdl[1]]) || x[mdl[1]] - x[mdl[0]] > mD + ALLOW)
					x[mdl[1]] = x[mdl[0]] + mD;
			}
			else { // same resistance to the left and to the right.
				// Update resCh0 resCh1 and x[mdl[0]] x[mdl[1]] by simultaneously
				// moving mdl[0] and mdl[1].
				diff = (mD - x[mdl[1]] + x[mdl[0]])/2;
				diff1 = bends[0].empty() ? diff + 1 : x[mdl[0]] - bends[0].peek();
				diff2 = bends[1].empty() ? diff + 1 : bends[1].peek()-x[mdl[1]];
				if(diff1 < diff + ALLOW && diff1 < diff2 + ALLOW)
					bends[0].pop(resCh0,newBend);
				if(diff2 < diff + ALLOW && diff2 < diff1 + ALLOW)
					bends[1].pop(resCh1,newBend);
				decrTo(diff,diff1);
				decrTo(diff,diff2);
				x[mdl[0]] -= diff;
				x[mdl[1]] += diff;
			}
			res[0] += resCh0; // Update the resistance by the change of resistance
			res[1] += resCh1; // Update the resistance by the change of resistance
		}


		// mdl[0] and mdl[1] respect minimal node distance mD. Place
		// subsequences accordingly.
		actNode = mdl[0];
		while(--actNode >= left &&
			x[mdl[0]] - x[actNode] < mDist[mdl[0]] - mDist[actNode])
			x[actNode] = x[mdl[0]] - mDist[mdl[0]] + mDist[actNode];
		actNode = mdl[1];
		while(++actNode <= right &&
			x[mdl[1]] - x[actNode] > mDist[mdl[1]] - mDist[actNode])
			x[actNode] = x[mdl[1]] - mDist[mdl[1]] + mDist[actNode];
	}
}


void FastHierarchyLayout::moveLongEdge(int actNode,
	int dir,
	bool *marked)
{
	if (!marked[actNode] && virt[actNode]) {
		// if actNode belongs to a long edge and has not been moved yet.
		// mark all virtual nodes of the long edge
		for (int next : *longEdge[actNode]) {
			marked[next] = 1;
		}

		// first non virtual node of long edge
		int fst = adj[0][longEdge[actNode]->front()].front();

		// second non virtual node of long edge
		int lst = adj[1][longEdge[actNode]->back()].front();

		// Contains an order of the two positions of the nonvirtual nodes of
		// the long edge. The function moveLongEdge first tries to place the
		// long edge onto the first position If not successful, moveLongEdge
		// tries to place the long edge onto the second position
		List<double> toTest;

		if (dir < 0) {
			toTest.pushBack(x[fst] < x[lst] ? x[fst] : x[lst]);
			toTest.pushBack(x[fst] < x[lst] ? x[lst] : x[fst]);
		}
		else if (dir > 0) {
			toTest.pushBack(x[fst] < x[lst] ? x[lst] : x[fst]);
			toTest.pushBack(x[fst] < x[lst] ? x[fst] : x[lst]);
		}
		else {
			toTest.pushBack(x[fst]);
			toTest.pushBack(x[lst]);
		}
		double xFirst; // stores the first and most preferred position
		xFirst = toTest.front();

		double xOpt;  // stores the preferred position

		bool done = false;
		while (!done && !toTest.empty()) {
			xOpt = toTest.front(); // Best position that can be reached by moving
			// the long edge
			toTest.popFront();
			done = 1;
			for (int next : *longEdge[actNode]) {
				// for all virtual nodes on the long edge

				// Try moving to the left
				if (!isFirst(next)) {
					// next does have a left sibling
					if (xOpt - x[next - 1] < totalB[next] - totalB[next - 1])
						// there is a conflict moving next to the position xOpt.
						moveLongEdge(next - 1, -1, marked);

					// done = 0 if minimal distances cannot be repsected.
					done = done &&
						xOpt - x[next - 1] >=
						totalB[next] - totalB[next - 1] - ALLOW;
				}

				// Try moving to the right
				if (!isLast(next)) {
					// next does have a right sibling
					if (xOpt - x[next + 1] > totalB[next] - totalB[next + 1])
						// there is a conflict moving next to the position xOpt.
						moveLongEdge(next + 1, 1, marked);

					// done = 0 if minimal distances cannot be respected.
					done = done &&
						xOpt - x[next + 1] <=
						totalB[next] - totalB[next + 1] + ALLOW;
				}
			}
		}
		if (!done) {
			// moveLongEdge was not able to move the virtual nodes to one of
			// the  two positions of the nonvirtual nodes. It now tries to
			// approximate the most preferred position.
			xOpt = xFirst;
			for (int next : *longEdge[actNode]) {
				if (!isFirst(next))
					incrTo(xOpt, x[next - 1] + totalB[next] - totalB[next - 1]);
				if (!isLast(next))
					decrTo(xOpt, x[next + 1] + totalB[next] - totalB[next + 1]);
			}
		}
		for (int next : *longEdge[actNode]) {
			x[next] = xOpt;
		}
	}
}


void FastHierarchyLayout::straightenEdge(int actNode,bool *marked)
{
	if(!marked[actNode] && // breadth[actNode] < ALLOW &&
		adj[0][actNode].size() == 1 &&
		adj[1][actNode].size() == 1 &&
		longEdge[actNode]->size() < 2)
	{
		marked[actNode]=1;

		int fst=adj[0][actNode].front();
		int lst=adj[1][actNode].front();

		// Get optimal position for actNode
		double xOpt=x[fst] + (x[lst] - x[fst]) *
			(y[layer[actNode]] - y[layer[fst]]) /
			(y[layer[lst]] - y[layer[fst]]);


		if(!isFirst(actNode)) {
			// actNode does have a left sibling
			if(xOpt - x[actNode-1] < totalB[actNode] - totalB[actNode-1] - ALLOW)
				// there is a conflict with the position of the direct left sibling
				// Recursively call straightenEdge.
				straightenEdge(actNode - 1,marked);
			if(xOpt - x[actNode - 1]<totalB[actNode] - totalB[actNode-1]-ALLOW)
				return;
		}
		if(!isLast(actNode)) {
			// actNode does have a right sibling
			if(x[actNode+1] - xOpt < totalB[actNode+1] - totalB[actNode] - ALLOW)
				// there is a conflict with the position of the direct left sibling
				// Recursively call straightenEdge.
				straightenEdge(actNode + 1,marked);
			if(x[actNode+1] - xOpt < totalB[actNode+1] - totalB[actNode] - ALLOW)
				return;
		}
		x[actNode] = xOpt;
	}
}


void FastHierarchyLayout::findPlacement()
{
	int actNode,actLayer,dir,leftBnd,leftNxt,rightNxt;
	bool *marked=new bool[n];
	//ListConstIterator<int> _it = 0;

	// Replace all virtual nodes in an edge traversing only one layer by a
	// nonvirtual node.
	forallnodes if(virt[actNode] &&
		!virt[adj[0][actNode].front()] &&
		!virt[adj[1][actNode].front()])
		virt[actNode] = 0;

	// Compute minimal distances between nodes (totalB)

	totalB = new double[n];
	double toAdd;
	forallnodes {
		if (!actNode ||
			layer[actNode-1] < layer[actNode])
			totalB[actNode]=0;
		else {
			toAdd = breadth[actNode-1] / 2 + breadth[actNode] / 2;
			// Enlarge the minimal Distance for nodes with a lot of neighbours.
			incrTo(toAdd,m_minNodeDist / 3 * (double)(adj[0][actNode-1].size() + adj[0][actNode].size()));
			incrTo(toAdd,m_minNodeDist / 3 * (double)(adj[1][actNode-1].size() + adj[1][actNode].size()));
			// distances are computed such that they are placed on a grid based on minNodeDist.
			toAdd = m_minNodeDist *(int)(toAdd/m_minNodeDist + 1 - ALLOW);
			toAdd += m_minNodeDist;
			totalB[actNode] = totalB[actNode-1] + toAdd;
		}
	}

	// Remove crossings of long edges
	// Applied for long edges that cross each other in inner segments.

	List<int> *newEdge,*oldEdge;
	int down,spl;


	// For every two consecutive layers l and l+1 , traverse the inner edge
	// segments between the two layers according to the order of virtual nodes
	// on layer l. We consider consecutive pairs of inner edges segments
	// from left to right.  If two inner edge segments cross, we split the
	// right long edge between the two layers by parting the corresponding list
	// into two sublists.
	foralllayers {
		int last=-1;
		forallnodesonlayer {
			if(virt[actNode]) {
				down=adj[1][actNode].front();
				if(virt[down]) {
					if(last!=-1&&last>down) {
						oldEdge = longEdge[actNode];
						spl = actLayer - layer[oldEdge->front()] + 1;
						newEdge = new List<int>;
						oldEdge->split(oldEdge->get(spl),(*newEdge),
							(*oldEdge));
						for (int next : *newEdge) {
							longEdge[next] = newEdge;
						}
					} else
						last = down;
				}
			}
		}
	}

	// Place long edges

	bool exD;
	int *block = new int[n];
	double *pos = new double[n];
	kList neighbours;
	double dist;

	forallnodes
		x[actNode] = 0;
	for(dir = 1; dir >= -1;dir -= 2) {
		// for dir = 1, move long edges as far as possible to the left
		// for dir = -1, move long edges as far as possible to the right


		// Partition the graph into blocks according to the technical report.
		int blockCount = 0;
		forallnodes
			block[actNode] = marked[actNode] = 0;
		foralllayers {
			actNode = (dir == 1 ? first[actLayer] : first[actLayer+1] - 1);
			if(!block[actNode]) {
				for (int next : *longEdge[actNode]){
					block[next] = blockCount;
				}
				blockCount++;
			}
			actNode += dir;
			while(actNode >= first[actLayer] && actNode < first[actLayer+1]) {
				if(!block[actNode]) {
					for (int next : *longEdge[actNode]){
						block[next] = block[actNode-dir];
					}
				}
				actNode += dir;
			}
		}

		// Store the nodes of every block in a separate list
		ArrayBuffer<int> *blockNodes = new ArrayBuffer<int>[n];
		foralllayers {
			for(actNode = ( (dir == 1) ? first[actLayer] : (first[actLayer+1]-1) );
				actNode >= first[actLayer] && actNode < first[actLayer+1];
				actNode += dir)
			{
				blockNodes[block[actNode]].push(actNode);
			}
		}

		for(int i = 0; i < blockCount;i++) {
			// for every block
			exD = 0;
			dist = 0;
			for (int actNodeInBlock : blockNodes[i]) {
				// for every node of the block apply sortLongEdges
				sortLongEdges(actNodeInBlock,dir,pos,exD,dist,block,marked);
			}
			if(!exD) {
				// The currently examined block does not share its layers with
				// another block that has been placed before. Thus the block
				// can be freely moved. We compute a position for the block by
				// minimizing the total edge length to neighbours of blocks
				// that have already been placed. dist has not been computed by
				// sortLongEdges and is now set to the optimal value.
				actLayer = layer[blockNodes[i][0]];
				forallnodesonlayer {
					for (int next : adj[0][actNode]) {
						neighbours.add(0,pos[next] - pos[actNode]);
					}
				}
				if(!neighbours.empty()) {
					cmpWithKey cmp;
					neighbours.quicksort(cmp);
					dist =- dir*neighbours.median();
					neighbours.clear();
				}
			}
			// If exD is true, dist has been computed by sortLongEdges.

			// Move the nodes of the block to their positions.
			for (int actNodeInBlock : blockNodes[i]) {
				pos[actNodeInBlock] -= dir * dist;
			}
		}

		// Compute for every node the average of the two positions of the left
		// and right placement.
		forallnodes
			x[actNode] += pos[actNode] / 2;

		delete[] blockNodes;
	}

	delete[] block;
	delete[] pos;

	// Place nonvirtual nodes

	// Stores for every internal sequence of in
	// wihch traversal the sequence has to be placed.
	// An internal sequence is represeted by its left virtual sibling. Values:
	// -1 : is not yet clear
	// 0  : top down traversal
	// 1  : bottom up traversal
	int *nodeDir = new int[n];

	forallnodes
		nodeDir[actNode] = -1;



	// Initialization of marked.
	// marked is used to indicate the placement of a sequence.
	forallnodes {
		if(virt[actNode]) {
			int next = actNode + 1;
			while(sameLayer(next,actNode) && !virt[next])
				next++;

			// If actNode is representative of a fixed inner sequence, the
			// sequence is already placed and marked is set
			marked[actNode] = sameLayer(next,actNode) &&
				x[next] - x[actNode] < totalB[next] - totalB[actNode] + ALLOW;

		} else
			marked[actNode] = 0;
	}

	forallnodes {
		if(marked[actNode] && nodeDir[actNode] == -1) {
			// for every fixed sequence, traverse the long edge belonging to
			// the representative actnode
			for (int next : *longEdge[actNode]) {
				// for every virtual node next of the long edge mark the
				// corrsponding sequence as to be placed in the upward
				// traversal if next is above of actnode and in the downward
				// traversal if next is below actNode.
				if(next != actNode)
					nodeDir[next] = next < actNode;
			}
		}
	}

	// mDist is equal to totalB during the first traversal. After the first
	// traversal it is set to the positions computed in the first traversal.
	// It is only modified for external sequences.
	mDist = new double[n];
	// Die folgende forallnodes Schleife wurde f?r den Vorschlag von
	// Christoph (siehe unten) entfernt, da ?berfl?ssig
	forallnodes
		mDist[actNode] = totalB[actNode];

	for(dir = 0;dir < 2;dir++) {
		// for every traversal

		for(actLayer = dir ? k-1 : 0; 0 <= actLayer && actLayer < k;
			actLayer += dir ? -1 : 1)
		{
			// for every layer (if dir = 0 top down)

#if 0
			// Änderungen vorgeschlagen von Christoph
			// führen aber dazu, dass sich Knoten zu Nahe kommen
			// können (näher als minDist) (Carsten)
			for(int i1 = first[actLayer]; i1 < first[actLayer+1]; i1++)
				mDist[i1] = x[i1];

			leftBnd=-1;
			forallnodesonlayer {
				if(virt[actNode] && virt[adj[dir][actNode].front()]) {
					x[actNode] = x[adj[dir][actNode].front()];

					if(leftBnd == -1)
						placeNodes(-1,actNode,first[actLayer],actNode-1,dir);
					else
						placeNodes(leftBnd,actNode,leftBnd+1,actNode-1,dir);

					leftBnd = actNode;
				}
			}

			if(leftBnd == -1)
				placeNodes(-1,-1,first[actLayer],first[actLayer+1]-1,dir);
			else
				placeNodes(leftBnd,-1,leftBnd+1,first[actLayer+1]-1,dir);

			for(int i1 = first[actLayer]; i1 < first[actLayer+1]; i1++)
				mDist[i1] = totalB[i1];
#endif

			leftBnd=-1;
			forallnodesonlayer {
				if(virt[actNode]) {
					// For every sequence, bounded by leftBnd and actNode.
					if(leftBnd == -1) {
						// leftBnd is not a node. The sequence is external. Place it.
						placeNodes(-1,actNode,first[actLayer],actNode - 1,dir);
						for(int next = first[actLayer]; next < actNode; next++)
							mDist[next] = mDist[actNode] - x[actNode] + x[next];
					}
					else if(nodeDir[leftBnd] != !dir) { // nodeDir[leftBnd] == dir ||  nodeDir[leftBnd] == -1
						// internal sequence

						if(!marked[leftBnd])
							// sequence has not been placed yet. Place it.
							placeNodes(leftBnd,actNode,leftBnd + 1,
							actNode - 1,dir);

						// Adjust nodeDir for the next layer.
						leftNxt = adj[!dir][leftBnd].front();
						rightNxt = adj[!dir][actNode].front();
						if(virt[leftNxt] && virt[rightNxt])
							for(int next = leftNxt + 1;next < rightNxt; next++)
								nodeDir[next] = dir;
					}
					leftBnd = actNode;
				}
			}
			if(leftBnd == -1) {
				// No virtual node in the complete layer. Place it.
				placeNodes(-1,-1,first[actLayer],first[actLayer+1] - 1,dir);
				for (int next = first[actLayer]; next < first[actLayer+1]; next++)
					mDist[next] = x[next];
			}
			else {
				// External sequence to the right. Place it.
				placeNodes(leftBnd,-1,leftBnd + 1,first[actLayer+1] - 1,dir);
				for (int next = first[actLayer+1] - 1;next > leftBnd; next--)
					mDist[next] = mDist[leftBnd] - x[leftBnd] + x[next];
			}
		}
	}
	delete[] nodeDir;
	delete[] mDist;

	// Apply moveLongEdge to every long edge.
	forallnodes
		marked[actNode] = 0;
	foralllayers {
		for (actNode = (first[actLayer] + first[actLayer + 1]) / 2;
			actNode < first[actLayer+1]; actNode++)
		{
			moveLongEdge(actNode,0,marked);
		}
		for (actNode = (first[actLayer] + first[actLayer+1]) / 2 - 1;
			actNode >= first[actLayer]; actNode--)
		{
			moveLongEdge(actNode,0,marked);
		}
	}

	// Compute ordinates for the layers and boxY

	double boxY = k ? height[0] / 2 : 0; // y-value for the bounding box.

	foralllayers {
		y[actLayer] = boxY;
		double minD = m_minLayerDist;
		if(!m_fixedLayerDist) {
			forallnodesonlayer {
				// adjust the distance of the layer to the longest edge
				for (int next : adj[1][actNode]) {
					incrTo(minD,(x[next] - x[actNode]) / 3);
					incrTo(minD,(x[actNode] - x[next]) / 3);
				}
			}
			decrTo(minD,10 * m_minLayerDist);
		}
		boxY += height[actLayer] / 2;
		if(actLayer < k - 1)
			boxY += minD + height[actLayer+1] / 2;
	}

	// Apply straightenEdge to every long edge with one virtual node.

	forallnodes
		marked[actNode] = 0;
	foralllayers {
		for (actNode = (first[actLayer] + first[actLayer+1]) / 2;
			actNode < first[actLayer+1]; actNode++)
		{
			straightenEdge(actNode,marked);
		}
		for (actNode = (first[actLayer] + first[actLayer+1]) / 2 - 1;
			actNode >= first[actLayer]; actNode--)
		{
			straightenEdge(actNode,marked);
		}
	}
	delete[] marked;
	delete[] totalB;
}

}
