/** \file
 * \brief Tests for several layered layout algorithms.
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

#include <ogdf/layered/BlockOrder.h>
#include <ogdf/layered/FastHierarchyLayout.h>
#include <ogdf/layered/FastSimpleHierarchyLayout.h>
#include <ogdf/layered/HierarchyLevels.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>

#include "layout_helpers.h"

#define TEST_HIERARCHY_LAYOUT(TYPE, SKIP, ...) describeHierarchyLayout<TYPE>(#TYPE, SKIP, {__VA_ARGS__})

template<class Layout, class Levels>
class HierarchyMock : public LayoutModule {
	Layout layout;

public:
	virtual void call(GraphAttributes &attr) override {
		const Graph& G = attr.constGraph();
		NodeArray<int> nodeRank(G);

		int numberOfRanks = sqrt(G.numberOfNodes());

		int i = 0;
		for(node v : G.nodes) {
			// each rank must contain at least one node
			nodeRank[v] = i < numberOfRanks ? i++ : randomNumber(0, numberOfRanks);
		}

		Hierarchy hierarchy(G, nodeRank);
		Levels levels(hierarchy);
		layout.call(levels, attr);
	}
};

template<class Layout>
void describeHierarchyLayout(const string& name, bool skipMe, std::initializer_list<GraphProperty> requirements) {
	std::set<GraphProperty> reqs(requirements);
	reqs.insert(GraphProperty::sparse);
	// TODO BlockOrder is infested by bugs. It is skipped for now.
	describeLayout<HierarchyMock<Layout, BlockOrder>>(name + " with BlockOrder", 0, reqs, false, GraphSizes(), true);
	describeLayout<HierarchyMock<Layout, HierarchyLevels>>(name + " with HierarchyLevels", 0, reqs, false, GraphSizes(), skipMe);
}

go_bandit([] { describe("Layered layouts", [] {
	TEST_HIERARCHY_LAYOUT(FastHierarchyLayout, false);
	TEST_HIERARCHY_LAYOUT(FastSimpleHierarchyLayout, false);
	TEST_HIERARCHY_LAYOUT(OptimalHierarchyLayout, false, GraphProperty::simple);
}); });
