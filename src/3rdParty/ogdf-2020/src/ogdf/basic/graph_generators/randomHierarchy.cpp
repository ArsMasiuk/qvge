/** \file
 * \brief Implements graph generator for hierarchical graphs.
 *
 * \author Carsten Gutwenger, Christoph Buchheim
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


#include <ogdf/basic/SList.h>
#include <ogdf/basic/graph_generators/deterministic.h>
#include <ogdf/basic/graph_generators/randomHierarchy.h>

using std::minstd_rand;
using std::uniform_int_distribution;
using std::uniform_real_distribution;


namespace ogdf {


class BEdge {
public:
	int head, tail, id, pos;
	BEdge *next;
	BEdge(int t,int h,int c) : head(h), tail(t), id(c), pos(-1), next(nullptr) { }
	OGDF_NEW_DELETE
};

using bEdge = BEdge*;


OGDF_DECLARE_COMPARER(CmpTail, bEdge, int, x->tail);
OGDF_DECLARE_COMPARER(CmpHead, bEdge, int, x->head);


void randomHierarchy(
	Graph &G,
	int numberOfNodes,
	int numberOfEdges,
	bool planar,
	bool singleSource,
	bool longEdges)
{
	Array<node> nnr (3*numberOfNodes);
	Array<int>  vrt (3*numberOfNodes);
	Array<int>  fst (numberOfNodes+1);

	/** Place nodes **/

	emptyGraph(G, numberOfNodes);

	minstd_rand rng(randomSeed());
	uniform_real_distribution<> dist_0_1(0.0,1.0);

	int numberOfLayers = 0, totNumber = 0, realCount = 0;
	fst[0] = 0;
	for(node v : G.nodes) {
		if(longEdges && numberOfLayers) vrt[totNumber++] = 1;

		nnr[totNumber] = v;
		vrt[totNumber++] = 0;
		realCount++;
		double r = dist_0_1(rng);
		if((totNumber == 1 && singleSource) || realCount == numberOfNodes || r*r*numberOfNodes < 1)
		{
			if(longEdges && numberOfLayers)
				vrt[totNumber++] = 1;
			fst[++numberOfLayers] = totNumber;
		}
	}

	/** Determine allowed neighbours **/

	Array<int> leftN (totNumber);
	Array<int> rightN(totNumber);
	for(int layer = 1; layer < numberOfLayers; layer++)
	{
		if(planar) {
			int n1 = fst[layer-1];
			int n2 = fst[layer];
			leftN[n2] = n1;
			while(n1 < fst[layer] && n2 < fst[layer+1]) {
				double r = dist_0_1(rng);
				if(n1 != fst[layer]-1 &&
					(n2 == fst[layer+1]-1 ||
					r < (double)(fst[layer]-fst[layer-1])/(double)(fst[layer+1]-fst[layer-1])))
					n1++;
				else {
					rightN[n2] = n1;
					if(++n2 < fst[layer+1])
						leftN[n2] = n1;
				}
			}
		}
		else
			for(int n2 = fst[layer]; n2 < fst[layer+1]; n2++) {
				leftN [n2] = fst[layer-1];
				rightN[n2] = fst[layer]-1;
			}
	}

	/** Insert edges **/

	List<bEdge> startEdges;
	Array<SList<bEdge>> edgeIn (totNumber);
	Array<SList<bEdge>> edgeOut(totNumber);

	if (numberOfLayers) {
		double x1 = numberOfEdges;
		double x2 = 0;
		for (int n2 = fst[1]; n2 < totNumber; n2++) {
			if (!vrt[n2])
				x2 += rightN[n2] - leftN[n2] + 1;
		}

		int idc = 0;
		for (int n2 = fst[1]; n2 < totNumber; n2++) {
			if (!vrt[n2]) {
				bool connected = !singleSource;
				for (int n1 = leftN[n2]; n1 <= rightN[n2] || !connected; n1++) {
					double r = dist_0_1(rng);
					if (r < x1 / x2 || n1 > rightN[n2]) {
						int next = (n1 <= rightN[n2] ? n1 : uniform_int_distribution<>(leftN[n2], rightN[n2])(rng));
						int act = n2;
						bEdge nextEdge = new BEdge(next, act, idc++);
						while (vrt[next]) {
							act = next;
							next = uniform_int_distribution<>(leftN[act], rightN[act])(rng);
							edgeOut[act].pushBack(nextEdge);
							nextEdge = new BEdge(next, act, idc++);
							edgeIn[act].pushBack(nextEdge);
						}
						startEdges.pushBack(nextEdge);
						connected = true;
						x1 -= 1;
					}
					if (n1 <= rightN[n2])
						x2 -= 1;
				}
			}
		}
	}

	if(planar)
		for(int act = 0; act < totNumber; act++) {
			CmpTail cmpTail;
			edgeIn[act].quicksort(cmpTail);
			CmpHead cmpHead;
			edgeOut[act].quicksort(cmpHead);
		}

	for(int act = 0; act < totNumber; act++) {
		for(bEdge nextEdge : edgeIn[act]) {
			nextEdge->next = edgeOut[act].popFrontRet();
		}
	}

	for(bEdge actEdge : startEdges) {
		bEdge nextEdge = actEdge;
		while(vrt[nextEdge->head])
			nextEdge = nextEdge->next;
		G.newEdge(nnr[actEdge->tail], nnr[nextEdge->head]);
	}

	/** Clean up **/
	for(bEdge nextEdge : startEdges) {
		bEdge toDelete = nextEdge;
		while(vrt[nextEdge->head]) {
			nextEdge = nextEdge->next;
			delete toDelete;
			toDelete = nextEdge;
		}
		delete toDelete;
	}
}


}
