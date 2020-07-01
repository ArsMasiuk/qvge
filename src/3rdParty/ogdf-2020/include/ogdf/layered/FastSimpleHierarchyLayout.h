/** \file
 * \brief declaration of the FastSimpleHierarchyLayout
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

#pragma once

#include <ogdf/layered/HierarchyLayoutModule.h>
#include <ogdf/layered/Hierarchy.h>
#include <ogdf/basic/NodeArray.h>

namespace ogdf {

/**
 * \brief Coordinate assignment phase for the Sugiyama algorithm by Ulrik Brandes and Boris Köpf
 *
 * @ingroup gd-hlm
 *
 * This class implements a hierarchy layout algorithm, i.e., it layouts
 * hierarchies with a given order of nodes on each layer. It is used as a third
 * phase of the Sugiyama algorithm.
 *
 * The Algorithm runs in three phases.<br>
 * - Alignment (4x)<br>
 * - Horizontal Compactation (4x)<br>
 * - Balancing
 *
 * The <i>Alignment</i> and <i>Horzontal Compactation</i> phase are calculated downward, upward,
 * left-to-right and right-to-left. The four resulting layouts are combined in a balancing step.
 *
 * The implementation is based on:
 *
 * Ulrik Brandes, Boris Köpf: <i>Fast and Simple Horizontal Coordinate Assignment</i>.
 * LNCS 2002, Volume 2265/2002, pp. 33-36
 *
 * <h3>Optional Parameters</h3>
 *
 * <table>
 *   <tr>
 *     <th>Option</th><th>Type</th><th>Default</th><th>Description</th>
 *   </tr><tr>
 *     <td><i>node distance</i></td><td>int</td><td>150</td>
 *     <td>the minimal horizontal distance between two nodes on the same layer</td>
 *   </tr><tr>
 *     <td><i>layer distance</i></td><td>int</td><td>75</td>
 *     <td>the minimal vertical distance between two nodes on adjacent layers</td>
 *   </tr><tr>
 *     <td><i>balanced</i></td><td>bool</td><td>true</td>
 *     <td>determines whether balancing is used</td>
 *   </tr><tr>
 *     <td><i>left-to-right</i></td><td>bool</td><td>true</td>
 *     <td>determines whether block alignment is computed by a left-to-right (true) or right-to-left traversal</td>
 *   </tr><tr>
 *     <td><i>downward</i></td><td>bool</td><td>true</td>
 *     <td>determines whether block alignment is computed by a downward (true) or upward traversal</td>
 *   </tr>
 * </table>
 */
class OGDF_EXPORT FastSimpleHierarchyLayout : public HierarchyLayoutModule
{
private:
	double m_minXSep;	//!< stores the option <i>node distance</i>.
	double m_ySep;		//!< stores the option <i>layer distance</i>.
	bool   m_balanced;	//!< stores the option <i>balanced</i>.
	bool   m_downward;	//!< stores the option <i>downward</i>.
	bool   m_leftToRight;	//!< stores the option <i>left-to-right</i>.

protected:
	virtual void doCall(const HierarchyLevelsBase &levels, GraphAttributes &AGC) override;

public:
	//! Creates an instance of fast simple hierarchy layout.
	/**
	 * Sets the options to default values, i.e., use balanced layout with left-to-right node
	 * direction on each layer, node distance as given by LayoutStandards, and layer distance as
	 * 1.5 times node distance.
	 */
	FastSimpleHierarchyLayout();


	//! Copy constructor.
	FastSimpleHierarchyLayout(const FastSimpleHierarchyLayout &);

	// destructor
	virtual ~FastSimpleHierarchyLayout() { }


	//! Assignment operator
	FastSimpleHierarchyLayout &operator=(const FastSimpleHierarchyLayout &);


	//! Returns the option <i>node distance</i>.
	double nodeDistance() const {
		return m_minXSep;
	}

	//! Sets the option node distance to \p dist.
	void nodeDistance(double dist) {
		m_minXSep = dist;
	}

	//! Returns the option <i>layer distance</i>.
	double layerDistance() const {
		return m_ySep;
	}

	//! Sets the option <i>layer distance</i> to \p dist.
	void layerDistance(double dist) {
		m_ySep = dist;
	}

	//! Returns the option <i>downward</i>.
	bool downward() const {
		return m_downward;
	}

	//! Sets the option <i>downward</i> to \p d.
	void downward(bool d) {
		m_downward = d;
	}

	//! Returns the option <i>left-to-right</i>.
	bool leftToRight() const {
		return m_leftToRight;
	}

