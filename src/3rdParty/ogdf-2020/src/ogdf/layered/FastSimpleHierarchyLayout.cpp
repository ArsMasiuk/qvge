/** \file
 * \brief Implementation of the FastSimpleHierarchyLayout
 * (third phase of sugiyama)
 *
 * \author Till Schäfer, Carsten Gutwenger
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


#include <ogdf/layered/FastSimpleHierarchyLayout.h>


namespace ogdf {

FastSimpleHierarchyLayout::FastSimpleHierarchyLayout()
{
	m_minXSep = LayoutStandards::defaultNodeSeparation();
	m_ySep    = 1.5 * LayoutStandards::defaultNodeSeparation();

	m_balanced    = true;
	m_downward    = true;
	m_leftToRight = true;
}


FastSimpleHierarchyLayout::FastSimpleHierarchyLayout(const FastSimpleHierarchyLayout &fshl)
{
	m_minXSep     = fshl.m_minXSep;
	m_ySep        = fshl.m_ySep;
	m_balanced    = fshl.m_balanced;
	m_downward    = fshl.m_downward;
	m_leftToRight = fshl.m_leftToRight;
}


FastSimpleHierarchyLayout &FastSimpleHierarchyLayout::operator=(const FastSimpleHierarchyLayout &fshl)
{
	m_minXSep     = fshl.m_minXSep;
	m_ySep        = fshl.m_ySep;
	m_balanced    = fshl.m_balanced;
	m_downward    = fshl.m_downward;
	m_leftToRight = fshl.m_leftToRight;

	return *this;
}


void FastSimpleHierarchyLayout::doCall(const HierarchyLevelsBase &levels, GraphAttributes &AGC)
{
	const Hierarchy &H  = levels.hierarchy();
	const GraphCopy &GC = H;

	if (GC.numberOfNodes() == 0) {
		return;
	}

	NodeArray<node> align(GC);

#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
	for(int i = 0; i <= levels.high(); ++i) {
		std::cout << "level " << i << ": ";
		const LevelBase &level = levels[i];
		for(int j = 0; j <= level.high(); ++j)
			std::cout << level[j] << " ";
		std::cout << std::endl;
	}
#endif

	if (m_balanced) {
		// the x positions; x = -infinity <=> x is undefined
		NodeArray<double> x[4];
		NodeArray<double> blockWidth[4];
		NodeArray<node> root[4];
		double width[4];
		double min[4];
		double max[4];
		int minWidthLayout = 0;

		// initializing
		for (int i = 0; i < 4; i++) {
			min[i] = std::numeric_limits<double>::max();
			max[i] = std::numeric_limits<double>::lowest();
		}

		// calc the layout for down/up and leftToRight/rightToLeft
		for (int downward = 0; downward <= 1; downward++) {
			NodeArray<NodeArray<bool> > type1Conflicts(GC);
			markType1Conflicts(levels, downward == 0, type1Conflicts);
			for (int leftToRight = 0; leftToRight <= 1; leftToRight++) {
				int k = 2 * downward + leftToRight;
				root[k].init(GC);
				verticalAlignment(levels, root[k], align, type1Conflicts, downward == 0, leftToRight == 0);
				computeBlockWidths(GC, AGC, root[k], blockWidth[k]);
				horizontalCompactation(align, levels, root[k], blockWidth[k], x[k], leftToRight == 0, downward == 0);
			}
		}

		/*
		* - calc min/max x coordinate for each layout
		* - calc x-width for each layout
		* - find the layout with the minimal width
		*/
		for (int i = 0; i < 4; i++) {
			for(node v : GC.nodes) {
				double bw = 0.5 * blockWidth[i][root[i][v]];
				double xp = x[i][v] - bw;
				if (min[i] > xp) {
					min[i] = xp;
				}
				xp = x[i][v] + bw;
				if (max[i] < xp) {
					max[i] = xp;
				}
			}
			width[i] = max[i] - min[i];
			if (width[minWidthLayout] > width[i]) {
				minWidthLayout = i;
			}
		}

		/*
		* shift the layout so that they align with the minimum width layout
		* - leftToRight: align minimum coordinate
		* - rightToLeft: align maximum coordinate
		*/
		double shift[4];
		for (int i = 0; i < 4; i++) {
			if (i % 2 == 0) {
				// for leftToRight layouts
				shift[i] = min[minWidthLayout] - min[i];
			} else {
				// for rightToLeft layouts
				shift[i] = max[minWidthLayout] - max[i];
			}
		}

		/*
		* shift the layouts and use the
		* median average coordinate for each node
		*/
		Array<double> sorting(4);
		for(node v : GC.nodes) {
			for (int i = 0; i < 4; i++) {
				sorting[i] = x[i][v] + shift[i];
			}
			sorting.quicksort();
			AGC.x(v) = 0.5 * (sorting[1] + sorting[2]);
		}

	} else {
		NodeArray<double> x;
		NodeArray<NodeArray<bool> > type1Conflicts(GC);
		markType1Conflicts(levels, m_downward, type1Conflicts);
		NodeArray<double> blockWidth; // the width of each block (max width of node in block)

		NodeArray<node> root(GC);
		verticalAlignment(levels, root, align, type1Conflicts, m_downward, m_leftToRight);
		computeBlockWidths(GC, AGC, root, blockWidth);
		horizontalCompactation(align, levels, root, blockWidth, x, m_leftToRight, m_downward);
		for(node v : GC.nodes) {
			AGC.x(v) = x[v];
		}
	}

	// compute y-coordinates
	const int k = levels.size();

	// compute height of each layer
	Array<double> height(0,k-1,0.0);

	for(int i = 0; i < k; ++i) {
		const LevelBase &level = levels[i];
		for(int j = 0; j < level.size(); ++j) {
			double h = getHeight(AGC, levels, level[j]);
			if(h > height[i])
				height[i] = h;
		}
	}

	// assign y-coordinates
	double yPos = 0.5 * height[0];

	for(int i = 0; ; ++i)
	{
		const LevelBase &level = levels[i];
		for(int j = 0; j < level.size(); ++j)
			AGC.y(level[j]) = yPos;

		if(i == k-1)
			break;

		yPos += m_ySep + 0.5 * (height[i] + height[i+1]);
	}
}


