/** \file
 * \brief Declaration of class MultilevelLayout which realizes a
 * wrapper for the multilevel layout computation using the
 * Modular Multilevel Mixer.
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

#pragma once

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/energybased/multilevel_mixer/ModularMultilevelMixer.h>
#include <ogdf/energybased/multilevel_mixer/InitialPlacer.h>
#include <ogdf/energybased/multilevel_mixer/ScalingLayout.h>
#include <ogdf/packing/ComponentSplitterLayout.h>
#include <ogdf/basic/PreprocessorLayout.h>

namespace ogdf {

//! The multilevel drawing framework.
/**
 * @ingroup gd-energy
 */
class OGDF_EXPORT MultilevelLayout : public LayoutModule
{
public:
	//! Constructor
	MultilevelLayout();

	//! Calculates a drawing for the Graph GA.
	virtual void call(GraphAttributes &GA) override;

	//Setting of the three main phases' methods
	//! Sets the single level layout
	void setLayout(LayoutModule* L);
	//! Sets the method used for coarsening
	void setMultilevelBuilder(MultilevelBuilder* B);
	//! Sets the placement method used when refining the levels again.
	void setPlacer(InitialPlacer* P);

private:
	ModularMultilevelMixer* m_mixer;
	PreprocessorLayout m_preproc;
};

}
