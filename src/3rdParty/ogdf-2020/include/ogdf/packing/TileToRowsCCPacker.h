/** \file
 * \brief Declaration of class TileToRowsCCPacker.
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

#pragma once

#include <ogdf/packing/CCLayoutPackModule.h>

namespace ogdf {


//! The tile-to-rows algorithm for packing drawings of connected components.
class OGDF_EXPORT TileToRowsCCPacker : public CCLayoutPackModule
{
	template<class POINT> struct RowInfo;

public:
	//! Creates an instance of tile-to-rows packer.
	TileToRowsCCPacker() { }

	virtual ~TileToRowsCCPacker() { }

	/**
	 * \brief Arranges the rectangles given by \p box.
	 *
	 * The algorithm call takes an input an array \p box of rectangles with
	 * real coordinates and computes in \p offset the offset to (0,0) of each
	 * rectangle in the layout.
	 * @param box is the array of input rectangles.
	 * @param offset is assigned the offset of each rectangle to the origin (0,0).
	 *        The offset of a rectangle is its lower left point in the layout.
	 * @param pageRatio is the desired page ratio (width / height) of the
	 *        resulting layout.
	 */
	virtual void call(Array<DPoint> &box,
		Array<DPoint> &offset,
		double pageRatio = 1.0) override;

	/**
	 * \brief Arranges the rectangles given by \p box.
	 *
	 * The algorithm call takes an input an array \p box of rectangles with
	 * real coordinates and computes in \p offset the offset to (0,0) of each
	 * rectangle in the layout.
	 * @param box is the array of input rectangles.
	 * @param offset is assigned the offset of each rectangle to the origin (0,0).
	 *        The offset of a rectangle is its lower left point in the layout.
	 * @param pageRatio is the desired page ratio (width / height) of the
	 *        resulting layout.
	 */
	virtual void call(Array<IPoint> &box,
		Array<IPoint> &offset,
		double pageRatio = 1.0) override;

private:
	template<class POINT>
	static void callGeneric(Array<POINT> &box,
		Array<POINT> &offset,
		double pageRatio);

	template<class POINT>
	static int findBestRow(Array<RowInfo<POINT> > &row,
		int nRows,
		double pageRatio,
		const POINT &rect);
};

}
