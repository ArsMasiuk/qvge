/** \file
 * \brief Declaration of classes GenericPoint, GenericPolyline, GenericLine,
 * GenericSegment, DPolygon, DRect, DIntersectableRect.
 *
 * \author Joachim Kupke, Rene Weiskircher, Ivo Hedtke
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

#include <ogdf/basic/List.h>
#include <ogdf/basic/Hashing.h>
#include <ogdf/basic/Math.h>
#include <ogdf/basic/EpsilonTest.h>
#include <cfloat>

namespace ogdf {

extern OGDF_EXPORT const EpsilonTest OGDF_GEOM_ET;

//! Determines the orientation in hierarchical layouts.
enum class Orientation {
	topToBottom, //!< Edges are oriented from top to bottom.
	bottomToTop, //!< Edges are oriented from bottom to top.
	leftToRight, //!< Edges are oriented from left to right.
	rightToLeft  //!< Edges are oriented from right to left.
};

//! Determines the type of intersection of two geometric objects.
enum class IntersectionType {
	None,        //!< Two geometric objects do not intersect.
	SinglePoint, //!< Two geometric objects intersect in a single point.
	Overlapping  //!< Two geometric objects intersect in infinitely many points.
};

/**
 * \brief Parameterized base class for points.
 *
 * This class serves as base class for two-dimensional points with specific
 * coordinate types like integer points (IPoint) and real points (DPoint).
 * The template parameter \a T is the type for the coordinates of the point
 * and has to support assignment and equality/inequality operators.
 *
 * This class also provides vector operations, e.g., computing an orthogonal
 * vector.
 */
template<typename T>
class GenericPoint
{
public:
	//! The type for coordinates of the point.
	using numberType = T;

	T m_x; //!< The x-coordinate.
	T m_y; //!< The y-coordinate.

	//! Creates a generic point (\p x,\p y).
	explicit GenericPoint(T x = 0, T y = 0) : m_x(x), m_y(y) { }

	//! Copy constructor.
	GenericPoint(const GenericPoint<T> &p) : m_x(p.m_x), m_y(p.m_y) { }

	//! Assignment operator.
	GenericPoint<T> &operator=(const GenericPoint<T> &p) {
		if (this != &p) {
			m_x = p.m_x;
			m_y = p.m_y;
		}
		return *this;
	}

	//! Equality operator.
	bool operator==(const GenericPoint<T> &dp) const {
		// OGDF_GEOM_ET uses different methods for integers and floats
		return OGDF_GEOM_ET.equal(m_x, dp.m_x) && OGDF_GEOM_ET.equal(m_y,dp.m_y);
	}

	//! Inequality operator.
	bool operator!=(const GenericPoint<T> &p) const {
		return !(*this == p);
	}

	//! Operator 'less'. Returns \c true iff the \a x coordinate of this is less than the \a x
	//! coordinate of \p p or, if they are equal, the same check is done for the \a y coordinate.
	bool operator<(const GenericPoint<T> &p) const {
		// OGDF_GEOM_ET uses different methods for integers and floats
		return OGDF_GEOM_ET.less(m_x, p.m_x)
		    || (OGDF_GEOM_ET.equal(m_x, p.m_x) && OGDF_GEOM_ET.less(m_y, p.m_y));
	}

	//! Operator 'greater'. Returns \c true iff \p p is less than this.
	bool operator>(const GenericPoint<T> &p) const {
		return p < *this;
	}

	//! Addition of points.
	GenericPoint<T> operator+(const GenericPoint<T>& p) const {
		return GenericPoint<T>(m_x + p.m_x, m_y + p.m_y);
	}

	//! Subtraction of points.
	GenericPoint<T> operator-(const GenericPoint<T>& p) const {
		return GenericPoint<T>(m_x - p.m_x, m_y - p.m_y);
	}

	//! Compute angle (in radians) between vectors
	double angle(GenericPoint<T> q, GenericPoint<T> r) const {
		const double dx1 = q.m_x - m_x, dy1 = q.m_y - m_y;
		const double dx2 = r.m_x - m_x, dy2 = r.m_y - m_y;

		// two vertices on the same place!
		if ((dx1 == 0 && dy1 == 0) || (dx2 == 0 && dy2 == 0)) {
			return 0.0;
		}

		double phi = std::atan2(dy2, dx2) - std::atan2(dy1, dx1);
		if (phi < 0) { phi += 2*Math::pi; }

		return phi;
	}

	//! Compute angle (in degrees) between vectors
	double angleDegrees(GenericPoint<T> q, GenericPoint<T> r) const {
		return Math::radiansToDegrees(angle(q, r));
	}

	//! Returns the %Euclidean distance between \p p and this point.
	double distance(const GenericPoint<T> &p) const {
		double dx = p.m_x - m_x;
		double dy = p.m_y - m_y;
		return sqrt( dx * dx + dy * dy );
	}

