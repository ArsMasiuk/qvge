/** \file
 * \brief Implements the sweep line algorithm for computing an
 *        intersection graph.
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

#include <ogdf/basic/LayoutStatistics.h>
#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/basic/SortedSequence.h>
#include <unordered_map>
#include <memory>

using std::unordered_map;

namespace ogdf {

class DPointRep {

	friend class DPointHandle;

	static std::mutex s_mutexID;
	static uint64_t s_idCount;

	uint64_t m_id;
	double m_x;
	double m_y;

public:

	DPointRep(double x, double y) : m_x(x), m_y(y) {
		std::lock_guard<std::mutex> guard(s_mutexID);
		m_id = s_idCount++;
	}

	~DPointRep() = default;
};


std::mutex DPointRep::s_mutexID;
uint64_t   DPointRep::s_idCount = 0;


class DPointHandle {

	std::shared_ptr<DPointRep> m_pRep;

public:
	DPointHandle() : m_pRep(new DPointRep(0,0)) { }
	DPointHandle(double x, double y) : m_pRep(new DPointRep(x,y)) { }
	DPointHandle(const DPointHandle &p) : m_pRep(p.m_pRep) { }

	~DPointHandle() = default;

	DPointHandle &operator=(const DPointHandle &p) {
		m_pRep = p.m_pRep;
		return *this;
	}

	bool identical(const DPointHandle &p) const {
		return m_pRep == p.m_pRep;
	}

	uint64_t id() const { return m_pRep->m_id; }

	double xcoord() const { return m_pRep->m_x; }
	double ycoord() const { return m_pRep->m_y; }

	bool operator==(const DPointHandle &p) const {
		return m_pRep->m_x == p.m_pRep->m_x
		    && m_pRep->m_y == p.m_pRep->m_y;
	}

	bool operator!=(const DPointHandle &p) const {
		return !operator==(p);
	}

	bool operator<(const DPointHandle &p) const {
		return m_pRep->m_x < p.m_pRep->m_x
		    || (m_pRep->m_x == p.m_pRep->m_x && m_pRep->m_y < p.m_pRep->m_y);
	}

	bool operator>(const DPointHandle &p) const {
		return !operator<(p);
	}

};


int orientation(const DPointHandle &p, const DPointHandle &q, const DPointHandle &r)
{
	double d1 = (p.xcoord() - q.xcoord()) * (p.ycoord() - r.ycoord());
	double d2 = (p.ycoord() - q.ycoord()) * (p.xcoord() - r.xcoord());

	if(d1 == d2)
		return 0;
	else
		return (d1 > d2) ? +1 : -1;
}


class DSegmentRep {

	friend class DSegmentHandle;

	static std::mutex s_mutexID;
	static uint64_t s_idCount;

	uint64_t m_id;
	DPointHandle m_start;
	DPointHandle m_end;

public:

	DSegmentRep(const DPointHandle &p1, const DPointHandle &p2) : m_start(p1), m_end(p2) {
		std::lock_guard<std::mutex> guard(s_mutexID);
		m_id = s_idCount++;
	}

	~DSegmentRep() = default;
};


std::mutex DSegmentRep::s_mutexID;
uint64_t   DSegmentRep::s_idCount = 0;


class DSegmentHandle {

	std::shared_ptr<DSegmentRep> m_pRep;

public:
	DSegmentHandle() : m_pRep(new DSegmentRep(DPointHandle(),DPointHandle())) { }
	DSegmentHandle(const DPointHandle &p1, const DPointHandle &p2) : m_pRep(new DSegmentRep(p1,p2)) { }
	DSegmentHandle(double x1, double y1, double x2, double y2) : m_pRep(new DSegmentRep(DPointHandle(x1,y1),DPointHandle(x2,y2))) { }

	DSegmentHandle(const DSegmentHandle &seg) : m_pRep(seg.m_pRep) { }

	~DSegmentHandle() = default;

	DSegmentHandle &operator=(const DSegmentHandle &seg) {
		m_pRep = seg.m_pRep;
		return *this;
	}

	bool identical(const DSegmentHandle &seg) const {
		return m_pRep == seg.m_pRep;
	}

	uint64_t id() const { return m_pRep->m_id; }

	const DPointHandle &start() const { return m_pRep->m_start; }
	const DPointHandle &end() const { return m_pRep->m_end; }

	double dx() const { return end().xcoord() - start().xcoord(); }
	double dy() const { return end().ycoord() - start().ycoord(); }

	double slope() const { return (dx() == 0) ? std::numeric_limits<double>::max() : dy()/dx(); }
	double yAbs() const { return (dx() == 0) ? std::numeric_limits<double>::max() : start().ycoord() - (slope() * start().xcoord()); }

	bool isVertical() const { return start().xcoord() == end().xcoord(); }

	bool intersectionOfLines(const DSegmentHandle &line, DPointHandle &inter) const;

	bool operator==(const DSegmentHandle &seg) const {
		return start() == seg.start() && end() == seg.end();
	}

	bool operator!=(const DSegmentHandle &seg) const {
		return !operator==(seg);
	}
};


int orientation(const DSegmentHandle &seg, const DPointHandle &p) {
	return orientation(seg.start(), seg.end(), p);
}

bool DSegmentHandle::intersectionOfLines(const DSegmentHandle &line, DPointHandle &inter) const
{
	double ix, iy;

	// supporting lines are parallel?
	if (slope() == line.slope()) return false;

	if (start() == line.start() || start() == line.end()) {
		inter = start();
		return true;
	}

	if (end() == line.start() || end() == line.end()) {
		inter = end();
		return true;
	}

	// if the edge is vertical, we cannot compute the slope
	if (isVertical())
		ix = start().xcoord();
	else
		if (line.isVertical())
			ix = line.start().xcoord();
		else
			ix = (line.yAbs() - yAbs())/(slope() - line.slope());

	// set iy to the value of the infinite line at xvalue ix
	// use a non-vertical line (can't be both, otherwise they're parallel)
	if (isVertical())
		iy = line.slope() * ix + line.yAbs();
	else
		iy = slope() * ix + yAbs();

	inter = DPointHandle(ix, iy); // the supporting lines cross point
	return true;
}


class EventCmp {
public:
	int compare(const DPointHandle &p, const DPointHandle &q) const {
		if (p.xcoord() < q.xcoord()) return -1;
		else if (p.xcoord() > q.xcoord()) return  1;
		else if (p.ycoord() < q.ycoord()) return -1;
		else if (p.ycoord() > q.ycoord()) return  1;

		return 0;
	}

	OGDF_AUGMENT_COMPARER(DPointHandle)
};


class SweepCmpInternal {

	DPointHandle m_pSweep;

public:
	explicit SweepCmpInternal(const DPointHandle &p) : m_pSweep(p) { }

	void setPosition(const DPointHandle &p) {
		m_pSweep = p;
	}
	const DPointHandle &getPosition() const { return m_pSweep; }
};


class SweepCmp {

	SweepCmpInternal *m_pInternal;

public:
	explicit SweepCmp(SweepCmpInternal *pInternal) : m_pInternal(pInternal) { }

	int compare(const DSegmentHandle &s1, const DSegmentHandle &s2) const {
		if(s1.identical(s2)) return 0;

		const DPointHandle &pSweep = m_pInternal->getPosition();

		int s = 0;
		if(pSweep.identical(s1.start()))
			s = orientation(s2,pSweep);
		else if(pSweep.identical(s2.start()))
			s = -orientation(s1, pSweep);
		else
			throw "compare error in sweep";

		if(s != 0 || s1.start() == s1.end() || s2.start() == s2.end())
			return s;

		s = orientation(s2, s1.end());

		return (s != 0) ? s : (int)(s1.id() - s2.id());
		// TODO: distinguish overlapping segments by id
	}

	OGDF_AUGMENT_COMPARER(DSegmentHandle)
};


struct DSegmentHash {
	size_t operator()(const DSegmentHandle &s) const {
		return std::hash<double>()(s.start().xcoord()) ^ std::hash<double>()(s.start().ycoord())
			^ std::hash<double>()(s.end().xcoord()) ^ std::hash<double>()(s.end().ycoord());
	}
};


struct SeqItemY;
struct SeqItemXY;

using XSequence = SortedSequence<DPointHandle,SeqItemY,EventCmp>;
using YSequence = SortedSequence<DSegmentHandle,SeqItemXY,SweepCmp>;


struct SeqItemY {
	SeqItemY() : m_iterY(), m_origNode(nullptr)  { }
	SeqItemY(YSequence::iterator iterY) : m_iterY(iterY), m_origNode(nullptr) { }
	explicit SeqItemY(node vOrig) : m_iterY(), m_origNode(vOrig) { }
	YSequence::iterator m_iterY;
	node m_origNode;
};

struct SeqItemXY {
	SeqItemXY() { }
	SeqItemXY(XSequence::iterator iterX) : m_iterX(iterX) { }
	SeqItemXY(YSequence::iterator iterY) : m_iterY(iterY) { }

	XSequence::iterator m_iterX;
	YSequence::iterator m_iterY;
};


static void computeIntersection(XSequence &xStructure, YSequence &yStructure, YSequence::iterator sit0)
{
	YSequence::iterator sit1 = sit0.succ();
	DSegmentHandle s0 = sit0.key();
	DSegmentHandle s1 = sit1.key();

	if( orientation(s0, s1.end()) <= 0 && orientation(s1, s0.end()) >= 0 ) {
		DPointHandle q;
		s0.intersectionOfLines(s1, q);
		XSequence::iterator it = xStructure.lookup(q);
		if(it.valid()) {
			it.info().m_iterY = sit0;
			sit0.info() = it;
		} else
			sit0.info() = xStructure.insert(q,sit0);
	}
}


inline double maxAbs(double a, double b, double c, double d)
{
	a = max(fabs(a), fabs(b));
	c = max(fabs(c), fabs(d));
	return max(a,c);
}


static void addSegment(
	const DPointHandle &p,
	const DPointHandle &q,
	edge e,
	node vp,
	node vq,
	XSequence &xStructure,
	unordered_map<DSegmentHandle,edge,DSegmentHash> &original,
	List<DSegmentHandle> &internal,
	PrioritizedQueue<DSegmentHandle, DPointHandle> &segQueue,
	double &infinity)
{
	double val = maxAbs(p.xcoord(), p.ycoord(), q.xcoord(), q.ycoord());
	while(val >= infinity)
		infinity *= 2;

	XSequence::iterator it1 = xStructure.insert(p, SeqItemY(vp));
	XSequence::iterator it2 = xStructure.insert(q, SeqItemY(vq));

	if(it1 == it2) return;

	DPointHandle p1 = it1.key();
	DPointHandle p2 = it2.key();

	EventCmp eventCmp;
	DSegmentHandle s1 = (eventCmp.compare(p1,p2) < 0) ? DSegmentHandle(p1,p2) : DSegmentHandle(p2,p1);

	original[s1] = e;
	internal.pushBack(s1);
	segQueue.push(s1, s1.start());
}


void LayoutStatistics::intersectionGraph(const GraphAttributes &ga, Graph &H, NodeArray<DPoint> &points, NodeArray<node> &origNode, EdgeArray<edge> &origEdge)
{
	const Graph &g = ga.constGraph();

	// local declarations
	DPointHandle pSweep;
	EventCmp eventCmp;
	SweepCmpInternal sweepCmpInternal(pSweep);
	SweepCmp sweepCmp(&sweepCmpInternal);

	XSequence xStructure(eventCmp);
	YSequence yStructure(sweepCmp);

	List<DSegmentHandle> internal;
	unordered_map<DSegmentHandle,edge,DSegmentHash> original;

	unordered_map<DSegmentHandle,node,DSegmentHash> lastNode;
	PrioritizedQueue<DSegmentHandle, DPointHandle> segQueue;


	// initialization
	H.clear();
	points.init(H);
	origNode.init(H,nullptr);
	origEdge.init(H,nullptr);

	if(g.numberOfEdges() == 0)
		return;

	double infinity = 1;

	for(edge e : g.edges) {
		const DPolyline &dpl = ga.bends(e);
		if(dpl.empty()) {
			double x1 = ga.x(e->source()), y1 = ga.y(e->source());
			double x2 = ga.x(e->target()), y2 = ga.y(e->target());

			DPointHandle p(x1,y1);
			DPointHandle q(x2,y2);

			addSegment(p, q, e, e->source(), e->target(), xStructure, original, internal, segQueue, infinity);

		} else {
			// TODO: Improve to use same point reps in segments(?)
			ListConstIterator<DPoint> it = dpl.begin();
			ListConstIterator<DPoint> itSucc = it.succ();
			for( ; itSucc.valid(); ++it, ++itSucc) {
				node vp = it == dpl.begin() ? e->source() : nullptr;
				node vq = !itSucc.succ().valid() ? e->target() : nullptr;

				DPointHandle p((*it).m_x,(*it).m_y);
				DPointHandle q((*itSucc).m_x,(*itSucc).m_y);
				addSegment(p, q, e, vp, vq, xStructure, original, internal, segQueue, infinity);
			}
		}

#if 0
		double x1 = ga.x(e->source()), y1 = ga.y(e->source());
		double x2 = ga.x(e->target()), y2 = ga.y(e->target());

		DPoint p = DPoint(x1,y1);
		DPoint q = DPoint(x2,y2);

		x1 = abs(x1);
		Math::updateMax(x1, abs(x2));
		Math::updateMax(x1, abs(y1));
		Math::updateMax(x1, abs(y2));

		while(x1 >= infinity)
			infinity *= 2;

		XSequence::iterator it1 = xStructure.insert(p, SeqItemY(e->source()));
		XSequence::iterator it2 = xStructure.insert(q, SeqItemY(e->target()));

		if(it1 == it2) continue;

		DSegment s1 = (eventCmp.compare(p,q) < 0) ? DSegment(p,q) : DSegment(q,p);

		original[s1] = e;
		internal.pushBack(s1);
		segQueue.insert(s1, s1.start());
#endif
	}

	DSegmentHandle lowerSentinel(-infinity, -infinity, infinity, -infinity);
	DSegmentHandle upperSentinel(-infinity,  infinity, infinity,  infinity);

	pSweep = lowerSentinel.start();
	sweepCmpInternal.setPosition(pSweep);

	yStructure.insert(upperSentinel, SeqItemXY());
	yStructure.insert(lowerSentinel, SeqItemXY());

	DPointHandle pStop (infinity,infinity);
	segQueue.push(DSegmentHandle(pStop,pStop), pStop);
	DSegmentHandle nextSegment = segQueue.topElement();

	// sweep
	while(!xStructure.empty())
	{
		// extract next event from the X-structure
		XSequence::iterator eventIter = xStructure.begin();
		pSweep = eventIter.key();
		sweepCmpInternal.setPosition(pSweep);

		node v = H.newNode();
		points[v].m_x = pSweep.xcoord();
		points[v].m_y = pSweep.ycoord();
		origNode[v] = eventIter.info().m_origNode;

		// handle passing and ending segments
		YSequence::iterator sit = eventIter.info().m_iterY;
		if(!sit.valid())
			sit = yStructure.lookup(DSegmentHandle(pSweep,pSweep));

		YSequence::iterator sitSucc;
		YSequence::iterator sitPred;
		YSequence::iterator sitPredSucc;
		YSequence::iterator sitFirst;

		if(sit.valid())
		{
			// determine passing and ending segments

			// walk up
			while(sit.info().m_iterX == eventIter || sit.info().m_iterY == sit.succ() )
				sit = sit.succ();

			sitSucc = sit.succ();

			// walk down
			bool overlapping;
			do {
				overlapping = false;
				DSegmentHandle s = sit.key();
				edge eOrig = original[s];

				edge e = ( s.start() == DPointHandle(ga.x(eOrig->source()),ga.y(eOrig->source())) ) ? H.newEdge(lastNode[s], v) :  H.newEdge(v, lastNode[s]);
				origEdge[e] = eOrig;

				if( pSweep.identical(s.end()) )  // ending segment
				{
					YSequence::iterator it = sit.pred();
					if(it.info().m_iterY == sit) {
						overlapping = true;
						it.info() = sit.info();
					}
					yStructure.delItem(sit);
					sit = it;

				} else {  // passing segment
					if( sit.info().m_iterY != sit.succ() )
						sit.info() = SeqItemXY();

					lastNode[s] = v;
					sit = sit.pred();
				}

			} while(sit.info().m_iterX == eventIter || overlapping || sit.info().m_iterY == sit.succ() );

			sitPred = sit;
			sitFirst = sitPred.succ();
			sitPredSucc = sitFirst;

			// reverse order of passing segments
			sit = sitFirst;

			// reverse subsequence of overlapping segments (if existing)
			while( sit != sitSucc )
			{
				YSequence::iterator subFirst = sit;
				YSequence::iterator subLast = subFirst;

				while( subLast.info().m_iterY == subLast.succ() )
					subLast = subLast.succ();

				if( subLast != subFirst )
					yStructure.reverseItems(subFirst, subLast);

				sit = subFirst.succ();
			}

			// reverse the entire bundle
			if( sitFirst != sitSucc )
				yStructure.reverseItems(sitPred.succ(), sitSucc.pred());
		}

		// insert starting segments
		while( pSweep.identical(nextSegment.start()) )
		{
			YSequence::iterator s_sit = yStructure.locate(nextSegment);
			YSequence::iterator p_sit = s_sit.pred();

			DSegmentHandle s = s_sit.key();

			if(orientation(s, nextSegment.start()) == 0 && orientation(s, nextSegment.end()) == 0)
				sit = yStructure.insert(nextSegment, s_sit);  // insertAt
			else
				sit = yStructure.insert(nextSegment, SeqItemXY());

			s = p_sit.key();

			if(orientation(s, nextSegment.start()) == 0 && orientation(s, nextSegment.end()) == 0)
				p_sit.info() = sit;

			xStructure.lookup(nextSegment.end()).info().m_iterY = sit;
			lastNode[nextSegment] = v;

			if(!sitSucc.valid()) {
				sitSucc = s_sit;
				sitPred = p_sit;
				sitPredSucc = sitSucc;
			}

			// delete minimum and assign new minimum to nextSegment
			segQueue.pop();
			nextSegment = segQueue.topElement();
		}

		// compute new intersections and update X-structure
		if( sitPred.valid() )
		{
			sitPred.info() = SeqItemXY();
			computeIntersection(xStructure, yStructure, sitPred);
			sit = sitSucc.pred();
			if( sit != sitPred )
				computeIntersection(xStructure, yStructure, sit);
		}

		xStructure.delItem(eventIter);
	}
}

}
