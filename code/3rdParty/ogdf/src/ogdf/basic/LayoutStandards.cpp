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

#include <ogdf/basic/LayoutStandards.h>

namespace ogdf {

double LayoutStandards::s_defNodeWidth = 20.0;
double LayoutStandards::s_defNodeHeight = 20.0;
Shape  LayoutStandards::s_defNodeShape = Shape::Rect;
Stroke LayoutStandards::s_defNodeStroke(Color::Name::Black);
Fill   LayoutStandards::s_defNodeFill(Color::Name::White);

Stroke    LayoutStandards::s_defEdgeStroke(Color::Name::Black);
EdgeArrow LayoutStandards::s_defEdgeArrow = EdgeArrow::Last;

Stroke LayoutStandards::s_defClusterStroke(Color::Name::Gray);
Fill   LayoutStandards::s_defClusterFill(Color::Name::White, FillPattern::None);

double LayoutStandards::s_defNodeSeparation = 20.0;
double LayoutStandards::s_defCCSeparation = 30.0;

}
