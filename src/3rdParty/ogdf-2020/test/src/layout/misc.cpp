/** \file
 * \brief Tests for several planar layouts
 *
 * \author Tilo Wiedera
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

#include <ogdf/basic/PreprocessorLayout.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/misclayout/BalloonLayout.h>
#include <ogdf/misclayout/BertaultLayout.h>
#include <ogdf/misclayout/CircularLayout.h>
#include <ogdf/misclayout/LinearLayout.h>
#include <ogdf/misclayout/ProcrustesSubLayout.h>
#include <ogdf/packing/ComponentSplitterLayout.h>
#include <ogdf/packing/SimpleCCPacker.h>
#include <ogdf/planarlayout/MixedModelLayout.h>
#include <ogdf/tree/RadialTreeLayout.h>
#include <ogdf/tree/TreeLayout.h>
#include <ogdf/upward/DominanceLayout.h>
#include <ogdf/upward/VisibilityLayout.h>

#include "layout_helpers.h"

go_bandit([] { describe("Miscellaneous layouts", [] {
	GraphSizes smallSizes = GraphSizes(16, 32, 16);

	PreprocessorLayout preProc;
	// CircularLayout requires simple graphs
	preProc.setLayoutModule(new CircularLayout);
	describeLayout("PreprocessorLayout with CircularLayout", preProc, 0);

	TEST_LAYOUT(BalloonLayout, GraphProperty::connected);

	describeLayout<BertaultLayout>("BertaultLayout", 0, {GraphProperty::sparse, GraphProperty::simple}, false, smallSizes);
	TEST_LAYOUT(CircularLayout, GraphProperty::simple);
	TEST_LAYOUT(LinearLayout);

	ProcrustesSubLayout procrustesLayout(new FMMMLayout);
	describeLayout("ProcrustesSubLayout", procrustesLayout);

	TEST_LAYOUT(ComponentSplitterLayout);

	// BalloonLayout requires connectivity
	SimpleCCPacker packerLayout(new BalloonLayout);
	describeLayout("SimpleCCPacker with BalloonLayout", packerLayout);

	TEST_LAYOUT(RadialTreeLayout, GraphProperty::arborescenceForest, GraphProperty::connected);
	TEST_LAYOUT(TreeLayout, GraphProperty::arborescenceForest);

	// skip iteration with maximum number of nodes as it takes too long
	describeLayout<DominanceLayout>("DominanceLayout", 0, {GraphProperty::connected, GraphProperty::simple, GraphProperty::sparse}, false, smallSizes);
	describeLayout<VisibilityLayout>("VisibilityLayout", 0, {GraphProperty::connected, GraphProperty::simple, GraphProperty::sparse}, false, smallSizes);
}); });
