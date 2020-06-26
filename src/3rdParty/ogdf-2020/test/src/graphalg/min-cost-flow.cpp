/** \file
 * \brief Tests for Min-Cost Flow Algorithms
 *
 * \author Stephan Beyer
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

#include "ogdf/graphalg/MinCostFlowReinelt.h"
#include <testing.h>

template<typename TCost>
void testModule(const char *name, MinCostFlowModule<TCost> *alg, TCost base)
{
	describe(name, [&]() {
		Graph G;
		node s = G.newNode();
		node a = G.newNode();
		node b = G.newNode();
		node t = G.newNode();
		edge sa = G.newEdge(s, a);
		edge sb = G.newEdge(s, b);
		edge at = G.newEdge(a, t);
		edge bt = G.newEdge(b, t);
		edge ab = G.newEdge(a, b);
		EdgeArray<int> lb(G, 0);
		it("works with costs all being zero", [&]() {
			EdgeArray<int> cap(G, 10000);
			cap[ab] = 1;
			EdgeArray<TCost> cost(G, 0);
			NodeArray<int> supply(G, 0);
			supply[s] = 20000;
			supply[t] = -20000;
			EdgeArray<int> flow(G);
			bool feasible = alg->call(G, lb, cap, cost, supply, flow);

			AssertThat(feasible, Equals(true));
			AssertThat(flow[sa], Equals(10000));
			AssertThat(flow[sb], Equals(10000));
			AssertThat(flow[at], Equals(10000));
			AssertThat(flow[bt], Equals(10000));
			AssertThat(flow[ab], Equals(0));
		});
		it("works with non-negative cost", [&]() {
			EdgeArray<int> cap(G, 10000);
			cap[ab] = 1;
			EdgeArray<TCost> cost(G, 100*base);
			cost[at] = 99*base;
			cost[ab] = 0;
			cost[sa] = base;
			cost[bt] = base;
			NodeArray<int> supply(G, 0);
			supply[s] = 10000;
			supply[t] = -10000;
			EdgeArray<int> flow(G);
			bool feasible = alg->call(G, lb, cap, cost, supply, flow);

			AssertThat(feasible, Equals(true));
			AssertThat(flow[sa], Equals(10000));
			AssertThat(flow[sb], Equals(0));
			AssertThat(flow[at], Equals(9999));
			AssertThat(flow[bt], Equals(1));
			AssertThat(flow[ab], Equals(1));
		});
		it("works with positive and negative cost", [&]() {
			EdgeArray<int> cap(G, 10000);
			cap[ab] = 1;
			EdgeArray<TCost> cost(G, base);
			cost[ab] = -base;
			NodeArray<int> supply(G, 0);
			supply[s] = 10000;
			supply[t] = -10000;
			EdgeArray<int> flow(G);
			bool feasible = alg->call(G, lb, cap, cost, supply, flow);

			AssertThat(feasible, Equals(true));
			AssertThat(flow[sa], Equals(10000));
			AssertThat(flow[sb], Equals(0));
			AssertThat(flow[at], Equals(9999));
			AssertThat(flow[bt], Equals(1));
			AssertThat(flow[ab], Equals(1));
		});
		it("is unfeasible if supply = demand > edge capacities", [&]() {
			EdgeArray<int> cap(G, 10000);
			cap[sb] = cap[at] = 5000;
			cap[ab] = 1;
			EdgeArray<TCost> cost(G, -base);
			NodeArray<int> supply(G, 0);
			supply[s] = 15000;
			supply[t] = -15000;
			EdgeArray<int> flow(G);
			bool feasible = alg->call(G, lb, cap, cost, supply, flow);

			AssertThat(feasible, Equals(false));
		});
	});
	delete alg;
}

go_bandit([]() {
describe("Min-Cost Flow algorithms", []() {
	testModule<int>("MinCostFlowReinelt with integral cost", new MinCostFlowReinelt<int>(), 1);
	testModule<double>("MinCostFlowReinelt wit real (double) cost [1]", new MinCostFlowReinelt<double>(), 1.92);
	testModule<double>("MinCostFlowReinelt wit real (double) cost [2]", new MinCostFlowReinelt<double>(), 0.1432);
});
});
