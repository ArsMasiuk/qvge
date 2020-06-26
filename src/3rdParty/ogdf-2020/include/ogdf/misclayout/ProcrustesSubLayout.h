/** \file
 * \brief Declares ogdf::ProcrustesSubLayout.
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

#include <ogdf/basic/LayoutModule.h>

namespace ogdf {

class ProcrustesPointSet
{
public:
	//! Constructor for allocating memory for \p numPoints points
	explicit ProcrustesPointSet(int numPoints);

	//! Destructor
	~ProcrustesPointSet();

	//! Translates and scales the set such that the average center is 0, 0 and the average size is 1.0.
	void normalize(bool flip = false);

	//! Rotates the point set so it fits somehow on \p other.
	void rotateTo(const ProcrustesPointSet& other);

	//! Calculates a value how good the two point sets match.
	double compare(const ProcrustesPointSet& other) const;

	//! Sets \p i'th coordinate.
	void set(int i, double x, double y)
	{
		m_x[i] = x;
		m_y[i] = y;
	}

	//! Returns \p i'th x-coordinate.
	double getX(int i) const
	{
		return m_x[i];
	}

	//! Returns \p i'th y-coordinate.
	double getY(int i) const
	{
		return m_y[i];
	}

	//! Returns the origin's x.
	double originX() const
	{
		return m_originX;
	}

	//! Returns the origin's y.
	double originY() const
	{
		return m_originY;
	}

	//! Returns the scale factor.
	double scale() const
	{
		return m_scale;
	}

	//! Returns the rotation angle.
	double angle() const
	{
		return m_angle;
	}

	//! Returns true if the point set is flipped by y coord.
	bool isFlipped() const
	{
		return m_flipped;
	}

private:
	//! Number of points.
	int m_numPoints;

	//! X coordinates.
	double* m_x;

	//! Y coordinates.
	double* m_y;

	//! Original average center's x when normalized.
	double m_originX;

	//! Original average center's y when normalized.
	double m_originY;

	//! Scale factor.
	double m_scale;

	//! If rotated, the angle.
	double m_angle;

	bool m_flipped;
};

//! Simple procrustes analysis.
class OGDF_EXPORT ProcrustesSubLayout : public LayoutModule
{
public:
	//! Constructor.
	explicit ProcrustesSubLayout(LayoutModule* pSubLayout);

	//! Destructor.
	virtual ~ProcrustesSubLayout() {
		delete m_pSubLayout;
	}

	virtual void call(GraphAttributes &GA) override;

	//! Should the new layout scale be used or the initial scale? Defaults to \c true.
	void setScaleToInitialLayout(bool flag)
	{
		m_scaleToInitialLayout = flag;
	}

	//! @copydoc #setScaleToInitialLayout
	bool scaleToInitialLayout() const
	{
		return m_scaleToInitialLayout;
	}

private:
	//! Does a reverse transform of graph attributes by using the origin, scale and angle in pointset.
	void reverseTransform(GraphAttributes& graphAttributes, const ProcrustesPointSet& pointSet);

	//! Moves all coords in graphAttributes by \p dx, \p dy.
	void translate(GraphAttributes& graphAttributes, double dx, double dy);

	//! Rotates all coords in graphAttributes by \p angle.
	void rotate(GraphAttributes& graphAttributes, double angle);

	//! Scales all coords in graphAttributes by \p scale.
	void scale(GraphAttributes& graphAttributes, double scale);

	//! Flips all y coordinates.
	void flipY(GraphAttributes& graphAttributes);

	//! Copies the coords in graph attributes to the point set.
	void copyFromGraphAttributes(const GraphAttributes& graphAttributes, ProcrustesPointSet& pointSet);

	//! Layout module to call for a new layout.
	LayoutModule* m_pSubLayout;

	//! Option for enabling/disabling scaling to initial layout scale.
	bool m_scaleToInitialLayout;
};

}
