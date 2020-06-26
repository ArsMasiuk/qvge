/** \file
 * \brief Tests for SugiyamaLayout
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

#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/FastHierarchyLayout.h>
#include <ogdf/layered/MedianHeuristic.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/layered/GreedyCycleRemoval.h>
#include <ogdf/layered/CoffmanGrahamRanking.h>
#include <ogdf/layered/LongestPathRanking.h>
#include <ogdf/layered/DfsAcyclicSubgraph.h>
#include <ogdf/layered/FastSimpleHierarchyLayout.h>
#include <ogdf/layered/GridSifting.h>
#include <ogdf/layered/BarycenterHeuristic.h>
#include <ogdf/layered/GreedyInsertHeuristic.h>
#include <ogdf/layered/GreedySwitchHeuristic.h>
#include <ogdf/layered/SplitHeuristic.h>
#include <ogdf/layered/SiftingHeuristic.h>

#include "layout_helpers.h"

#define DESCRIBE_SUGI_LAYOUT(TYPE, ...) describeSugi<TYPE>(#TYPE, __VA_ARGS__)
#define DESCRIBE_SUGI_RANKING(TYPE, ...) describeSugiRanking<TYPE>(#TYPE, __VA_ARGS__)
#define DESCRIBE_SUGI_CROSSMIN(TYPE, ...) describeSugiCrossMin<TYPE>(#TYPE, __VA_ARGS__)

template<class CrossMin>
void describeSugiCrossMin(const std::string& name, SugiyamaLayout& sugi, const std::set<GraphProperty>& reqs, bool skipMe = false) {
	sugi.setCrossMin(new CrossMin);
	describeLayout(name, sugi, 0, reqs, false, GraphSizes(16, 32, 16), skipMe);
}

template<class Ranking>
void describeSugiRanking(const std::string& name, SugiyamaLayout& sugi, const std::set<GraphProperty>& reqs, Ranking* ranking = nullptr) {
	describe(name, [&] {
		sugi.setRanking(ranking == nullptr ? new Ranking : ranking);

		auto reqsGS = reqs;
		reqsGS.insert(GraphProperty::connected);

		// TODO GlobalSifting and GridSifting use BlockOrder which appears broken
		DESCRIBE_SUGI_CROSSMIN(GlobalSifting, sugi, reqs, true);
		DESCRIBE_SUGI_CROSSMIN(GridSifting, sugi, reqs, true);
		DESCRIBE_SUGI_CROSSMIN(BarycenterHeuristic, sugi, reqs);
		DESCRIBE_SUGI_CROSSMIN(GreedyInsertHeuristic, sugi, reqs);
		DESCRIBE_SUGI_CROSSMIN(GreedySwitchHeuristic, sugi, reqsGS);
		DESCRIBE_SUGI_CROSSMIN(MedianHeuristic, sugi, reqs);
		DESCRIBE_SUGI_CROSSMIN(SiftingHeuristic, sugi, reqs);
		DESCRIBE_SUGI_CROSSMIN(SplitHeuristic, sugi, reqs);
	});
}

template<class Layout>
void describeSugi(const std::string& name, const std::set<GraphProperty>& reqs) {
	describe(name, [&] {
		SugiyamaLayout sugi;
		sugi.runs(std::min(4u, Thread::hardware_concurrency()));
		sugi.setLayout(new Layout);

		DESCRIBE_SUGI_RANKING(CoffmanGrahamRanking, sugi, reqs);
		DESCRIBE_SUGI_RANKING(LongestPathRanking, sugi, reqs);

		auto* r = new OptimalRanking;
		r->setSubgraph(new DfsAcyclicSubgraph);
		describeSugiRanking<>("OptimalRanking with DfsAcyclicSubgraph", sugi, reqs, r);

		r = new OptimalRanking;
		r->setSubgraph(new GreedyCycleRemoval);
		describeSugiRanking<>("OptimalRanking with GreedyCycleRemoval", sugi, reqs, r);
	});
}

go_bandit([] {
	describe("SugiyamaLayout", [] {
		DESCRIBE_SUGI_LAYOUT(FastHierarchyLayout, {GraphProperty::sparse});
		DESCRIBE_SUGI_LAYOUT(FastSimpleHierarchyLayout, {GraphProperty::sparse});
		describeSugi<OptimalHierarchyLayout>("OptimalHierarchyLayout", {GraphProperty::simple, GraphProperty::sparse});
	});
});
