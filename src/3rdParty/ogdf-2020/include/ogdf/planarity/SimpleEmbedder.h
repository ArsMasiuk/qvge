/** \file
 * \brief A simple embedder algorithm.
 *
 * \author Thorsten Kerkhof (thorsten.kerkhof@udo.edu)
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

#include <ogdf/planarity/EmbedderModule.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/planarity/PlanRep.h>

namespace ogdf {

//! Embedder that chooses a largest face as the external one.
/**
 * Uses ogdf::planarEmbed() to compute an embedding.
 *
 * @ingroup ga-planarembed
 */
class OGDF_EXPORT SimpleEmbedder : public EmbedderModule
{
public:
	// construction / destruction
	SimpleEmbedder() { }
	~SimpleEmbedder() { }

	/**
	 * \brief Call embedder algorithm.
	 * \param G is the original graph. Its adjacency list is changed by the embedder.
	 * \param adjExternal is an adjacency entry on the external face and is set by the embedder.
	 */
	virtual void doCall(Graph& G, adjEntry& adjExternal) override;

private:
	/**
	 * \brief Find best suited external face according to certain criteria.
	 * \param PG is a planar representation of the original graph.
	 * \param E is a combinatorial embedding of the original graph.
	 * \return Best suited external face.
	 */
	face findBestExternalFace(const PlanRep& PG, const CombinatorialEmbedding& E);

};

}