	//! Returns the norm of the point.
	double norm() const {
		return sqrt(m_x*m_x + m_y*m_y);
	}

	//! Adds \p p to this.
	GenericPoint<T> &operator+=(const GenericPoint<T> &p) {
		m_x += p.m_x;
		m_y += p.m_y;
		return *this;
	}

	//! Subtracts \p p from this.
	GenericPoint<T> &operator-=(const GenericPoint<T> &p) {
		m_x -= p.m_x;
		m_y -= p.m_y;
		return *this;
	}

	//! Point-wise multiplies this with \p c.
	GenericPoint<T> &operator*=(T c) {
		m_x *= c;
		m_y *= c;
		return *this;
	}

	//! Point-wise multiplies \p p with \p c.
	friend GenericPoint<T> operator*(T c, const GenericPoint<T> &p) {
		return GenericPoint<T>(c*p.m_x, c*p.m_y);
	}

	//! Point-wise multiplies \p p with \p c.
	friend GenericPoint<T> operator*(const GenericPoint<T> &p, T c) {
		return GenericPoint<T>(c*p.m_x, c*p.m_y);
	}

	//! Point-wise divide this by \p c.
	GenericPoint<T> &operator/=(T c) {
		m_x /= c;
		m_y /= c;
		return *this;
	}

	//! Point-wise divide \p p by \p c.
	friend GenericPoint<T> operator/(const GenericPoint<T> &p, double c) {
		return GenericPoint<T>(p.m_x/c, p.m_y/c);
	}

	//! Returns the determinant of the matrix (\c this, \p dv).
	T determinant(const GenericPoint<T> &dv) const {
		return (m_x * dv.m_y) - (m_y * dv.m_x);
	}

	//! Returns the scalar product of this and \p dv.
	T operator*(const GenericPoint<T> &dv) const {
		return (m_x * dv.m_x) + (m_y * dv.m_y);
	}

	/**
	 * Returns a vector that is orthogonal to this vector.
	 *
	 * Returns the vector \a (y/x,1) if \a x != 0, or \a (1,0)
	 * otherwise, where \a (x,y) is this vector.
	 */
	GenericPoint<T> orthogonal() const {
		GenericPoint<T> ret(1, 1);
		if (m_x != 0.0) {
			ret.m_x = - m_y / m_x;
		} else {
			ret.m_y = 0.0;
		}
		return ret;
	}
};

//! Output operator for generic points.
template<typename T>
std::ostream &operator<<(std::ostream &os, const GenericPoint<T>& p) {
	os << "(" << p.m_x << "," << p.m_y << ")";
	return os;
}

//! Representing a two-dimensional point with integer coordinates
using IPoint = GenericPoint<int>;

//! Representing two-dimensional point with real coordinates.
using DPoint = GenericPoint<double>;

template<> class DefHashFunc<IPoint>
{
public:
	int hash(const IPoint &ip) const {
		return 7*ip.m_x + 23*ip.m_y;
	}
};

/**
 * Polylines with \a PointType points.
 *
 * This class represents polylines by a list of \a PointType points.
 * Such polylines are, e.g., used in layouts for representing bend
 * point lists. Note that in this case, only the bend points are in the
 * list and neither the start nor the end point.
 */
template<class PointType>
class GenericPolyline : public List<PointType> {
public:
	//! Creates an empty polyline.
	GenericPolyline() { }

	//! Creates a polyline using the list of points \p pl.
	GenericPolyline(const List<PointType> &pl) : List<PointType>(pl) { }

	//! Copy constructor.
	GenericPolyline(const GenericPolyline<PointType> &pl) : List<PointType>(pl) { }

	//! Assignment operator.
	GenericPolyline<PointType> &operator=(const GenericPolyline &pl) {
		List<PointType>::operator =(pl);
		return *this;
	}

	//! Returns the %Euclidean length of the polyline.
	double length() const {
		OGDF_ASSERT(!this->empty());

		double len = 0.0;
		ListConstIterator<PointType> pred, iter;

		pred = iter = this->begin();
		++iter;

		while (iter.valid()) {
			len += (*iter).distance(*pred);
			++pred;
			++iter;
		}

		return len;
	}

