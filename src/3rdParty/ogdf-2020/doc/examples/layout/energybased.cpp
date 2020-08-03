#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/fileformats/GraphIO.h>

using namespace ogdf;

int main()
{
	Graph G;
	GraphAttributes GA(G);
	if (!GraphIO::read(G, "sierpinski_04.gml")) {
		std::cerr << "Could not load sierpinski_04.gml" << std::endl;
		return 1;
	}

	for (node v : G.nodes)
		GA.width(v) = GA.height(v) = 5.0;

	FMMMLayout fmmm;

	fmmm.useHighLevelOptions(true);
	fmmm.unitEdgeLength(15.0);
	fmmm.newInitialPlacement(true);
	fmmm.qualityVersusSpeed(FMMMOptions::QualityVsSpeed::GorgeousAndEfficient);

	fmmm.call(GA);
	GraphIO::write(GA, "output-energybased-sierpinski-layout.gml", GraphIO::writeGML);
	GraphIO::write(GA, "output-energybased-sierpinski-layout.svg", GraphIO::drawSVG);

	return 0;
}
