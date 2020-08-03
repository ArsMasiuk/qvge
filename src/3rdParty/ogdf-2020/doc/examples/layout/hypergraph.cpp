#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/hypergraph/HypergraphLayout.h>

using namespace ogdf;

int main()
{
	Hypergraph H;

	H.readBenchHypergraph("c17.bench");

	HypergraphAttributesES HA(H, EdgeStandardType::tree);
	HypergraphLayoutES hlES;

	hlES.setProfile(HypergraphLayoutES::Profile::Normal);
	hlES.call(HA);

	GraphIO::write(HA.repGA(), "output-c17.gml", GraphIO::writeGML);
	GraphIO::write(HA.repGA(), "output-c17.svg", GraphIO::drawSVG);

	return 0;
}
