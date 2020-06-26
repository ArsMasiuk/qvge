/** \file
 * \brief Declaration of class VariablEmbeddingInserterDynUML.
 *
 * \author Carsten Gutwenger, Jan Papenfuß
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

#include <ogdf/uml/UMLEdgeInsertionModule.h>
#include <ogdf/planarity/RemoveReinsertType.h>

namespace ogdf {

//! Optimal edge insertion module.
/**
 * The class VariableEmbeddingInserterDynUML represents the optimal edge insertion
 * algorithm, which inserts a single edge with a minum number of crossings
 * into a planar graph. This version is specialized for UML class diagrams and
 * makes sure that generalization edges do not cross.
 *
 * The implementation is based on the following publication:
 *
 * Carsten Gutwenger, Petra Mutzel, Rene Weiskircher: <i>Inserting an Edge into
 * a Planar %Graph</i>. Algorithmica 41(4), pp. 289-308, 2005.
 */
class OGDF_EXPORT VariableEmbeddingInserterDynUML : public UMLEdgeInsertionModule
{
public:
	//! Creates an instance of variable embedding edge inserter with default settings.
	VariableEmbeddingInserterDynUML();

	//! Creates an instance of variable embedding inserter with the same settings as \p inserter.
	VariableEmbeddingInserterDynUML(const VariableEmbeddingInserterDynUML &inserter);

	//! Destructor.
	~VariableEmbeddingInserterDynUML() { }

	//! Returns a new instance of the variable embedding inserter with the same option settings.
	virtual UMLEdgeInsertionModule *clone() const override;

	//! Assignment operator. Copies option settings only.
	VariableEmbeddingInserterDynUML &operator=(const VariableEmbeddingInserterDynUML &inserter);


	/**
	 *  @name Optional parameters
	 *  @{
	 */

	//! Sets the remove-reinsert postprocessing method.
	void removeReinsert(RemoveReinsertType rrOption) {
		m_rrOption = rrOption;
	}

	//! Returns the current setting of the remove-reinsert postprocessing method.
	RemoveReinsertType removeReinsert() const {
		return m_rrOption;
	}


	//! Sets the option <i>percentMostCrossed</i> to \p percent.
	/**
	 * This option determines the portion of most crossed edges used if the remove-reinsert
	 * method is set to RemoveReinsertType::MostCrossed. This portion is number of edges * percentMostCrossed() / 100.
	 */
	void percentMostCrossed(double percent) {
		m_percentMostCrossed = percent;
	}

	//! Returns the current setting of option percentMostCrossed.
	double percentMostCrossed() const {
		return m_percentMostCrossed;
	}

	//! @}

private:
	//! Implements the algorithm call.
	virtual ReturnType doCall(
		PlanRepLight              &pr,
		const Array<edge>         &origEdges,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<uint32_t> *pEdgeSubgraph) override;

	RemoveReinsertType m_rrOption; //!< The remove-reinsert method.
	double m_percentMostCrossed;   //!< The portion of most crossed edges considered.
};

}