	/**
	 * \brief Returns a point on the polyline which is \p fraction * \p len
	 *        away from the start point.
	 *
	 * @param fraction defines the fraction of \p len to be considered.
	 * @param len is the given length, or the length of the polyline if \p len < 0.
	 */
	DPoint position(const double fraction, double len = -1.0) const {
		OGDF_ASSERT( !this->empty() );
		OGDF_ASSERT( fraction >= 0.0 );
		OGDF_ASSERT( fraction <= 1.0 );
		if (len < 0.0) { len = length(); }
		OGDF_ASSERT( len >= 0.0 );

		DPoint p = *(this->begin());
		double liter = 0.0;
		double pos = len * fraction;
		double seglen = 0.0;
		ListConstIterator<PointType> pred, iter;

		pred = iter = this->begin();
		++iter;

		// search the segment, which contains the desired point
		double DX = 0, DY = 0; // for further use
		while (iter.valid()) {
			DX = (*iter).m_x - (*pred).m_x;
			DY = (*iter).m_y - (*pred).m_y;
			seglen = sqrt( DX*DX + DY*DY );
			liter += seglen;
			if (liter >= pos)
				break;
			++pred;
			++iter;
		}

		if (!iter.valid()) { // position not inside the polyline, return last point!
			p = *(this->rbegin());
		} else {
			if (seglen == 0.0) // *pred == *iter and pos is inbetween
				return *pred;

			double segpos = seglen + pos - liter;

			double dx = DX * segpos / seglen;
			double dy = DY * segpos / seglen;

			p = *pred;
			p.m_x += dx;
			p.m_y += dy;
		}

		return p;
	}

	//! Deletes all successive points with equal coordinates.
	void unify() {
		if (this->empty()) return;
		ListIterator<PointType> iter, next;
		for (iter = next = this->begin(), ++next; next.valid() && (this->size() > 2); ++next) {
			if (*iter == *next) {
				this->del(next);
				next = iter;
			} else
				iter = next;
		}
	}

protected:
	/**
	 * @copydoc ogdf::GenericPolyline<PointType>::normalize()
	 * @pre This polyline must be unified using
	 *      ogdf::GenericPolyline<PointType>::unify().
	 */
	void normalizeUnified(double minAngle) {
		OGDF_ASSERT(OGDF_GEOM_ET.geq(minAngle, 0.0));
		OGDF_ASSERT(OGDF_GEOM_ET.leq(minAngle, Math::pi));

		double maxAngle = 2*Math::pi - minAngle;
		ListIterator<PointType> iter = this->begin();
		ListIterator<PointType> next, onext;

		while (iter.valid()) {
			next  = iter; ++next;
			if (!next.valid())  { break; }
			onext = next; ++onext;
			if (!onext.valid()) { break; }
			double phi = (*next).angle(*iter, *onext);

			// Is *next on the way from *iter to *onext?
			if (OGDF_GEOM_ET.geq(phi, minAngle) &&
				OGDF_GEOM_ET.leq(phi, maxAngle)) {
				this->del(next);
				if (iter != this->begin()) {
					--iter;
				}
			} else {
				++iter;
			}
		}
	}

public:
	//! Deletes all redundant points on the polyline that lie on a (nearly)
	//! straight line given by their adjacent points.
	/**
	 * How straight the line has to be depends on \p minAngle. If this parameter
	 * is omitted, only points on completely straight lines are removed.
	 *
	 * In each iteration look at three points of the polyline, the middle one
	 * being a candidate for deletion. If it is deleted, advance in the polyline
	 * by one point. If it is not deleted, go back in the polyline by one point
	 * (the deletion might have lead to a greater angle between the points
	 * before and behind the deleted one). Repeat this process until the end of
	 * the polyline is reached.
	 *
	 * @param minAngle in [0..Pi] is a lower bound for the smaller angle between
	 * two line segments such that the Point between them is still removed.
	 */
	void normalize(double minAngle = Math::pi) {
		unify();
		normalizeUnified(minAngle);
	}

	/**
	 * @copydoc ogdf::GenericPolyline<PointType>::normalize
	 * @param src is used as a point that comes before all points in this polyline.
	 * @param tgt is used as a point that comes after all points in this polyline.
	 */
	void normalize(PointType src, PointType tgt, double minAngle = Math::pi) {
		unify();
		this->pushFront(src);
		this->pushBack(tgt);
		normalize(minAngle);
		this->popFront();
		this->popBack();
	}
};

//! Polylines with IPoint points.
using IPolyline = GenericPolyline<IPoint>;

//! Polylines with DPoint points.
using DPolyline = GenericPolyline<DPoint>;

//! Infinite lines.
template<class PointType>
class GenericLine {

public:
	using numberType = typename PointType::numberType;

protected:
	PointType m_p1; //!< The first point of the line.
	PointType m_p2; //!< The second point of the line.

	//! Returns the x-coordinate of the difference (second point - first point).
	numberType dx() const { return m_p2.m_x - m_p1.m_x; }

	//! Returns the y-coordinate of the difference (second point - first point).
	numberType dy() const { return m_p2.m_y - m_p1.m_y; }

public:
	//! Creates an empty line.
	GenericLine() : m_p1(), m_p2() {}

	//! Creates a line through the points \p p1 and \p p2.
	GenericLine(const PointType &p1, const PointType &p2) : m_p1(p1), m_p2(p2) {}

