/** \file
 * \brief Declaration of BoothLueker which implements a planarity
 *        test and planar embedding algorithm.
 *
 * \author Sebastian Leipert, Markus Chimani
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

#pragma once

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/SList.h>
#include <ogdf/planarity/PlanarityModule.h>

namespace ogdf {

//! Booth-Lueker planarity test.
/**
 * @ingroup ga-planembed
 *
 * This class implements the linear-time planarity test proposed by Booth and Luecker, based on PQ-trees.\n
 * This class is deprecated! You will usually want to use the more modern/faster/versatile linear-time planarity test
 * by Boyer and Myrvold instead, implemented in the class BoyerMyrvold.
 * Generally, it is suggested to use the direct function calls isPlanar and planarEmbed in extended_graph_alg.h (which in turn use BoyerMyrvold).
 */
class OGDF_EXPORT BoothLueker : public PlanarityModule {

public:

	BoothLueker() { }
	~BoothLueker() { }

	//! Returns true, if G is planar, false otherwise.
	virtual bool isPlanarDestructive(Graph &G) override;

	//! Returns true, if G is planar, false otherwise.
	virtual bool isPlanar(const Graph &G) override;

	//! Returns true, if G is planar, false otherwise. If true, G contains a planar embedding.
	virtual bool planarEmbed(Graph &G) override {
		return preparation(G,true);
	}

	//! Returns true, if G is planar, false otherwise. If true, G contains a planar embedding.
	/**
	 * For BoothLueker, this procedure is exactly the same as planarEmbed. (See PlanarityModule or
	 * BoyerMyrvold for the rationale of this function's existence.
	 */
	virtual bool planarEmbedPlanarGraph(Graph &G) override {
		return preparation(G,true);
	}

private:

	//! Prepares the planarity test and the planar embedding
	bool preparation(Graph &G, bool embed);

	/**
	 * Performs a planarity test on a biconnected component of \p G.
	 *
	 * \p numbering contains an st-numbering of the component.
	 */
	bool doTest(Graph &G,NodeArray<int> &numbering);

	/**
	 * Performs a planarity test on a biconnected component of \p G and embedds it planar.
	 *
	 * \p numbering contains an st-numbering of the component.
	 */
	bool doEmbed(Graph &G,
		NodeArray<int>  &numbering,
		EdgeArray<edge> &backTableEdges,
		EdgeArray<edge> &forwardTableEdges);

	// Used by doEmbed. Computes an entire embedding from an
	// upward embedding.
	void entireEmbed(Graph &G,
		NodeArray<SListPure<adjEntry> > &entireEmbedding,
		NodeArray<SListIterator<adjEntry> > &adjMarker,
		NodeArray<bool> &mark,
		node v);

	void prepareParallelEdges(Graph &G);


	//private Members for handling parallel edges
	EdgeArray<ListPure<edge> > m_parallelEdges;
	EdgeArray<bool> m_isParallel;
	int	m_parallelCount;
};

}