void FastSimpleHierarchyLayout::markType1Conflicts(const HierarchyLevelsBase &levels, const bool downward, NodeArray<NodeArray<bool> > &type1Conflicts)
{
	const GraphCopy& GC = levels.hierarchy();

	for(node v : GC.nodes) {
		type1Conflicts[v].init(GC,false);
	}

	if (levels.size() >= 4) {
		int upper, lower; // iteration bounds
		int k1; // node position boundaries of closest inner segments
		HierarchyLevelsBase::TraversingDir relupward; // upward relativ to the direction from downward

		if (downward) {
			lower = 1;
			upper = levels.high() - 2;

			relupward = HierarchyLevelsBase::TraversingDir::downward;
		}
		else {
			lower = levels.high() - 1;
			upper = 2;

			relupward = HierarchyLevelsBase::TraversingDir::upward;
		}

		/*
		 * iterate level[2..h-2] in the given direction
		 *
		 * availible levels: 1 to h
		 */
		for (int i = lower; (downward && i <= upper) || (!downward && i >= upper); i = downward ? i + 1 : i - 1)
		{
			int k0 = 0;
			int firstIndex = 0; // index of first node on layer
			const LevelBase &currentLevel = levels[i];
			const LevelBase &nextLevel = downward ? levels[i+1] : levels[i-1];

			// for all nodes on next level
			for (int l1 = 0; l1 <= nextLevel.high(); l1++) {
				const node virtualTwin = virtualTwinNode(levels, nextLevel[l1], relupward);

				if (l1 == nextLevel.high() || virtualTwin != nullptr) {
					k1 = currentLevel.high();

					if (virtualTwin != nullptr) {
						k1 = levels.pos(virtualTwin);
					}

					for (; firstIndex <= l1; firstIndex++) {
						const Array<node> &upperNeighbours = levels.adjNodes(nextLevel[l1], relupward);

						for (auto currentNeighbour : upperNeighbours) {
							/*
							 * XXX: < 0 in first iteration is still ok for indizes starting
							 * with 0 because no index can be smaller than 0
							 */
							if (levels.pos(currentNeighbour) < k0 || levels.pos(currentNeighbour) > k1) {
								type1Conflicts[nextLevel[l1]][currentNeighbour] = true;
							}
						}
					}
					k0 = k1;
				}
			}
		}
	}
}


