/** \file
 * \brief Declares class LayoutStandards which specifies default /
 *        standard values used in graph layouts.
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

#pragma once

#include <ogdf/basic/graphics.h>


namespace ogdf {

//! Standard values for graphical attributes and layouts.
/**
 * @ingroup graph-drawing
 */
class OGDF_EXPORT LayoutStandards {

	static double s_defNodeWidth;	//!< the default width of a node (must be positive).
	static double s_defNodeHeight;	//!< the default height of a node (must be positive).
	static Shape  s_defNodeShape;	//!< the default shape of a node.
	static Stroke s_defNodeStroke;	//!< the default stroke of a node.
	static Fill   s_defNodeFill;	//!< the default fill of a node.

	static Stroke    s_defEdgeStroke;	//!< the default stroke of an edge.
	static EdgeArrow s_defEdgeArrow;	//!< the default arrow type of an edge .

	static Stroke s_defClusterStroke;	//!< the default cluster stroke.
	static Fill   s_defClusterFill;		//!< the default cluster fill.

	static double s_defNodeSeparation;	//!< the default node separation (for layout algorithms) (must be positive).
	static double s_defCCSeparation;	//!< the default separation between connected components (for layout algorithms) (must be positive).

public:

	/**
	 * @name Global default node attributes
	 * These attribute values are used by GraphAttributes as default graphical representation of nodes.
	 */
	//@{

	//! Returns the global default width for nodes.
	static double defaultNodeWidth() { return s_defNodeWidth; }
	//! Sets the global default width for nodes to \p w.
	/**
	 * \param w must be a positive value and is set as new default node width.
	 */
	static void setDefaultNodeWidth(double w) {
		if(w > 0.0)
			s_defNodeWidth = w;
	}

	//! Returns the global default height for nodes.
	static double defaultNodeHeight() { return s_defNodeHeight; }
	//! Sets the global default height for nodes to \p h.
	/**
	 * \param h must be a positive value and is set as new default node height.
	 */
	static void setDefaultNodeHeight(double h) {
		if(h > 0.0)
			s_defNodeHeight = h;
	}

	//! Returns the global default shape for nodes.
	static Shape defaultNodeShape() { return s_defNodeShape; }
	//! Sets the global default shape for nodes to \p s.
	static void setDefaultNodeShape(Shape s) { s_defNodeShape = s; }

	//! Returns the global default stroke for nodes.
	static Stroke defaultNodeStroke() { return s_defNodeStroke; }
	//! Returns the global default stroke color for nodes.
	static Color defaultNodeStrokeColor() { return s_defNodeStroke.m_color; }
	//! Returns the global default stroke width for nodes.
	static float defaultNodeStrokeWidth() { return s_defNodeStroke.m_width; }
	//! Sets the global default stroke for nodes to \p stroke.
	static void setDefaultNodeStroke(Stroke stroke) { s_defNodeStroke = stroke; }

	//! Returns the global default fill for nodes.
	static Fill defaultNodeFill() { return s_defNodeFill; }
	//! Returns the global default fill color for nodes.
	static Color defaultNodeFillColor() { return s_defNodeFill.m_color; }
	//! Sets the global default fill for nodes to \p fill.
	static void setDefaultNodeFill(Fill fill) { s_defNodeFill = fill; }

	//@}
	/**
	 * @name Global default edge attributes
	 * These attribute values are used by GraphAttributes as default graphical representation of edges.
	 */
	//@{

	//! Returns the global default stroke for edges.
	static Stroke defaultEdgeStroke() { return s_defEdgeStroke; }
	//! Returns the global default stroke color for edges.
	static Color defaultEdgeStrokeColor() { return s_defEdgeStroke.m_color; }
	//! Returns the global default stroke width for edges.
	static float defaultEdgeStrokeWidth() { return s_defEdgeStroke.m_width; }
	//! Sets the global default stroke for edges to \p stroke.
	static void setDefaultEdgeStroke(Stroke stroke) { s_defEdgeStroke = stroke; }

	//! Returns the global default arrow type for edges.
	static EdgeArrow defaultEdgeArrow() { return s_defEdgeArrow; }
	//! Sets the global default arrow type for edges to \p arrow.
	static void setDefaultEdgeArrow(EdgeArrow arrow) { s_defEdgeArrow = arrow; }

	//@}
	/**
	 * @name Global default cluster attributes
	 * These attribute values are used by ClusterGraphAttributes as default graphical representation of clusters.
	 */
	//@{

	//! Returns the global default stroke for clusters.
	static Stroke defaultClusterStroke() { return s_defClusterStroke; }
	//! Returns the global default stroke color for clusters.
	static Color defaultClusterStrokeColor() { return s_defClusterStroke.m_color; }
	//! Returns the global default stroke width for clusters.
	static float defaultClusterStrokeWidth() { return s_defClusterStroke.m_width; }
	//! Sets the global default stroke for cluster to \p stroke.
	static void setDefaultClusterStroke(Stroke stroke) { s_defClusterStroke = stroke; }

	//! Returns the global default fill for clusters.
	static Fill defaultClusterFill() { return s_defClusterFill; }
	//! Returns the global default fill color for clusters.
	static Color defaultClusterFillColor() { return s_defClusterFill.m_color; }
	//! Sets the global default fill for clusters to \p fill.
	static void setDefaultClusterFill(Fill fill) { s_defClusterFill = fill; }

	//@}
	/**
	 * @name Global default separation parameters
	 * These values \a can be used by layout algorithms as useful settings for separation parameters.
	 */
	//@{

	//! Returns the global default node separation.
	static double defaultNodeSeparation() { return s_defNodeSeparation; }
	//! Sets the global default node separation to \p d.
	/**
	 * \param d must be a positive value and is set as new default node separation.
	 */
	static void setDefaultNodeSeparation(double d) {
		if(d > 0.0)
			s_defNodeSeparation = d;
	}

	//! Returns the global default separation between connected components.
	static double defaultCCSeparation() { return s_defCCSeparation; }
	//! Sets the global default separation between connected components to \p d.
	/**
	 * \param d must be a positive value and is set as new default separation between connected components.
	 */
	static void setDefaultCCSeparation(double d) {
		if(d > 0.0)
			s_defCCSeparation = d;
	}

	//@}
};

}
