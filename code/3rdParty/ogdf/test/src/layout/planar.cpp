/** \file
 * \brief Tests for several planar layouts
 *
 * \author Carsten Gutwenger, Tilo Wiedera
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

#include <ogdf/planarlayout/FPPLayout.h>
#include <ogdf/planarlayout/MixedModelLayout.h>
#include <ogdf/planarlayout/PlanarDrawLayout.h>
#include <ogdf/planarlayout/PlanarStraightLayout.h>
#include <ogdf/planarlayout/SchnyderLayout.h>
#include <ogdf/planarity/EmbedderMaxFace.h>
#include <ogdf/planarity/EmbedderMaxFaceLayers.h>
#include <ogdf/planarity/EmbedderMinDepth.h>
#include <ogdf/planarity/EmbedderMinDepthMaxFace.h>
#include <ogdf/planarity/EmbedderMinDepthMaxFaceLayers.h>
#include <ogdf/planarity/EmbedderMinDepthPiTa.h>
#include <ogdf/planarity/EmbedderOptimalFlexDraw.h>
#include <ogdf/planarity/SimpleEmbedder.h>
#include <ogdf/planarlayout/TriconnectedShellingOrder.h>
#include <ogdf/planarlayout/BiconnectedShellingOrder.h>

#include "layout_helpers.h"

template<typename Layout>
static void describeForAllEmbedders(string name, Layout &layout, std::set<GraphProperty> requirements = {}, bool skipMe = false) {
	name += " and ";

	requirements.insert({GraphProperty::planar, GraphProperty::simple});

	layout.setEmbedder(new SimpleEmbedder);

	describeLayout(name + "SimpleEmbedder", layout, 0, requirements, false, GraphSizes(), skipMe);

	layout.setEmbedder(new EmbedderMaxFace);
	describeLayout(name + "EmbedderMaxFace", layout, 0, requirements, false, GraphSizes(), skipMe);

	layout.setEmbedder(new EmbedderMaxFaceLayers);
	describeLayout(name + "EmbedderMaxFaceLayers", layout, 0, requirements, false, GraphSizes(), skipMe);

	layout.setEmbedder(new EmbedderMinDepth);
	describeLayout(name + "EmbedderMinDepth", layout, 0, requirements, false, GraphSizes(), skipMe);

	layout.setEmbedder(new EmbedderMinDepthMaxFace);
	describeLayout(name + "EmbedderMinDepthMaxFace", layout, 0, requirements, false, GraphSizes(), skipMe);

	layout.setEmbedder(new EmbedderMinDepthMaxFaceLayers);
	describeLayout(name + "EmbedderMinDepthMaxFaceLayers", layout, 0, requirements, false, GraphSizes(), skipMe);

	// the two embedders below are currently disabled since they cause failures

	layout.setEmbedder(new EmbedderMinDepthPiTa);
	describeLayout(name + "EmbedderMinDepthPiTa", layout, 0, requirements, false, GraphSizes(), true);

	layout.setEmbedder(new EmbedderOptimalFlexDraw);
	describeLayout(name + "EmbedderOptimalFlexDraw", layout, 0, requirements, false, GraphSizes(), true);
}

template<typename Layout>
static void describePlanarLayout(string name, std::set<GraphProperty> requirements = {}) {
	Layout layout;
	name += " with ";

	layout.setShellingOrder(new BiconnectedShellingOrder);
	describeForAllEmbedders(name + "BiconnectedShellingOrder", layout, requirements);

	requirements.insert(GraphProperty::triconnected);
	layout.setShellingOrder(new TriconnectedShellingOrder);
	describeForAllEmbedders(name + "TriconnectedShellingOrder", layout, requirements);
}

go_bandit([] { describe("Planar layouts", [] {
	describePlanarLayout<PlanarStraightLayout>("PlanarStraightLayout");
	describePlanarLayout<PlanarDrawLayout>("PlanarDrawLayout");

	describePlanarLayout<MixedModelLayout>("MixedModelLayout", {GraphProperty::connected});

	describeLayout<FPPLayout>("FPPLayout", 0, {GraphProperty::planar, GraphProperty::simple, GraphProperty::connected}, false, GraphSizes());

	describeLayout<SchnyderLayout>("SchnyderLayout", 0, {GraphProperty::planar, GraphProperty::simple, GraphProperty::connected}, false, GraphSizes());
}); });
