/** \file
 * \brief Splits and packs the components of a Graph.
 *
 * \author Gereon Bartel
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

#include <memory>
#include <ogdf/energybased/multilevel_mixer/MultilevelGraph.h>
#include <ogdf/packing/CCLayoutPackModule.h>
#include <ogdf/basic/LayoutModule.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/basic/GraphAttributes.h>
#include <vector>

namespace ogdf {

class OGDF_EXPORT ComponentSplitterLayout : public LayoutModule
{
private:
	std::unique_ptr<LayoutModule> m_secondaryLayout;
	std::unique_ptr<CCLayoutPackModule> m_packer;

	double m_targetRatio;
	int m_border;

	//! Combines drawings of connected components to
	//! a single drawing by rotating components and packing
	//! the result (optimizes area of axis-parallel rectangle).
	void reassembleDrawings(GraphAttributes &GA, const Array<List<node> > &nodesInCC);

public:
	ComponentSplitterLayout();

	void call(GraphAttributes &GA) override;

	void setLayoutModule(LayoutModule *layout) {
		m_secondaryLayout.reset(layout);
	}

	void setPacker(CCLayoutPackModule *packer) {
		m_packer.reset(packer);
	}
};

}
