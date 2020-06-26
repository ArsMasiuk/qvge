#include <ogdf/graphalg/MaxAdjOrdering.h>
#include <ogdf/basic/graph_generators.h>

#include <graphs.h>
#include <testing.h>

void testAllMAOs(const Graph &G)
{
	Graph P;
	emptyGraph(P, G.numberOfNodes());
	MaxAdjOrdering perms;
	ListPure<ListPure<node>> allPerms;
	perms.calcAll(&P,&allPerms);

	MaxAdjOrdering m;
	ListPure<ListPure<node>> MAOs;
	m.calcAll(&G,&MAOs);

	AssertThat(m.testIfAllMAOs(&G,&MAOs,&allPerms), IsTrue());
}

void testMAOBfs(const Graph &G)
{
	MaxAdjOrdering m;
	ListPure<node> MAO;
	m.calcBfs(&G,&MAO);

	AssertThat(m.testIfMAO(&G,&MAO), IsTrue());
	AssertThat(m.testIfMAOBfs(&G,&MAO), IsTrue());
}

go_bandit([](){
	describe("Maximum Adjacency Orderings", [](){
		describe("calculate exactly all MAOs", [](){
			constexpr int MIN_N = 4;
			constexpr int MAX_N = 8;

			forEachGraphItWorks({GraphProperty::simple}, testAllMAOs,
				GraphSizes(MIN_N, MAX_N, 1), 0, MAX_N
			);
		});

		describe("calculate MAOs with correct lex-bfs tie breaking", [](){
			constexpr int MIN_N = 10;
			constexpr int MAX_N = 20;

			forEachGraphItWorks({GraphProperty::simple}, testMAOBfs,
				GraphSizes(MIN_N, MAX_N, 1), 0, MAX_N
			);
		});
	});
});
