/** \file
 * \brief Declaration of class VariablEmbeddingInserterBase.
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

#pragma once

#include <ogdf/planarity/EdgeInsertionModule.h>
#include <ogdf/planarity/RemoveReinsertType.h>

namespace ogdf {

//! Common parameter functionality for ogdf::VariableEmbeddingInserter and ogdf::VariableEmbeddingInserterDyn.
class OGDF_EXPORT VariableEmbeddingInserterBase : public EdgeInsertionModule
{
public:
	//! Creates an instance of variable embedding edge inserter with default settings.
	VariableEmbeddingInserterBase()
			: m_rrOption(RemoveReinsertType::None),
			  m_percentMostCrossed(25)
	{}

	//! Creates an instance of variable embedding inserter with the same settings as \p inserter.
	VariableEmbeddingInserterBase(const VariableEmbeddingInserterBase &inserter)
			: EdgeInsertionModule(inserter),
			  m_rrOption(inserter.m_rrOption),
			  m_percentMostCrossed(inserter.m_percentMostCrossed)
	{}

	//! Assignment operator. Copies option settings only.
	VariableEmbeddingInserterBase &operator=(const VariableEmbeddingInserterBase &inserter)
	{
		m_rrOption = inserter.m_rrOption;
		m_percentMostCrossed = inserter.m_percentMostCrossed;
		return *this;
	}

	//! Destructor.
	virtual ~VariableEmbeddingInserterBase() { }

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

	/** @}
	 *  @name Further information
	 *  @{
	 */

	//! Returns the number of runs performed by the remove-reinsert method after the algorithm has been called.
	int runsPostprocessing() const {
		return m_runsPostprocessing;
	}

protected:
	//! Sets the number of runs performed by the remove-reinsert method.
	void runsPostprocessing(int runs) {
		m_runsPostprocessing = runs;
	}

	//! @}

private:
	RemoveReinsertType m_rrOption; //!< The remove-reinsert method.
	double m_percentMostCrossed;   //!< The portion of most crossed edges considered.

	int m_runsPostprocessing; //!< Runs of remove-reinsert method.
};

}