void FastSimpleHierarchyLayout::verticalAlignment(
	const HierarchyLevelsBase &levels,
	NodeArray<node> &root,
	NodeArray<node> &align,
	const NodeArray<NodeArray<bool> > &type1Conflicts,
	bool downward,
	const bool leftToRight)
{
	const GraphCopy& GC = levels.hierarchy();
	HierarchyLevelsBase::TraversingDir relupward; // upward relativ to the direction from downward

	relupward = downward ? HierarchyLevelsBase::TraversingDir::downward : HierarchyLevelsBase::TraversingDir::upward;

	// initialize root and align
	for(node v : GC.nodes) {
		root[v] = v;
		align[v] = v;
	}

	// for all Level
	for (int i = downward ? 0 : levels.high();
		(downward && i <= levels.high()) || (!downward && i >= 0);
		i = downward ? i + 1 : i - 1)
	{
		const LevelBase &currentLevel = levels[i];
		int r = leftToRight ? -1 : std::numeric_limits<int>::max();

		// for all nodes on Level i (with direction leftToRight)
		for (int j = leftToRight ? 0 : currentLevel.high();
		     (leftToRight && j <= currentLevel.high()) || (!leftToRight && j >= 0);
		     leftToRight ? j++ : j--)
		{
			node v = currentLevel[j];
			// the first median
			int median = (int)floor((levels.adjNodes(v, relupward).size() + 1) / 2.0);

			int medianCount = (levels.adjNodes(v, relupward).size() % 2 == 1) ? 1 : 2;
			if (levels.adjNodes(v, relupward).size() == 0) {
				medianCount = 0;
			}

			// for all median neighbours in direction of H
			for (int count = 0; count < medianCount; count++) {
				node u = levels.adjNodes(v, relupward)[median + count - 1];

				if (align[v] == v
				 // if segment (u,v) not marked by type1 conflicts AND ...
				 && !type1Conflicts[v][u]
				 && ((leftToRight && r < levels.pos(u))
				  || (!leftToRight && r > levels.pos(u)))) {
					align[u] = v;
					root[v] = root[u];
					align[v] = root[v];
					r = levels.pos(u);
				}
			}
		}
	}

#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
	for(node v : GC.nodes) {
		std::cout
		  << "node: " << GC.original(v) << "/" << v
		  << ", root: " << GC.original(root[v]) << "/" << root[v]
		  << ", alignment: " << GC.original(align[v]) << "/" << align[v] << std::endl;
	}
#endif
}


void FastSimpleHierarchyLayout::computeBlockWidths(
	const GraphCopy &GC,
	const GraphAttributes &GCA,
	NodeArray<node> &root,
	NodeArray<double> &blockWidth)
{
	blockWidth.init(GC, 0.0);
	for(node v : GC.nodes) {
		if (!GC.isDummy(v)) {
			Math::updateMax(blockWidth[root[v]], GCA.width(v));
		}
	}
}


void FastSimpleHierarchyLayout::horizontalCompactation(
	const NodeArray<node> &align,
	const HierarchyLevelsBase &levels,
	const NodeArray<node> &root,
	const NodeArray<double> &blockWidth,
	NodeArray<double> &x,
	const bool leftToRight, bool downward)
{
#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
	std::cout << "-------- Horizontal Compactation --------" << std::endl;
#endif

	const GraphCopy& GC = levels.hierarchy();

	NodeArray<node> sink(GC);
	NodeArray<double> shift(GC, std::numeric_limits<double>::max());

	x.init(GC, std::numeric_limits<double>::lowest());

	for(node v : GC.nodes) {
		sink[v] = v;
	}

	// calculate class relative coordinates for all roots
	for (int i = downward ? 0 : levels.high();
		(downward && i <= levels.high()) || (!downward && i >= 0);
		i = downward ? i + 1 : i - 1)
	{
		const LevelBase &currentLevel = levels[i];

		for (int j = leftToRight ? 0 : currentLevel.high();
			(leftToRight && j <= currentLevel.high()) || (!leftToRight && j >= 0);
			leftToRight ? j++ : j--)
		{
			node v = currentLevel[j];
			if (root[v] == v) {
				placeBlock(v, sink, shift, x, align, levels, blockWidth, root, leftToRight);
			}
		}
	}

	double d = 0;
	for (int i = downward ? 0 : levels.high();
		(downward && i <= levels.high()) || (!downward && i >= 0);
		i = downward ? i + 1 : i - 1)
	{
		const LevelBase &currentLevel = levels[i];

		node v = currentLevel[leftToRight ? 0 : currentLevel.high()];

		if(v == sink[root[v]]) {
			double oldShift = shift[v];
			if(oldShift < std::numeric_limits<double>::max()) {
				shift[v] += d;
				d += oldShift;
			} else
				shift[v] = 0;
		}
	}

#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
	std::cout << "------- Sinks ----------" << std::endl;
#endif
	// apply root coordinates for all aligned nodes
	// (place block did this only for the roots)
	for(node v : GC.nodes) {
#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
		if (sink[root[v]] == v) {
			std::cout << "Topmost Root von Senke!: " << GC.original(v) << std::endl;
			std::cout << "-> Shift: " << shift[v] << std::endl;
			std::cout << "-> x: " << x[v] << std::endl;
		}
#endif
		x[v] = x[root[v]];
	}

	// apply shift for each class
	for(node v : GC.nodes) {
		x[v] += shift[sink[root[v]]];
	}
}


