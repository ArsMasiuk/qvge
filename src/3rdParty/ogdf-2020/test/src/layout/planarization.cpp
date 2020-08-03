/** \file
 * \brief Tests for planarization layouts
 *
 * \author Carsten Gutwenger, Tilo Wiedera
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

#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/PlanarizationGridLayout.h>
#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <ogdf/planarity/FixedEmbeddingInserter.h>
#include <ogdf/planarlayout/MixedModelLayout.h>
#include <ogdf/planarlayout/MMCBLocalStretch.h>
#include <ogdf/upward/UpwardPlanarizationLayout.h>

#include "layout_helpers.h"

go_bandit([] { describe("Planarization layouts", [] {
	PlanarizationLayout pl, plFixed;
	PlanarizationGridLayout pgl, pglMM;

	VariableEmbeddingInserter *pVarInserter = new VariableEmbeddingInserter;
	FixedEmbeddingInserter *pFixInserter = new FixedEmbeddingInserter;

	SubgraphPlanarizer *pCrossMin = new SubgraphPlanarizer;
	pCrossMin->setSubgraph(new PlanarSubgraphFast<int>);
	pCrossMin->setInserter(pVarInserter);
	pCrossMin->permutations(4);

	pl.setCrossMin(pCrossMin->clone());
	pgl.setCrossMin(pCrossMin->clone());
	pglMM.setCrossMin(pCrossMin->clone());

	pCrossMin->setInserter(pFixInserter);
	pCrossMin->permutations(1);
	plFixed.setCrossMin(pCrossMin);

	MixedModelLayout *pMml = new MixedModelLayout;
	pMml->setCrossingsBeautifier(new MMCBLocalStretch);
	pglMM.setPlanarLayouter(pMml);

	GraphSizes smallSizes = GraphSizes(16, 48, 16);

	describeLayout("PlanarizationLayout", pl, GraphAttributes::edgeType | GraphAttributes::nodeType, {GraphProperty::simple, GraphProperty::sparse}, true, smallSizes);
	describeLayout("PlanarizationLayout with fixed inserter", plFixed, GraphAttributes::edgeType | GraphAttributes::nodeType, {GraphProperty::simple, GraphProperty::sparse}, true, smallSizes);

	describeLayout("PlanarizationGridLayout", pgl, 0, {GraphProperty::simple, GraphProperty::sparse}, true, smallSizes);
	describeLayout("PlanarizationGridLayout with mixed model", pglMM, 0, {GraphProperty::simple, GraphProperty::sparse}, true, smallSizes);

	UpwardPlanarizationLayout upl;
	describeLayout("UpwardPlanarizationLayout", upl, 0, {GraphProperty::acyclic, GraphProperty::simple, GraphProperty::sparse, GraphProperty::connected}, false, smallSizes);
}); });