	//! Copy constructor.
	GenericLine(const GenericLine<PointType> &dl) : m_p1(dl.m_p1), m_p2(dl.m_p2) {}

	//! Creates a line through the points (\p x1,\p y1) and (\p x2,\p y2).
	GenericLine(numberType x1, numberType y1, numberType x2, numberType y2)
	: GenericLine(PointType(x1, y1), PointType(x2, y2)) {}

	//! Equality operator.
	bool operator==(const GenericLine<PointType> &dl) const {
		return isVertical() ? dl.isVertical() && m_p1.m_x == dl.m_p1.m_x :
		                      slope() == dl.slope() && yAbs() == dl.yAbs();
	}

	//! Inequality operator.
	bool operator!=(const GenericLine<PointType> &dl) const {
		return !(*this == dl);
	}

	//! Assignment operator.
	GenericLine<PointType> &operator=(const GenericLine<PointType> &dl) {
		if (this != &dl) { // don't assign myself
			m_p1 = dl.m_p1;
			m_p2 = dl.m_p2;
		}
		return *this;
	}

	//! Returns true iff this line runs vertically.
	bool isVertical() const { return OGDF_GEOM_ET.equal(dx(), 0.0); }

	//! Returns true iff this line runs horizontally.
	bool isHorizontal() const { return OGDF_GEOM_ET.equal(dy(), 0.0); }

	//! Returns the slope of the line.
	double slope() const {
		return isVertical() ? std::numeric_limits<double>::max() : dy()/dx();
	}

	//! Returns the value y' such that (0,y') lies on the unlimited
	//! straight-line defined by this line.
	double yAbs() const {
		return isVertical() ? std::numeric_limits<double>::max() : m_p1.m_y - (slope() * m_p1.m_x);
	}

	/**
	 * Determines if \p line is left or right of this line.
	 *
	 * @param line is the second line.
	 * \return a positive number if \p line is left of this line, and
	 *         a negative number if \p line is right of this line.
	 */
	double det(const GenericLine<PointType> &line) const {
		return dx() * line.dy() - dy() * line.dx();
	}

	/**
	 * Returns an IntersectionType specifying whether \p line and this line
	 * intersect.
	 *
	 * @param line is the second line.
	 * @param inter is assigned an intersection point if
	 * IntersectionType::SinglePoint or IntersectionType::Overlapping is returned.
	 */
	IntersectionType intersection(const GenericLine<PointType> &line, DPoint &inter) const {
		if (isVertical() && line.isVertical()) {
			inter = m_p1;
			return OGDF_GEOM_ET.equal(m_p1.m_x, line.m_p1.m_x) ?
			       IntersectionType::Overlapping : IntersectionType::None;
		} else if (isVertical()) {
			inter = DPoint(m_p1.m_x, line.slope() * m_p1.m_x + line.yAbs());
			return IntersectionType::SinglePoint;
		} else if (line.isVertical()) {
			inter = DPoint(line.m_p1.m_x, slope() * line.m_p1.m_x + yAbs());
			return IntersectionType::SinglePoint;
		} else if (OGDF_GEOM_ET.equal(slope(), line.slope())) {
			// For parallel lines only return true if the lines are equal.
			inter = m_p1;
			return OGDF_GEOM_ET.equal(yAbs(), line.yAbs()) ?
			       IntersectionType::Overlapping : IntersectionType::None;
		} else {
			double ix = (line.yAbs() - yAbs()) / (slope() - line.slope());
			inter = DPoint(ix, slope() * ix + yAbs());
			return IntersectionType::SinglePoint;
		}
	}

	//! Returns true iff \p p lies on this line.
	virtual bool contains(const DPoint &p) const {
		if (p == m_p1 || p == m_p2) {
			return true;
		}

		if (isVertical()) {
			return OGDF_GEOM_ET.equal(p.m_x, m_p1.m_x);
		}

		double dx2p = p.m_x - m_p1.m_x;
		double dy2p = p.m_y - m_p1.m_y;

		// dx() != 0.0 since this line is not vertical.
		if (dx2p == 0.0) {
			return false;
		}

		return OGDF_GEOM_ET.equal(slope(), (dy2p/dx2p));
	}


	/**
	 * \brief Computes the intersection of this line and the horizontal line through y = \p horAxis.
	 *
	 * @param horAxis defines the horizontal line.
	 * @param crossing is assigned the x-coordinate of the intersection point.
	 *
	 * \return the IntersectionType of the intersection between this line and \p horAxis.
	 */
	virtual IntersectionType horIntersection(const double horAxis, double &crossing) const {
		if (isHorizontal()) {
			crossing = 0.0;
			return m_p1.m_y == horAxis ? IntersectionType::Overlapping : IntersectionType::None;
		}
		crossing = (m_p1.m_x * (m_p2.m_y - horAxis) -
					m_p2.m_x * (m_p1.m_y - horAxis)   ) / dy();
		return IntersectionType::SinglePoint;
	}

