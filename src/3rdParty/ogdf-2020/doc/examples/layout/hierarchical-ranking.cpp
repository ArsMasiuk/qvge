#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/layered/MedianHeuristic.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>
#include <ogdf/layered/SugiyamaLayout.h>

using namespace ogdf;

int r[] = {
	0, 1, 2, 3, 4, 5, 5, 6, 7, 8, 9, 9, 10, 10, 11, 12, 12,
	13, 14, 14, 15, 16, 17, 18, 18, 19, 19, 20, 21, 22, 22,
	22, 23, 23, 23, 23, 24, 25, 26, 27, 27, 27, 28, 29, 29,
	29, 30, 30, 31, 31, 31, 32, 33, 33, 34, 34, 35, 35, 35,
	35, 0, 1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 14, 15, 16, 18,
	19, 20, 21, 22, 23, 25, 27, 29, 30, 31, 32, 33, 34, 35, -1
};

int main()
{
	Graph G;
	GraphAttributes GA(G,
	  GraphAttributes::nodeGraphics |
	  GraphAttributes::edgeGraphics |
	  GraphAttributes::nodeLabel |
	  GraphAttributes::edgeStyle |
	  GraphAttributes::nodeStyle |
	  GraphAttributes::nodeTemplate);
	if (!GraphIO::read(GA, G, "unix-history-time.gml", GraphIO::readGML)) {
		std::cerr << "Could not load unix-history-time.gml" << std::endl;
		return 1;
	}

	NodeArray<int> rank(G);
	int i = 0;
	for(node v : G.nodes)
		rank[v] = r[i++];

	SugiyamaLayout SL;
	SL.setCrossMin(new MedianHeuristic);
	SL.arrangeCCs(false);

	OptimalHierarchyLayout *ohl = new OptimalHierarchyLayout;
	ohl->layerDistance(30.0);
	ohl->nodeDistance(25.0);
	ohl->weightBalancing(0.7);
	SL.setLayout(ohl);

	SL.call(GA, rank);
	GraphIO::write(GA, "output-unix-history-hierarchical-ranking.gml", GraphIO::writeGML);
	GraphIO::write(GA, "output-unix-history-hierarchical-ranking.svg", GraphIO::drawSVG);

	return 0;
}
