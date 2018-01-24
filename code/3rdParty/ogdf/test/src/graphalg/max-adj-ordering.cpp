#include <ogdf/graphalg/MaxAdjOrdering.h>
#include <ogdf/basic/graph_generators.h>

#include <testing.h>

go_bandit([](){
	describe("Maximum Adjacency Orderings", [](){
		it("should calculate exactly all MAOs", [](){
			for (int N = 4; N <= 8; N++){
				std::cout << "    " << "Busy with graphs that have " << N << " nodes." << std::endl;
				Graph P;
				emptyGraph(P, N);
				MaxAdjOrdering perms;
				ListPure<ListPure<node>> allPerms;
				perms.calcAll(&P,&allPerms);

				for (int i = 1; i < 10; i++){
					Graph G;
					randomSimpleGraph(G,N,1*N);

					//make an instance for the MAOs
					MaxAdjOrdering m;

					//make structures for saving all MAOs of G
					ListPure<ListPure<node>> MAOs;

					//calculate them
					m.calcAll(&G,&MAOs);

					AssertThat(m.testIfAllMAOs(&G,&MAOs,&allPerms), IsTrue());
				}
			}
		});
		it("should calculate MAOs with correct lex-bfs tie breaking", [](){
			for (int N = 10; N <= 20; N++){
				std::cout << "    " << "Busy with graphs that have " << N << " nodes." << std::endl;

				for (int i = 1; i < 10; i++){
					Graph G;
					randomSimpleGraph(G, N, (N*(N-4))/2);

					//make an instance for the MAOs
					MaxAdjOrdering m;

					//make structures for saving all MAOs of G
					ListPure<node> MAO;

					//calculate them
					m.calcBfs(&G,&MAO);

					AssertThat(m.testIfMAO(&G,&MAO), IsTrue());
					AssertThat(m.testIfMAOBfs(&G,&MAO), IsTrue());
				}
			}
		});
	});
});