	/**
	 * \brief Computes the intersection between this line and the vertical line through x = \p verAxis.
	 *
	 * @param verAxis defines the vertical line.
	 * @param crossing is assigned the y-coordinate of the intersection point.
	 *
	 * \return the IntersectionType of the intersection between this line and \p verAxis.
	 */
	virtual IntersectionType verIntersection(const double verAxis, double &crossing) const {
		if (isVertical()) {
			crossing = 0.0;
			return m_p1.m_x == verAxis ? IntersectionType::Overlapping : IntersectionType::None;
		}
		crossing = (m_p1.m_y * (m_p2.m_x - verAxis) -
		            m_p2.m_y * (m_p1.m_x - verAxis)   ) / dx();
		return IntersectionType::SinglePoint;
	}
};

//! Output operator for lines.
template<class PointType>
std::ostream &operator<<(std::ostream &os, const GenericLine<PointType> &line) {
	if (line.isVertical()) {
		os << "Line: vertical with x = " << line.m_p1.m_x ;
	} else {
		os << "Line: f(x) = " << line.slope() << "x + " << line.yAbs() ;
	}
	return os;
}

//! Lines with real coordinates.
using DLine = GenericLine<DPoint>;

//! Finite line segments.
template<class PointType>
class GenericSegment : public GenericLine<PointType> {

private:
	//! Returns whether \p p lies in the rectangle which has #m_p1 and #m_p2 as
	//! opposing corners.
	/**
	 * @param p is the point to be tested.
	 * @param includeBorders determines whether true is also returned when \p
	 * lies on the borders of the rectangle given by #m_p1 and #m_p2.
	 */
	bool inBoundingRect(const PointType &p, bool includeBorders = true) const {
		double minx = min(this->m_p1.m_x, this->m_p2.m_x);
		double miny = min(this->m_p1.m_y, this->m_p2.m_y);
		double maxx = max(this->m_p1.m_x, this->m_p2.m_x);
		double maxy = max(this->m_p1.m_y, this->m_p2.m_y);

		if (includeBorders) {
			return OGDF_GEOM_ET.geq(p.m_x, minx) &&
			       OGDF_GEOM_ET.leq(p.m_x, maxx) &&
			       OGDF_GEOM_ET.geq(p.m_y, miny) &&
			       OGDF_GEOM_ET.leq(p.m_y, maxy);
		} else {
			return OGDF_GEOM_ET.greater(p.m_x, minx) &&
			       OGDF_GEOM_ET.less   (p.m_x, maxx) &&
			       OGDF_GEOM_ET.greater(p.m_y, miny) &&
			       OGDF_GEOM_ET.less   (p.m_y, maxy);
		}
	}

public:

	//! Creates an empty line segment.
	GenericSegment() : GenericLine<PointType>() {}

	//! Creates a line segment from \p p1 to \p p2.
	GenericSegment(const PointType &p1, const PointType &p2)
	: GenericLine<PointType>(p1, p2) {}

	//! Creates a line segment defined by the start and end point of line \p dl.
	explicit GenericSegment(const GenericLine<PointType> &dl)
	: GenericLine<PointType>(dl) {}

	//! Creates a line segment from (\p x1,\p y1) to (\p x2,\p y2).
	GenericSegment(double x1, double y1, double x2, double y2)
	: GenericLine<PointType>(x1, y1, x2, y2) {}

	//! Copy constructor.
	GenericSegment(const GenericSegment<PointType> &ds) = default;

	//! Copy assignment operator.
	GenericSegment& operator=(const GenericSegment<PointType> &ds) = default;

	//! Equality operator.
	bool operator==(const GenericSegment<PointType> &dl) const {
		return this->m_p1 == dl.m_p1 && this->m_p2 == dl.m_p2;
	}

	//! Inequality operator.
	bool operator!=(const GenericSegment<PointType> &dl) const {
		return !(*this == dl);
	}

	//! Returns the start point of the line segment.
	const PointType &start() const { return this->m_p1; }

	//! Returns the end point of the line segment.
	const PointType &end() const { return this->m_p2; }

	//! Returns the x-coordinate of the difference (end point - start point).
	typename GenericLine<PointType>::numberType dx() const {
		return GenericLine<PointType>::dx();
	}

	//! Returns the y-coordinate of the difference (end point - start point).
	typename GenericLine<PointType>::numberType dy() const {
		return GenericLine<PointType>::dy();
	}

