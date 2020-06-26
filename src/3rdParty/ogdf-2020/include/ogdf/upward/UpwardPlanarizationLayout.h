/** \file
 * \brief Declaration of upward planarization layout algorithm.
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/basic/LayoutModule.h>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/upward/LayerBasedUPRLayout.h>
#include <ogdf/upward/SubgraphUpwardPlanarizer.h>

namespace ogdf {

class UpwardPlanarizationLayout : public LayoutModule
{
public:
	// constructor: sets options to default values
	UpwardPlanarizationLayout()
	{
		m_cr_nr = 0;
		// set default module
		m_layout.reset(new LayerBasedUPRLayout());
		m_UpwardPlanarizer.reset(new SubgraphUpwardPlanarizer());
	}

	// calls the algorithm for attributed graph GA
	// returns layout information in GA
	virtual void call(GraphAttributes &GA) override
	{
		if(GA.constGraph().numberOfNodes() > 2) {
			UpwardPlanRep UPR;
			UPR.createEmpty(GA.constGraph());
			m_UpwardPlanarizer->call(UPR);
			m_layout->call(UPR, GA);
			m_cr_nr = UPR.numberOfCrossings();
			m_numLevels = m_layout->numberOfLevels;
		}
	}

	// module option for the computation of the final layout
	void setUPRLayout(UPRLayoutModule *pLayout) {
		m_layout.reset(pLayout);
	}

	void setUpwardPlanarizer(UpwardPlanarizerModule *pUpwardPlanarizer) {
		m_UpwardPlanarizer.reset(pUpwardPlanarizer);
	}

	// returns the number of crossings in the layout after the algorithm
	// has been applied
	int numberOfCrossings() const { return m_cr_nr; }

	int numberOfLevels() const { return m_numLevels; }

protected:
	int m_cr_nr;
	int m_numLevels;
	std::unique_ptr<UpwardPlanarizerModule> m_UpwardPlanarizer;
	std::unique_ptr<UPRLayoutModule> m_layout;
};

}
