/** \file
 * \brief implements class VariableEmbeddingInserterDyn
 *
 * \author Carsten Gutwenger
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

#include <ogdf/planarity/VariableEmbeddingInserterDyn.h>
#include <ogdf/planarity/embedding_inserter/VarEdgeInserterDynCore.h>

namespace ogdf {

// clone method
EdgeInsertionModule *VariableEmbeddingInserterDyn::clone() const
{
	return new VariableEmbeddingInserterDyn(*this);
}

// actual call method
Module::ReturnType VariableEmbeddingInserterDyn::doCall(
	PlanRepLight &pr,
	const Array<edge> &origEdges,
	const EdgeArray<int> *pCostOrig,
	const EdgeArray<bool> *pForbiddenOrig,
	const EdgeArray<uint32_t> *pEdgeSubgraphs)
{
	VarEdgeInserterDynCore core(pr, pCostOrig, pForbiddenOrig, pEdgeSubgraphs);
	core.timeLimit(timeLimit());

	ReturnType retVal = core.call(origEdges, removeReinsert(), percentMostCrossed());
	runsPostprocessing(core.runsPostprocessing());
	return retVal;
}

}