	/**
	 * Returns an IntersectionType specifying whether \p segment and this line
	 * segment intersect.
	 *
	 * @param segment is the second line segment.
	 * @param inter is assigned an intersection point if
	 * IntersectionType::SinglePoint or IntersectionType::Overlapping is returned.
	 * @param endpoints determines if common endpoints are treated as potential
	 * intersection points.
	 */
	IntersectionType intersection(const GenericSegment<PointType> &segment, PointType &inter, bool endpoints = true) const {
		IntersectionType lineIntersection = GenericLine<PointType>::intersection(segment, inter);

		if (lineIntersection == IntersectionType::None) {
			return IntersectionType::None;
		} else if (lineIntersection == IntersectionType::SinglePoint) {
			return inBoundingRect(inter, endpoints)
			    && segment.inBoundingRect(inter, endpoints) ?
				IntersectionType::SinglePoint : IntersectionType::None;
		} else {
			// Let inter be the second smallest point of this/the given segment.
			Array<DPoint> points({this->m_p1, this->m_p2, segment.m_p1, segment.m_p2});
			std::sort(points.begin(), points.end(), [](DPoint p1, DPoint p2) {
				return p1 < p2;
			});
			inter = points[1];

			if (!inBoundingRect(inter, endpoints) ||
				!segment.inBoundingRect(inter, endpoints)) {
				return IntersectionType::None;
			} else if (points[1] == points[2] &&
				!(this->m_p1 == inter && this->m_p2 == inter) &&
				!(segment.m_p1 == inter && segment.m_p2 == inter)) {
				// There is an intersection at a single point inter, which is
				// both an endpoint of this and an endpoint of the other segment.
				return IntersectionType::SinglePoint;
			} else {
				return IntersectionType::Overlapping;
			}
		}
	}

	//! Returns true iff \p p lies on this line segment.
	bool contains(const PointType &p) const override {
		return GenericLine<PointType>::contains(p) && inBoundingRect(p);
	}

	//! Returns the length (Euclidean distance between start and end point) of this line segment.
	double length() const {
		return this->m_p1.distance(this->m_p2);
	}

	/**
	 * \brief Computes the intersection of this line segment and the horizontal line through y = \p horAxis.
	 *
	 * @param horAxis defines the horizontal line.
	 * @param crossing is assigned the x-coordinate of the intersection point.
	 *
	 * \return the IntersectionType of the intersection between this line
	 *         segment and \p horAxis.
	 */
	IntersectionType horIntersection(const double horAxis, double &crossing) const override {
		IntersectionType result = GenericLine<PointType>::horIntersection(horAxis, crossing);
		if (result != IntersectionType::SinglePoint) {
			return result;
		} else if (inBoundingRect(DPoint(crossing, horAxis))) {
			return IntersectionType::SinglePoint;
		} else {
			crossing = 0.0;
			return IntersectionType::None;
		}
	}

	/**
	 * \brief Computes the intersection between this line segment and the vertical line through x = \p verAxis.
	 *
	 * @param verAxis defines the vertical line.
	 * @param crossing is assigned the y-coordinate of the intersection point.
	 *
	 * \return the IntersectionType of the intersection between this line
	 *         segment and \p verAxis.
	 */
	IntersectionType verIntersection(const double verAxis, double &crossing) const override {
		IntersectionType result = GenericLine<PointType>::verIntersection(verAxis, crossing);
		if (result != IntersectionType::SinglePoint) {
			return result;
		} else if (inBoundingRect(DPoint(verAxis, crossing))) {
			return IntersectionType::SinglePoint;
		} else {
			crossing = 0.0;
			return IntersectionType::None;
		}
	}
};

//! Output operator for line segments.
template<class PointType>
std::ostream &operator<<(std::ostream &os, const GenericSegment<PointType> &dl) {
	os << "Segment-Start: " << dl.start() << ", Segment-End: " << dl.end();
	return os;
}

//! Segments with real coordinates.
using DSegment = GenericSegment<DPoint>;

/**
 * \brief Rectangles with real coordinates.
 */
class OGDF_EXPORT DRect {

	friend OGDF_EXPORT std::ostream &operator<<(std::ostream &os, const DRect &dr);

protected:
	DPoint m_p1; //!< The lower left point of the rectangle.
	DPoint m_p2; //!< The upper right point of the rectangle.

public:
	//! Creates a rectangle with lower left and upper right point (0,0).
	DRect() = default;

	//! Creates a rectangle with lower left point \p p1 and upper right point \p p2.
	DRect(const DPoint &p1, const DPoint &p2)
	: m_p1(p1)
	, m_p2(p2)
	{ normalize(); }

	//! Copy constructor
	DRect(const DRect& dr)
	: m_p1(dr.m_p1)
	, m_p2(dr.m_p2) {}

	//! Creates a rectangle with lower left point (\p x1,\p y1) and upper right point (\p x2,\p y2).
	DRect(double x1, double y1, double x2, double y2)
	: DRect(DPoint(x1,y1), DPoint(x2,y2)) {}

