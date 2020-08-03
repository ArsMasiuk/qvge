/** \file
 * \brief Implements class MultilevelLayout
 *
 * \author Karsten Klein
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

#include <ogdf/energybased/MultilevelLayout.h>
#include <ogdf/energybased/SpringEmbedderGridVariant.h>

namespace ogdf {

//! Sets the single level layout
void MultilevelLayout::setLayout(LayoutModule* L)
{
	m_mixer->setLevelLayoutModule(L);
}


//! Sets the method used for coarsening
void MultilevelLayout::setMultilevelBuilder(MultilevelBuilder* B)
{
	m_mixer->setMultilevelBuilder(B);
}


//! Sets the placement method used when refining the levels again.
void MultilevelLayout::setPlacer(InitialPlacer* P)
{
	m_mixer->setInitialPlacer(P);
}


MultilevelLayout::MultilevelLayout()
{
	// For the layout, we set a scaling layout with
	// standard level layout FR. This scales the layout
	// on each level (with a constant factor) and then applies the FR.

	ScalingLayout* scaling = new ScalingLayout;
	scaling->setSecondaryLayout(new SpringEmbedderGridVariant);
	scaling->setScalingType(ScalingLayout::ScalingType::RelativeToDrawing);
	scaling->setLayoutRepeats(1);

	scaling->setScaling(1.0, 1.5);
	scaling->setExtraScalingSteps(2);

	m_mixer = new ModularMultilevelMixer;
	m_mixer->setLevelLayoutModule(scaling);
#if 0
	m_mixer->setLayoutRepeats(1);
	m_mixer->setAllEdgeLengths(5.0);
	m_mixer->setAllNodeSizes(1.0);
#endif

	ComponentSplitterLayout* splitter = new ComponentSplitterLayout;
	splitter->setLayoutModule(m_mixer);
	m_preproc.setLayoutModule(splitter);
	m_preproc.setRandomizePositions(true);
}


void MultilevelLayout::call(GraphAttributes &GA)
{
	MultilevelGraph MLG(GA);

	// Call the nested call, including preprocessing,
	// component splitting, scaling, level layout.
	m_preproc.call(MLG);

	MLG.exportAttributes(GA);
}

}
