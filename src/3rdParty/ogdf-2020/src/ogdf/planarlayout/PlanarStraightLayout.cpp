/** \file
 * \brief Implements class PlanarStraightLayout
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


#include <ogdf/planarlayout/PlanarStraightLayout.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/augmentation/PlanarAugmentation.h>
#include <ogdf/augmentation/PlanarAugmentationFix.h>
#include <ogdf/planarlayout/BiconnectedShellingOrder.h>
#include <ogdf/planarity/SimpleEmbedder.h>

namespace ogdf {


PlanarStraightLayout::PlanarStraightLayout()
{
	m_sizeOptimization = true;
	m_baseRatio        = 0.33;

	m_augmenter.reset(new PlanarAugmentation);
	m_computeOrder.reset(new BiconnectedShellingOrder);
	m_embedder.reset(new SimpleEmbedder);
}


void PlanarStraightLayout::doCall(
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
		// determine adjacency entry on external face of GC (if required)
		if(adjExternal != nullptr) {
			edge eG  = adjExternal->theEdge();
			edge eGC = GC.copy(eG);
			adjExternal = (adjExternal == eG->adjSource()) ? eGC->adjSource() : eGC->adjTarget();
		}

		PlanarAugmentationFix augmenter;
		augmenter.call(GC);

	} else {
		adjExternal = nullptr;

		// augment graph planar biconnected
		m_augmenter->call(GC);

		// embed augmented graph
		m_embedder->call(GC,adjExternal);
	}

	// compute shelling order with shelling order module
	m_computeOrder->baseRatio(m_baseRatio);

	ShellingOrder order;
	m_computeOrder->callLeftmost(GC,order,adjExternal);

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


void PlanarStraightLayout::computeCoordinates(const Graph &G,
	ShellingOrder &lmc,
	NodeArray<int> &x,
	NodeArray<int> &y)
{
	// let c_1,...,c_q be the the current contour, then
	// next[c_i] = c_i+1, prev[c_i] = c_i-1
	NodeArray<node>	next(G), prev(G);

	// upper[v] = w means x-coord. of v is relative to w
	// (abs. x-coord. of v = x[v] + abs. x-coord of w)
	NodeArray<node>	upper(G,nullptr);

	// initialize contour with base
	const ShellingOrderSet &V1 = lmc[1];
	node v1 = V1[1];
	node v2 = V1[V1.len()];

	int i;
	for (i = 1; i <= V1.len(); ++i)
	{
		y [V1[i]] = 0;
		x [V1[i]] = (i == 1) ? 0 : 2;
		if (i < V1.len())
			next[V1[i]] = V1[i+1];
		if (i > 1)
			prev[V1[i]] = V1[i-1];
	}
	prev[v1] = next[v2] = nullptr;

	// process shelling order from bottom to top
	const int n = lmc.length();
	int k;
	for (k = 2; k <= n; ++k)
	{
		const ShellingOrderSet &Vk = lmc[k]; // Vk = { z_1,...,z_l }
		int len = Vk.len();
		node z1 = Vk[1];
		node cl = Vk.left();  // left vertex
		node cr = Vk.right(); // right vertex

		// compute relative x-distance from c_i to cl for i = len+1, ..., r
		int x_cr = 0;
		node v;
		for (v = next[cl]; v != cr; v = next[v])
		{
			x_cr += x[v];
			x[v] = x_cr;
		}
		x_cr += x[cr]; // x_cr = abs. x-coord(cr) - abs. x-coord(cl)

		int offset;
		if (m_sizeOptimization) {
			// optimization: compute minimal value offset by which cr must be
			// shift to right
			int yMax = y[cr];

			// if there is an edge from cl to right with slope +1, or from cr
			// to left with slope -1, then offset must be at least 2
			offset = (y[cl] < y[next[cl]] || y[cr] < y[prev[cr]]) ? 2 : 0;

			// y_max = max { y[c_i] | len <= i <= r }
			for (v = cl; v != cr; v = next[v]) {
				if (y[v] > yMax)
					yMax = y[v];
			}

			// offset must be at least so large, such that
			// y[z_i] > y_max for all i
			offset = max (offset, 2*(yMax+len) - x_cr - y[cl] - y[cr]);

		} else // no size optimization
			offset = 2*len;

		x_cr += offset;

		// compute insert coordinates of z_i for 1 <= i <= len
		x[z1] = (x_cr + y[cr] - y[cl]) / 2 - len + 1;
		y[z1] = (x_cr + y[cr] + y[cl]) / 2 - len + 1;

		for (i = 2; i <= len; i++) {
			x[Vk[i]] = 2;
			y[Vk[i]] = y[z1];
		}

		// Compute shift values for cl,...,cr and relative x-coord. to a node
		// upper[v] for inner nodes
		// Let len <= c_alpha <= c_beta <= r max. with
		//		y[cl] > ... > y[c_alpha] and y[c_beta] < ... < y[cr]
		// Shift c_alpha,...,c_beta by offset/2 (c_alpha,c_beta only if != cl,
		// cr)
		// Shift c_beta+1,...,cr by offset
		// x-coord. of c_l+1,    ...,c_alpha  relative to cl
		//       -"-   c_alpha+1,...,c_beta-1 relative to z1
		//       -"-   c_beta,   ...,cr       relative to cr
		node c_alpha, c_beta;
		if (y[cl] <= y[next[cl]]) {
			c_alpha = cl;

		} else {
			for (c_alpha = next[cl], v = next[c_alpha];
				c_alpha != cr && y[v] < y[c_alpha];
				c_alpha = v, v = next[v])
			{
				x    [c_alpha] += 0;
				upper[c_alpha]  = cl;
			}
			if (c_alpha != cr) {
				x    [c_alpha] += (offset/2);
				upper[c_alpha]  = cl;
			}
		}

		if (y[cr] <= y[prev[cr]]) {
			c_beta = cr;

		} else {
			for (c_beta = prev[cr], v = prev[c_beta];
				c_beta != cl && y[v] < y[c_beta];
				c_beta = v, v = prev[v])
			{
				x    [c_beta] += offset - x_cr;
				upper[c_beta]  = cr;
			}
			if (c_beta != cl && c_beta != c_alpha) {
				x    [c_beta] += (offset/2) - x_cr;
				upper[c_beta]  = cr;
			}
		}

		if (c_alpha != c_beta)
			for (v = next[c_alpha]; v != c_beta; v = next[v]) {
				x    [v] += (offset/2) - x[z1];
				upper[v]  = z1;
			}

		x[cr] = x_cr - (x[z1] + 2*(len-1));

		// update contour after insertion of z_1,...,z_l
		for (i = 1; i <= len; i++)
		{
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
	for (node v = v1; v != nullptr; v = next[v]) {
		x[v] = (sum += x[v]);
	}

	// compute final x-coordinates for the inner nodes
	for (k = n-1; k >= 1; k--)
	{
		for (i = 1; i <= lmc.len(k); i++)
		{
			node zi = lmc (k,i);
			if (upper[zi] != nullptr)	// upper[zi] == 0 <=> z_i on contour
				x[zi] += x[upper[zi]];
		}
	}
}

}
