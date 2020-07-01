/** \file
 * \brief implements class VariableEmbeddingInserterUML
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

#include <ogdf/uml/VariableEmbeddingInserterUML.h>
#include <ogdf/planarity/embedding_inserter/VarEdgeInserterCore.h>

namespace ogdf {

// constructor
// sets default values for options
VariableEmbeddingInserterUML::VariableEmbeddingInserterUML()
{
	m_rrOption = RemoveReinsertType::None;
	m_percentMostCrossed = 25;
}

// copy constructor
VariableEmbeddingInserterUML::VariableEmbeddingInserterUML(const VariableEmbeddingInserterUML &inserter)
	: UMLEdgeInsertionModule(inserter)
{
	m_rrOption = inserter.m_rrOption;
	m_percentMostCrossed = inserter.m_percentMostCrossed;
}

// clone method
UMLEdgeInsertionModule *VariableEmbeddingInserterUML::clone() const
{
	VariableEmbeddingInserterUML *pInserter = new VariableEmbeddingInserterUML;
	pInserter->m_rrOption = m_rrOption;
	pInserter->m_percentMostCrossed = m_percentMostCrossed;

	return pInserter;
}

// assignment operator
VariableEmbeddingInserterUML &VariableEmbeddingInserterUML::operator=(const VariableEmbeddingInserterUML &inserter)
{
	m_timeLimit = inserter.m_timeLimit;
	m_rrOption = inserter.m_rrOption;
	m_percentMostCrossed = inserter.m_percentMostCrossed;
	return *this;
}

// actual call method
Module::ReturnType VariableEmbeddingInserterUML::doCall(
	PlanRepLight              &pr,
	const Array<edge>         &origEdges,
	const EdgeArray<int>      *pCostOrig,
	const EdgeArray<uint32_t> *pEdgeSubgraph)
{
	VarEdgeInserterUMLCore core(pr, pCostOrig, pEdgeSubgraph);
	core.timeLimit(timeLimit());

	return core.call(origEdges, m_rrOption, m_percentMostCrossed);
}

}