	//! Sets the option <i>left-to-right</i> to \p b.
	void leftToRight(bool b) {
		m_leftToRight = b;
	}

	//! Returns the option <i>balanced</i>.
	bool balanced() const {
		return m_balanced;
	}

	//! Sets the option <i>balanced</i> to \p b.
	void balanced(bool b) {
		m_balanced = b;
	}


private:
	/**
	 * Preprocessing step to find all type1 conflicts.
	 * A type1 conflict is a crossing of a inner segment with a non-inner segment.
	 *
	 * This is for preferring straight inner segments.
	 *
	 * @param levels The Hierarchy
	 * @param downward The level direction
	 * @param type1Conflicts is assigned the conflicts, (type1Conflicts[v])[u]=true means (u,v) is marked, u is the upper node
	 */
	void markType1Conflicts(const HierarchyLevelsBase &levels, bool downward, NodeArray<NodeArray<bool> > &type1Conflicts);

	/**
	 * Align each node to a node on the next higher level. The result is a blockgraph where each
	 * node is in a block whith a nother node when they have the same root.
	 *
	 * @param levels The Hierarchy
	 * @param root The root for each node (calculated by this method)
	 * @param align The alignment to the next level node (align(v)=u <=> u is aligned to v) (calculated by this method)
	 * @param type1Conflicts Type1 conflicts to prefer straight inner segments
	 * @param downward The level direction
	 * @param leftToRight The node direction on each level
	 */
	void verticalAlignment(
		const HierarchyLevelsBase &levels,
		NodeArray<node> &root,
		NodeArray<node> &align,
		const NodeArray<NodeArray<bool> > &type1Conflicts,
		const bool downward,
		const bool leftToRight);

	/**
	 * Computes the width of each block, i.e., the maximal width of a node in the block, and
	 * stores it in blockWidth for the root of the block.
	 *
	 * @param GC The input graph copy
	 * @param GCA The input graph copies (gives in particular the widths of nodes)
	 * @param root The root for each node (calculated by this method)
	 * @param blockWidth Is assigned the width of each block (stored for the root)
	 */
	void computeBlockWidths(
		const GraphCopy &GC,
		const GraphAttributes &GCA,
		NodeArray<node> &root,
		NodeArray<double> &blockWidth);

	/**
	 * Calculate the coordinates for each node
	 *
	 * @param align The alignment to the next level node (align(v)=u <=> u is aligned to v)
	 * @param levels The Hierarchy
	 * @param root The root for each node
	 * @param blockWidth The width of each block
	 * @param x The x-coordinates for each node (calculated by this method)
	 * @param leftToRight The node direction on each level
	 * @param downward The level direction
	 */
	void horizontalCompactation(
		const NodeArray<node> &align,
		const HierarchyLevelsBase &levels,
		const NodeArray<node> &root,
		const NodeArray<double> &blockWidth,
		NodeArray<double> &x,
		const bool leftToRight,
		bool downward);

	/**
	 * Calculate the coordinate for root nodes (placing)
	 *
	 * @param v The root node to place
	 * @param sink The Sink for each node. A sink identifies each block class (calculated by this method)
	 * @param shift The shift for each class (calculated by this method)
	 * @param x The class relative x-coordinate for each node (calculated by this method)
	 * @param align The alignment to the next level node (align(v)=u <=> u is aligned to v)
	 * @param levels The Hierarchy
	 * @param blockWidth The width of each block
	 * @param root The root for each node
	 * @param leftToRight The node direction on each level
	 */
	void placeBlock(
		node v,
		NodeArray<node> &sink,
		NodeArray<double> &shift,
		NodeArray<double> &x,
		const NodeArray<node> &align,
		const HierarchyLevelsBase &levels,
		const NodeArray<double> &blockWidth,
		const NodeArray<node> &root,
		const bool leftToRight);

	/**
	 * The twin of an inner Segment
	 *
	 * @return Parent node which is connected by an inner segment.
	 * nullptr if there is no parent segment or if the segment is not an inner segment.
	 */
	node virtualTwinNode(const HierarchyLevelsBase &levels, const node v, const HierarchyLevelsBase::TraversingDir dir) const;

	/**
	 * Predecessor of v on the same level,
	 *
	 * @param v The node for which the predecessor should be calculated.
	 * @param levels The Hierarchy
	 * @param leftToRight If true the left predecessor is choosen. Otherwise the right predecessor.
	 * @return Predescessor on the same level. nullptr if there is no predecessor.
	 */
	node pred(const node v, const HierarchyLevelsBase &levels, const bool leftToRight);
};

}
