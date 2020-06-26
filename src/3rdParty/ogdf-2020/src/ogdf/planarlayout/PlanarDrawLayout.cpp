/** \file
 * \brief Implements class PlanarDrawLayout
 *
 * \author Carsten Gutwenger
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


#include <ogdf/planarlayout/PlanarDrawLayout.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/augmentation/PlanarAugmentation.h>
#include <ogdf/augmentation/PlanarAugmentationFix.h>
#include <ogdf/planarlayout/BiconnectedShellingOrder.h>
#include <ogdf/planarity/SimpleEmbedder.h>

namespace ogdf {


PlanarDrawLayout::PlanarDrawLayout()
{
	m_sizeOptimization = true;
	m_sideOptimization = false;
	m_baseRatio        = 0.33;

	m_augmenter.reset(new PlanarAugmentation);
	m_computeOrder.reset(new BiconnectedShellingOrder);
	m_embedder.reset(new SimpleEmbedder);
}


void PlanarDrawLayout::doCall(
	const Graph &G,
	adjEntry adjExternal,
	GridLayout &gridLayout,
	IPoint &boundingBox,
	bool fixEmbedding)
{
	// require to have a planar graph without multi-edges and self-loops;
	// planarity is checked below
	OGDF_ASSERT(isSimple(G));

	if (G.numberOfNodes() < 2) {
		return;
	}

	// we make a copy of G since we use planar biconnected augmentation
	GraphCopySimple GC(G);

	if(fixEmbedding) {
		PlanarAugmentationFix augmenter;
		augmenter.call(GC);

	} else {
		// augment graph planar biconnected
		m_augmenter->call(GC);

		// embed augmented graph
		m_embedder->call(GC,adjExternal);
	}

	// compute shelling order
	m_computeOrder->baseRatio(m_baseRatio);

	ShellingOrder order;
	m_computeOrder->call(GC,order,adjExternal);

	// compute grid coordinates for GC
	NodeArray<int> x(GC), y(GC);
	computeCoordinates(GC,order,x,y);

	boundingBox.m_x = x[order(1,order.len(1))];
	boundingBox.m_y = 0;
	for(node v : GC.nodes)
		if(y[v] > boundingBox.m_y) boundingBox.m_y = y[v];

	// copy coordinates from GC to G
	for(node v : G.nodes) {
		node vCopy = GC.copy(v);
		gridLayout.x(v) = x[vCopy];
		gridLayout.y(v) = y[vCopy];
	}
}

void PlanarDrawLayout::computeCoordinates(const Graph &G,
	ShellingOrder &order,
	NodeArray<int> &x,
	NodeArray<int> &y)
{
	// let c_1,...,c_q be the the current contour, then
	// next[c_i] = c_i+1, prev[c_i] = c_i-1
	NodeArray<node>	next(G), prev(G);

	// upper[v] = w means x-coord. of v is relative to w
	// (abs. x-coord. of v = x[v] + abs. x-coord of w)
	NodeArray<node>	upper(G,nullptr);

	// maximal rank of a neighbour
	NodeArray<int> maxNeighbour(G,0);
	// internal nodes (nodes not on contour)
	ArrayBuffer<node> internals(G.numberOfNodes());

	for(node v : G.nodes)
	{
		for(adjEntry adj : v->adjEntries) {
			int r = order.rank(adj->twinNode());
			if (r > maxNeighbour[v])
				maxNeighbour[v] = r;
		}
	}

	// initialize contour with base
	const ShellingOrderSet &V1 = order[1];
	node v1 = V1[1];
	node v2 = V1[V1.len()];
	node rightSide = v2;

	int i;
	for (i = 1; i <= V1.len(); ++i)
	{
		y[V1[i]] = 0;
		x[V1[i]] = (i == 1) ? 0 : 1;
		if (i < V1.len())
			next[V1[i]] = V1[i+1];
		if (i > 1)
			prev[V1[i]] = V1[i-1];
	}
	prev[v1] = next[v2] = nullptr;

	// process shelling order from bottom to top
	for (int k = 2; k <= order.length(); k++)
	{
		// Referenz auf aktuelle Menge Vk (als Abk?rzung)
		const ShellingOrderSet &Vk = order[k]; // Vk = { z_1,...,z_l }
		int len = Vk.len();

		node z1 = Vk[1];
		node cl = Vk.left();  // left vertex
		node cr = Vk.right(); // right vertex

		bool isOuter;
		if (m_sideOptimization && cr == rightSide && maxNeighbour[cr] <= k)
		{
			isOuter   = true;
			rightSide = Vk[len];
		} else
			isOuter = false;

		// compute relative x-distance from c_i to cl for i = len+1, ..., r
		int sum = 0;
		for (node v = next[cl]; v != cr; v = next[v]) {
			sum += x[v];
			x[v] = sum;
		}
		x[cr] += sum;

		int eps = (maxNeighbour [cl] <= k && k > 2) ? 0 : 1;

		int x_cr, y_z;
		if (m_sizeOptimization)
		{
			int yMax;
			if (isOuter)
			{
				yMax = max(y[cl]+1-eps,
					y[cr] + ((x[cr] == 1 && eps == 1) ? 1 : 0));
				for (node v = next[cl]; v != cr; v = next[v]) {
					if (x[v] < x[cr]) {
						int y1 = (y[cr]-y[v])*(eps-x[cr])/(x[cr]-x[v])+y[cr];
						if (y1 >= yMax)
							yMax = 1+y1;
					}
				}
				for (node v = cr; v != cl; v = prev[v]) {
					if (y[prev[v]] > y[v] && maxNeighbour[v] >= k) {
						if (yMax <= y[v] + x[v] - eps) {
							eps  = 1;
							yMax = y[v] + x[v];
						}
						break;
					}
				}
				x_cr = max(x[cr]-eps-len+1, (y[cr] == yMax) ? 1 : 0);
				y_z  = yMax;

			} else {
				// yMax = max { y[c_i] | len <= i <= r }
				yMax = y[cl] - eps;
				for (node v = cr; v != cl; v = prev[v]) {
					if (y[v] > yMax)
						yMax = y[v];
				}
				int offset = max (yMax-x[cr]+len+eps-y[cr],
					(y[prev[cr]] > y[cr]) ? 1 : 0);
				y_z  = y[cr] + x[cr] + offset - len + 1 - eps;
				x_cr = y_z - y[cr];
			}

		} else {
			y_z  = y[cr] + x[cr] + 1 - eps;
			x_cr = y_z - y[cr];
		}

		node alpha = cl;
		for (node v = next[cl];
			maxNeighbour[v] <= k-1 && order.rank(v) <= order.rank(prev[v]);
			v = next[v])
		{
			if (order.rank (v) < order.rank (alpha))
				alpha = v;
			if (v == cr)
				break;
		}

		node beta = prev[cr];
		for (node v = prev[cr];
			maxNeighbour[v] <= k-1 && order.rank(v) <= order.rank(next[v]);
			v = prev[v])
		{
			if (order.rank (v) <= order.rank (beta))
				beta = v;
			if (v == cl)
				break;
		}

		for (i = 1; i <= len; ++i) {
			x[Vk[i]] = 1;
			y[Vk[i]] = y_z;
		}
		x[z1] = eps;

		for (node v = alpha; v != cl; v = prev [v]) {
			upper[v] = cl;
			internals.push (v);
		}
		for (node v = next [beta]; v != cr; v = next [v]) {
			upper[v]  = cr;
			x [v]    -= x[cr];
			internals.push (v);
		}
		for (node v = beta; v != alpha; v = prev[v]) {
			upper[v]  = z1;
			x [v]    -= x[z1];
			internals.push (v);
		}

		x[cr] = x_cr;

		// update contour after insertion of z_1,...,z_l
		for (i = 1; i <= len; i++) {
			if (i < len)
				next[Vk[i]] = Vk[i+1];
			if (i > 1)
				prev[Vk[i]] = Vk[i-1];
		}
		next [cl] = z1;
		next [Vk[len]] = cr;
		prev [cr] = Vk[len];
		prev [z1] = cl;
	}

	// compute final x-coordinates for the nodes on the (final) contour
	int sum = 0;
	for (node v = v1; v != nullptr; v = next[v])
		x [v] = (sum += x[v]);

	// compute final x-coordinates for the internal nodes
	while (!internals.empty()) {
		node v = internals.popRet();
		x[v] += x[upper[v]];
	}
}

}
