/** \file
 * \brief Declaration of class VariablEmbeddingInserterDyn.
 *
 * \author Carsten Gutwenger, Jan Papenfuß, Tilo Wiedera
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

#include <ogdf/planarity/VariableEmbeddingInserterBase.h>
#include <ogdf/planarity/RemoveReinsertType.h>


namespace ogdf {


//! Optimal edge insertion module.
/**
 * @ingroup ga-insert
 *
 * Optimal edge insertion algorithm that inserts a single edge with a minimum number of crossings
 * into a planar graph.
 *
 * In contrast to ogdf::VariableEmbeddingInserter, a ogdf::DynamicSPQRForest is used.
 *
 * The implementation is based on the following publication:
 *
 * Carsten Gutwenger, Petra Mutzel, Rene Weiskircher: <i>Inserting an Edge into
 * a Planar %Graph</i>. Algorithmica 41(4), pp. 289-308, 2005.
 */
class OGDF_EXPORT VariableEmbeddingInserterDyn : public VariableEmbeddingInserterBase
{
public:
	using VariableEmbeddingInserterBase::VariableEmbeddingInserterBase;

	//! Returns a new instance of the variable embedding inserter with the same option settings.
	virtual EdgeInsertionModule *clone() const override;

private:
	//! Implements the algorithm call.
	virtual ReturnType doCall(
		PlanRepLight              &pr,
		const Array<edge>         &origEdges,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<bool>     *pForbiddenOrig,
		const EdgeArray<uint32_t> *pEdgeSubgraphs) override;
};

}