	//! Creates a rectangle defined by the end points of line segment \p dl.
	explicit DRect(const DSegment &dl)
	: DRect(dl.start(), dl.end()) {}

	virtual ~DRect() = default;

	//! Equality operator: both rectangles have the same coordinates
	bool operator==(const DRect &dr) const {
		return m_p1 == dr.m_p1 && m_p2 == dr.m_p2;
	}

	//! Inequality operator.
	bool operator!=(const DRect &dr) const {
		return !(*this == dr);
	}

	//! Assignment operator.
	DRect &operator= (const DRect &dr) {
		if (this != &dr) { // don't assign myself
			m_p1 = dr.m_p1;
			m_p2 = dr.m_p2;
		}
		return *this;
	}

	//! Returns the width of the rectangle.
	double width() const {
		return m_p2.m_x - m_p1.m_x;
	}

	//! Returns the height of the rectangle.
	double height() const {
		return m_p2.m_y - m_p1.m_y;
	}

	/**
	 * \brief Normalizes the rectangle.
	 *
	 * Makes sure that the lower left point lies below and left of the upper
	 * right point.
	 */
	void normalize() {
		if (width() < 0) std::swap(m_p2.m_x, m_p1.m_x);
		if (height() < 0) std::swap(m_p2.m_y, m_p1.m_y);
	}

	//! Returns the lower left point of the rectangle.
	const DPoint &p1() const { return m_p1; }

	//! Returns the upper right point of the rectangle.
	const DPoint &p2() const { return m_p2; }

	//! Returns the top side of the rectangle.
	const DSegment top() const {
		return DSegment( DPoint(m_p1.m_x, m_p2.m_y), DPoint(m_p2.m_x, m_p2.m_y));
	}

	//! Returns the right side of the rectangle.
	const DSegment right() const {
		return DSegment( DPoint(m_p2.m_x, m_p2.m_y), DPoint(m_p2.m_x, m_p1.m_y));
	}

	//! Returns the left side of the rectangle.
	const DSegment left() const {
		return DSegment( DPoint(m_p1.m_x, m_p1.m_y), DPoint(m_p1.m_x, m_p2.m_y));
	}

	//! Returns the bottom side of the rectangle.
	const DSegment bottom() const {
		return DSegment( DPoint(m_p2.m_x, m_p1.m_y), DPoint(m_p1.m_x, m_p1.m_y));
	}

	//! Swaps the y-coordinates of the two points.
	void yInvert() { std::swap(m_p1.m_y, m_p2.m_y); }

	//! Swaps the x-coordinates of the two points.
	void xInvert() { std::swap(m_p1.m_x, m_p2.m_x); }

	//! Returns true iff \p p lies within this rectangle, modulo the comparison epsilon #OGDF_GEOM_ET.
	bool contains(const DPoint &p) const {
		return OGDF_GEOM_ET.geq(p.m_x, m_p1.m_x)
			&& OGDF_GEOM_ET.leq(p.m_x, m_p2.m_x)
			&& OGDF_GEOM_ET.geq(p.m_y, m_p1.m_y)
			&& OGDF_GEOM_ET.leq(p.m_y, m_p2.m_y);
	}

	//! Returns true iff \p segment intersects this DRect.
	bool intersection(const DSegment &segment) const {
		DPoint inter;
		return segment.intersection(top(),    inter) != IntersectionType::None
		    || segment.intersection(bottom(), inter) != IntersectionType::None
		    || segment.intersection(right(),  inter) != IntersectionType::None
		    || segment.intersection(left(),   inter) != IntersectionType::None;
	}

protected:
	//! Computes distance between parallel line segments.
	double parallelDist(const DSegment &d1, const DSegment &d2) const;

	//! Computes distance between two points.
	double pointDist(const DPoint &p1, const DPoint &p2) const {
		return sqrt((p1.m_y - p2.m_y) * (p1.m_y - p2.m_y) + (p1.m_x - p2.m_x) * (p1.m_x - p2.m_x));
	}
};

/**
 * \brief Rectangles with real coordinates.
 *
 * Stores its #m_area and #m_center and provides methods to compute the
 * intersection with other rectangles.
 */
class OGDF_EXPORT DIntersectableRect : public DRect {

	friend OGDF_EXPORT std::ostream &operator<<(std::ostream &os, const DIntersectableRect &dr);

	double m_area = 0.0;
	DPoint m_center;

	/**
	 * @copydoc ogdf::DRect::normalize()
	 * Update #m_area and #m_center.
	 */
	void initAreaAndCenter();

public:
	//! Creates a rectangle with lower left and upper right point (0,0).
	DIntersectableRect() = default;