void FastSimpleHierarchyLayout::placeBlock(
	node v,
	NodeArray<node> &sink,
	NodeArray<double> &shift,
	NodeArray<double> &x,
	const NodeArray<node> &align,
	const HierarchyLevelsBase &levels,
	const NodeArray<double> &blockWidth,
	const NodeArray<node> &root,
	const bool leftToRight)
{
	const Hierarchy &H = levels.hierarchy();

#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
	const GraphCopy& GC = H;
#endif

	if (x[v] == std::numeric_limits<double>::lowest()) {
		x[v] = 0;
		node w = v;
#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
		std::cout << "---placeblock: " << GC.original(v) << " ---" << std::endl;
#endif
		do {
			// if not first node on layer
			if ((leftToRight && levels.pos(w) > 0) || (!leftToRight && levels.pos(w) < levels[H.rank(w)].high())) {
				node u = root[pred(w, levels, leftToRight)];
				placeBlock(u, sink, shift, x, align, levels, blockWidth, root, leftToRight);
				if (sink[v] == v) {
					sink[v] = sink[u];
				}
				if (sink[v] != sink[u]) {
#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
					std::cout << "old shift " << GC.original(sink[u]) << ": " << shift[sink[u]] << "<>" << x[v] - x[u] - m_minXSep << std::endl;
#endif
					if (leftToRight) {
						shift[sink[u]] = min<double>(shift[sink[u]], x[v] - x[u] - m_minXSep - 0.5 * (blockWidth[u] + blockWidth[v]));
					} else {
						shift[sink[u]] = max<double>(shift[sink[u]], x[v] - x[u] + m_minXSep + 0.5 * (blockWidth[u] + blockWidth[v]));
					}
#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
					std::cout << "-> new shift: " << shift[sink[u]] << std::endl;
#endif
				}
				else {
					if (leftToRight) {
						x[v] = max<double>(x[v], x[u] + m_minXSep + 0.5 * (blockWidth[u] + blockWidth[v]));
					} else {
						x[v] = min<double>(x[v], x[u] - m_minXSep - 0.5 * (blockWidth[u] + blockWidth[v]));
					}
				}
#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
				std::cout << "placing w: " << GC.original(w) << "; predecessor: " << GC.original(pred(w, levels, leftToRight)) <<
					"; root(w)=v: " << GC.original(v) << "; root(pred(u)): " << GC.original(u) <<
					"; sink(v): " << GC.original(sink[v]) << "; sink(u): " << GC.original(sink[u]) << std::endl;
				std::cout << "x(v): " << x[v] << std::endl;
			} else {
				std::cout << "not placing w: " << GC.original(w) << " because at beginning of layer" << std::endl;
#endif
			}
			w = align[w];
		} while (w != v);
#ifdef OGDF_FAST_SIMPLE_HIERARCHY_LAYOUT_LOGGING
		std::cout << "---END placeblock: " << GC.original(v) << " ---" << std::endl;
#endif
	}
}


node FastSimpleHierarchyLayout::virtualTwinNode(const HierarchyLevelsBase &levels, const node v, const HierarchyLevelsBase::TraversingDir dir) const
{
	const Hierarchy &H = levels.hierarchy();

	if (!H.isLongEdgeDummy(v) || levels.adjNodes(v, dir).size() == 0) {
		return nullptr;
	}

	if (levels.adjNodes(v, dir).size() > 1) {
		// since v is a dummy there sould be only one upper neighbour
		throw AlgorithmFailureException("FastSimpleHierarchyLayout.cpp");
	}

	return *levels.adjNodes(v, dir).begin();
}


node FastSimpleHierarchyLayout::pred(const node v, const HierarchyLevelsBase &levels, const bool leftToRight)
{
	const Hierarchy &H = levels.hierarchy();

	int pos = levels.pos(v);
	int rank = H.rank(v);

	const LevelBase &level = levels[rank];
	if ((leftToRight && pos != 0) || (!leftToRight && pos != level.high())) {
		return level[leftToRight ? pos - 1 : pos + 1];
	}
	return nullptr;
}

}
