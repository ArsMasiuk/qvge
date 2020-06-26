/** \file
 * \brief Declaration of ogdf::SimpleCCPacker.
 *
 * \author Martin Gronemann
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
#include <ogdf/basic/LayoutModule.h>

namespace ogdf {

//! Splits and packs the components of a Graph.
/**
 * Simple proxy class that uses the TileToRowsCCPacker.
 * Use it for layouts that do not support disconnected graphs.
 */
class OGDF_EXPORT SimpleCCPacker : public LayoutModule
{
public:
	//! Constructor
	explicit SimpleCCPacker(LayoutModule* pSubLayoutModule = nullptr) : m_pSubLayoutModule(pSubLayoutModule)
	{
		m_leftMargin = 10.0;
		m_rightMargin = 10.0;
		m_bottomMargin = 10.0;
		m_topMargin = 10.0;
	}

	void setMargins(double left, double top, double right, double bottom)
	{
		m_leftMargin = left;
		m_rightMargin = right;
		m_bottomMargin = bottom;
		m_topMargin = top;
	}

	virtual void call(GraphAttributes& GA) override;

protected:
	double m_leftMargin;
	double m_rightMargin;
	double m_bottomMargin;
	double m_topMargin;

	std::unique_ptr<LayoutModule> m_pSubLayoutModule;

	void computeBoundingBox(const GraphAttributes& graphAttributes, DPoint& min_coord, DPoint& max_coord);
};

}
