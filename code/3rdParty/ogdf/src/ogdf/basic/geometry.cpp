/** \file
 * \brief Implementation of Geometry classes like ogdf::DPoint, ogdf::DRect,
 * ogdf::DIntersectableRect, ogdf::DPolygon.
 *
 * \author Joachim Kupke
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


#include <ogdf/basic/geometry.h>


namespace ogdf {

const EpsilonTest OGDF_GEOM_ET(1.0e-6);

// output the rect
std::ostream &operator<<(std::ostream &os, const DRect &dr) {
	os << "\nLower left corner: " << dr.m_p1;
	os << "\nUpper right corner: " << dr.m_p2;
	os << "\nWidth: " << dr.width();
	os << "\nHeight: " << dr.height();
	return os;
}

double DRect::parallelDist(const DSegment &d1, const DSegment &d2) const {
	OGDF_ASSERT((d1.isHorizontal() && d2.isHorizontal()) ||
		(d1.isVertical() && d2.isVertical()));
	double d1min, d1max, d2min, d2max, paraDist, dist;
	if(d1.isVertical()) {
		d1min = d1.start().m_y;
		d1max = d1.end().m_y;
		d2min = d2.start().m_y;
		d2max = d2.end().m_y;
		paraDist = fabs(d1.start().m_x - d2.start().m_x);
	}
	else {
		d1min = d1.start().m_x;
		d1max = d1.end().m_x;
		d2min = d2.start().m_x;
		d2max = d2.end().m_x;
		paraDist = fabs(d1.start().m_y - d2.start().m_y);
	}
	if(d1min > d1max) std::swap(d1min, d1max);
	if(d2min > d2max) std::swap(d2min, d2max);
	if(d1min > d2max || d2min > d1max) { // no overlap
		dist = pointDist(d1.start(), d2.start());
		Math::updateMin(dist, pointDist(d1.start(), d2.end()));
		Math::updateMin(dist, pointDist(d1.end(), d2.start()));
		Math::updateMin(dist, pointDist(d1.end(), d2.end()));
	}
	else
		dist = paraDist; // segments overlap
	return dist;
}

// output the rect
std::ostream &operator<<(std::ostream &os, const DIntersectableRect &dr) {
	os << static_cast<DRect>(dr);
	os << "\nCenter: " << dr.center();
	os << "\nArea: " << dr.area();
	return os;
}

void DIntersectableRect::initAreaAndCenter() {
	m_area = (m_p2.m_x-m_p1.m_x)*(m_p2.m_y-m_p1.m_y);
	m_center.m_x = m_p1.m_x + 0.5*(m_p2.m_x-m_p1.m_x);
	m_center.m_y = m_p1.m_y + 0.5*(m_p2.m_y-m_p1.m_y);
}

void DIntersectableRect::move(const DPoint &point) {
	double dX = point.m_x - m_center.m_x;
	double dY = point.m_y - m_center.m_y;
	m_center = point;
	m_p1.m_x += dX;
	m_p1.m_y += dY;
	m_p2.m_x += dX;
	m_p2.m_y += dY;
}

double DIntersectableRect::distance(const DIntersectableRect &other) const {
	double dist = 0.0;
	if(!intersects(other)) {
		dist = parallelDist(top(), other.bottom());
		Math::updateMin(dist, parallelDist(left(), other.right()));
		Math::updateMin(dist, parallelDist(right(), other.left()));
		Math::updateMin(dist, parallelDist(bottom(), other.top()));
	}
	return dist;
}

bool DIntersectableRect::intersects(const DIntersectableRect &rectangle) const {
	bool intersect = false;
	if(contains(rectangle.m_center) || rectangle.contains(m_center)) intersect = true;
	else {
		DPoint p1(rectangle.m_p1.m_x, rectangle.m_p2.m_y);
		DPoint p2(rectangle.m_p2.m_x, rectangle.m_p1.m_y);
		intersect = contains(p1) || contains(p2) || contains(rectangle.m_p1) || contains(rectangle.m_p2);
	}
	return intersect;
}

DIntersectableRect DIntersectableRect::intersection(const DIntersectableRect &other) const {
	double top1    = m_p2.m_y;
	double bottom1 = m_p1.m_y;
	double left1   = m_p1.m_x;
	double right1  = m_p2.m_x;

	double top2    = other.m_p2.m_y;
	double bottom2 = other.m_p1.m_y;
	double left2   = other.m_p1.m_x;
	double right2  = other.m_p2.m_x;

	OGDF_ASSERT(top1 >= bottom1);
	OGDF_ASSERT(left1 <= right1);
	OGDF_ASSERT(top2 >= bottom2);
	OGDF_ASSERT(left2 <= right2);

	double bottomInter = max(bottom1,bottom2);
	double topInter    = min(top1,top2);
	double leftInter   = max(left1,left2);
	double rightInter  = min(right1,right2);

	if(bottomInter > topInter)   return DIntersectableRect();
	if(leftInter   > rightInter) return DIntersectableRect();

	return DIntersectableRect(DPoint(leftInter,bottomInter),DPoint(rightInter,topInter));
}

// gives the segment starting at point 'it'
DSegment DPolygon::segment(ListConstIterator<DPoint> it) const
{
	OGDF_ASSERT(!empty());
	OGDF_ASSERT(size() != 1);
	return DSegment(*it, *cyclicSucc(it));
}



// Assignment operator (for assigning from a rectangle).
DPolygon &DPolygon::operator=(const DRect &rect)
{
	clear();
	DRect  r1(rect);
	DRect  r2(rect);
	if (m_counterclock)
		r2.xInvert();
	else
		r2.yInvert();

	pushBack(r1.p1());
	pushBack(r2.p1());
	pushBack(r1.p2());
	pushBack(r2.p2());

	unify();
	return *this;
}


// inserts the point p, which must ly on the boarder of the polygon, between the two points p1 and p2
// returns the index to that point, which is inserted only once
ListIterator<DPoint> DPolygon::insertPoint(
	const DPoint &p,
	ListIterator<DPoint> p1,
	ListIterator<DPoint> p2)
{
	ListIterator<DPoint> i = p1;

	do {
		DSegment seg = segment(i);
		if (seg.contains(p)) {
			if (seg.start() == p)
				return i;
			else if (seg.end() == p) {
				i = cyclicSucc(i);
				return i;
			}
			else
				return insertAfter(p, i);
		}

		i = cyclicSucc(i);
	} while (i != p2);

	OGDF_ASSERT(false); // Point not in polygon, should not be reached!
	return i;
}


// inserts 'p' on every segment (a,b) with p in the open range ]a, b[
void DPolygon::insertCrossPoint(const DPoint &p)
{
	ListIterator<DPoint> i = begin();

	do {
		DSegment seg = segment(i);
		if (seg.contains(p)
		 && seg.start() != p
		 && seg.end() != p) {
			i = insertAfter(p, i);
		}

		i = cyclicSucc(i);
	} while (i != begin());
}


//
int DPolygon::getCrossPoints(const DPolygon &p, List<DPoint> &crossPoints) const
{
	crossPoints.clear();

	for (auto i = begin(); i.valid(); ++i) {
		DSegment s1 = segment(i);
		for (auto j = p.begin(); j.valid(); ++j) {
			DSegment s2 = p.segment(j);
			DPoint intersec;

			// TODO: What to do when IntersectionType::Overlapping is returned?
			if (s1.intersection(s2, intersec) == IntersectionType::SinglePoint)
				crossPoints.pushBack(intersec);
		}
	}
	// unify the list
	for (auto i = crossPoints.begin(); i.valid(); ++i) {
		for (auto j = i.succ(); j.valid(); ++j) {
			if (*i == *j) {
				--j;
				crossPoints.del(crossPoints.cyclicSucc(j));
			}
		}
	}

	return crossPoints.size();
}



// delete all consecutive double-points
void DPolygon::unify()
{
	ListIterator<DPoint> iter, next;
	for (iter = begin(); iter.valid(); ++iter) {
		next = cyclicSucc(iter);
		while (*iter == *next) {
			del(next);
			next = cyclicSucc(iter);
			if (iter == next)
				break;
		}
	}
}


// deletes all points, which are not facets
void DPolygon::normalize()
{
	unify();

	ListIterator<DPoint> iter, next;
	for (iter = begin(); iter.valid(); ++iter) {
		for( ; ; ) {
			next = cyclicSucc(iter);
			DSegment s1 = segment(iter);
			DSegment s2 = segment(next);
			DRect    r    (*iter, *cyclicSucc(next));
			if (s1.slope() == s2.slope() && r.contains(*next))
				del(next);
			else
				break; // while
		}
	}
}



// Checks wether a Point /a p is inside the Poylgon or not.
bool DPolygon::containsPoint(DPoint &p) const
{
	if (size() < 3) {
		return false;
	}

	double angle = 0.0;
	DPolygon::const_iterator i = cyclicPred(begin());
	double lastangle = atan2((*i).m_y - p.m_y, (*i).m_x - p.m_x);
	for (const DPoint &q : *this)
	{
		double tempangle = atan2(q.m_y - p.m_y, q.m_x - p.m_x);
		double step = lastangle - tempangle;
		while (step > Math::pi) step -= 2.0*Math::pi;
		while (step < -Math::pi) step += 2.0*Math::pi;
		angle += step;
		lastangle = tempangle;
	}

	double d = angle / (2.0 * Math::pi);
	int rounds = static_cast<int>(d<0?d-.5:d+.5);

	return (rounds % 2) != 0;
}


// outputs the polygon
std::ostream &operator<<(std::ostream &os, const DPolygon &dop)
{
	print(os, dop, ' ');
	return os;
}

int orientation(const DPoint &p, const DPoint &q, const DPoint &r)
{
	double d1 = (p.m_x - q.m_x) * (p.m_y - r.m_y);
	double d2 = (p.m_y - q.m_y) * (p.m_x - r.m_x);

	if(d1 == d2)
		return 0;
	else
		return (d1 > d2) ? +1 : -1;
}

}
