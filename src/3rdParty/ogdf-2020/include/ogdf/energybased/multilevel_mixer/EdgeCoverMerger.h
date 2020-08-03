/** \file
 * \brief Merges nodes with neighbour to get a Multilevel Graph
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

#include <ogdf/energybased/multilevel_mixer/MultilevelBuilder.h>

namespace ogdf {

//! The edge cover merger for multilevel layout.
/**
 * @ingroup gd-multi
 */
class OGDF_EXPORT EdgeCoverMerger : public MultilevelBuilder
{
private:
	double m_levelSizeFactor;
	NodeArray<node> m_substituteNodes;

	bool doMerge(MultilevelGraph &MLG, node parent, node mergePartner, int level);
	bool buildOneLevel(MultilevelGraph &MLG) override;

public:
	EdgeCoverMerger();
	void setFactor(double factor);
};

}