	//! Creates a rectangle with lower left point \p p1 and upper right point \p p2.
	DIntersectableRect(const DPoint &p1, const DPoint &p2)
	: DRect(p1, p2)
	{ initAreaAndCenter(); }

	//! Creates a rectangle with lower left point (\p x1,\p y1) and upper right point (\p x1,\p y2).
	DIntersectableRect(double x1, double y1, double x2, double y2)
	: DIntersectableRect(DPoint(x1,y1), DPoint(x2,y2)) {}

	//! Copy constructor
	DIntersectableRect(const DIntersectableRect& dr)
	: DRect(static_cast<DRect>(dr))
	, m_area(dr.m_area)
	, m_center(dr.m_center) {}

	//! Constructs a rectangle from the \p center point, \p width and \p height
	DIntersectableRect(const DPoint &center, double width, double height)
	: DIntersectableRect(DPoint(center.m_x-width/2,center.m_y-height/2),
	                     DPoint(center.m_x+width/2,center.m_y+height/2)) {};

	//! Assignment operator.
	DIntersectableRect &operator= (const DIntersectableRect &dr) {
		if (this != &dr) { // don't assign myself
			m_p1 = dr.m_p1;
			m_p2 = dr.m_p2;
			m_center = dr.m_center;
			m_area = dr.m_area;
		}
		return *this;
	}

	//! Center of the rectangle
	DPoint center() const { return m_center; }

	//! Area of the rectangle
	double area() const { return m_area; }

	//! Tests if this and the argument \p rectangle intersect
	bool intersects(const DIntersectableRect &rectangle) const;

	//! Returns the rectangle resulting from intersection of this and \p other.
	//! Returns a rectangle with zero width and height and center (0,0) if intersection
	//! is empty.
	DIntersectableRect intersection(const DIntersectableRect &other) const;

	//! Computes distance between two rectangles.
	double distance(const DIntersectableRect &other) const;

	//! Moves the rectangle such that its center is at the given \p point
	void move(const DPoint &point);
};

/**
 * \brief Polygons with real coordinates.
 */
class OGDF_EXPORT DPolygon : public DPolyline {

protected:

	bool m_counterclock; //!< If true points are given in conter-clockwise order.

public:
	/**
	 * \brief Creates an empty polygon.
	 *
	 * @param cc determines in which order the points will be given; true means
	 *        counter-clockwise, false means clockwise.
	 */
	DPolygon(bool cc = true) : m_counterclock(cc) { }

	//! Creates a polgon from a rectangle.
	explicit DPolygon(const DRect &rect, bool cc = true) : m_counterclock(cc) {
		operator=(rect);
	}

	//! Copy constructor.
	DPolygon(const DPolygon &dop) : DPolyline(dop), m_counterclock(dop.m_counterclock) { }

	//! Returns true iff points are given in counter-clockwise order.
	bool counterclock() { return m_counterclock; }

	//! Assignment operator.
	DPolygon &operator=(const DPolygon &dop) {
		List<DPoint>::operator =(dop);
		m_counterclock = dop.m_counterclock;
		return *this;
	}

	//! Assignment operator (for assigning from a rectangle).
	DPolygon &operator=(const DRect &rect);

	//! Returns the line segment that starts at position \p it.
	DSegment segment(ListConstIterator<DPoint> it) const;


	//! Inserts point \p p, that must lie on a polygon segment.
	ListIterator<DPoint> insertPoint(const DPoint &p) {
		return insertPoint(p, begin(), begin());
	}

	/**
	 * \brief Inserts point \p p, but just searching from point \p p1 to \p p2.
	 *
	 * That is, from the segment starting at \p p1 to the segment ending at \p p2.
	 */
	ListIterator<DPoint> insertPoint(const DPoint &p,
		ListIterator<DPoint> p1,
		ListIterator<DPoint> p2);

	//! Inserts point p on every segment (a,b) with \p p in the open range ]a, b[.
	void insertCrossPoint(const DPoint &p);

	//! Returns the list of intersection points of this polygon with \p p.
	int getCrossPoints(const DPolygon &p, List<DPoint> &crossPoints) const;

	//! Deletes all consecutive points that are equal.
	void unify();

	//! Deletes all points, which are not facets.
	void normalize();

	/**
	 * \brief Checks wether a Point /a p is inside the Poylgon or not.
	 * \note Polygons with crossings have inner areas that count as outside!
	 * \par p the Point to check.
	 * return true if Point is inside.
	 */
	bool containsPoint(DPoint &p) const;
};

//! Output operator for polygons.
OGDF_EXPORT std::ostream &operator<<(std::ostream &os, const DPolygon &dop);

int orientation(const DPoint &p, const DPoint &q, const DPoint &r);


inline int orientation(const DSegment &s, const DPoint &p)
{
	return orientation(s.start(), s.end(), p);
}

}
