/** \file
 * \brief Implements simple labeling algorithm
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


#include <ogdf/labeling/ELabelPosSimple.h>

namespace ogdf {


	ELabelPosSimple::ELabelPosSimple() :
m_absolut(true),
	m_marginDistance(0.2),
	m_edgeDistance(0.2),
	m_midOnEdge(true)
{ }


ELabelPosSimple::~ELabelPosSimple()
{ }


// returns the segment of the polyline containing the point
// that is fraction*bends.length() away from the start
static DSegment segment(DPolyline &bends, double fraction)
{
	double targetpos = bends.length()*fraction;
	double pos = 0.0;

	ListConstIterator<DPoint> iter, next;
	for (iter = next = bends.begin(), ++next; next.valid(); ++iter, ++next) {
		pos += (*iter).distance(*next);
		if (pos >= targetpos)
			return DSegment(*iter, *next);
	}

	return DSegment(*--iter, bends.back());
}


// liefert den Punkt neben dem Segment (links oder rechts, siehe 'left'), der orthogonal von
// 'p' die Entfernung 'newLen' von diesem Segment hat.
static DPoint leftOfSegment(const DSegment &segment, const DPoint &p, double newLen, bool left = true)
{
	DPoint v;
	if (p == segment.start())
		v = segment.end() - p;
	else
		v = p - segment.start();

	DPoint newPos(v.orthogonal());
	if (!left) newPos *= -1;

	// newPos has nonzero length
	newPos = (newPos * newLen) / newPos.norm();

	return p + newPos;
}


void ELabelPosSimple::call(GraphAttributes &ug, ELabelInterface<double> &eli)
{
	//ug.addNodeCenter2Bends();
	for(edge e : ug.constGraph().edges) {
		EdgeLabel<double> &el = eli.getLabel(e);
		DPolyline       bends = ug.bends(e);

		bends.normalize();

		if (bends.size() < 2)
			OGDF_THROW_PARAM(AlgorithmFailureException, AlgorithmFailureCode::Label);

		double frac;

		if (m_absolut) {
			double len = bends.length();
			if (len == 0.0)
				frac = 0.0;
			else
				frac = m_marginDistance / len;

		}
		else {
			frac = m_marginDistance;
		}

		if (frac < 0.0) frac = 0.0;
		if (frac > 0.4) frac = 0.4;

		double midFrac   = 0.5;
		double startFrac = frac;
		double endFrac   = 1.0 -frac;

		// hole Positionen auf der Kante
		DPoint midPoint   = bends.position(midFrac);
		DPoint startPoint = bends.position(startFrac);
		DPoint endPoint   = bends.position(endFrac);

		// hole die beteiligten Segmente
		DSegment midLine   = segment(bends, midFrac);
		DSegment startLine = segment(bends, startFrac);
		DSegment endLine   = segment(bends, endFrac);

		// berechne die Labelpositionen
		if (el.usedLabel(LabelType::End1)) {
			DPoint np = leftOfSegment(startLine, startPoint, m_edgeDistance, true);
			el.setX(LabelType::End1, np.m_x);
			el.setY(LabelType::End1, np.m_y);
		}

		if (el.usedLabel(LabelType::Mult1)) {
			DPoint np = leftOfSegment(startLine, startPoint, m_edgeDistance, false);
			el.setX(LabelType::Mult1, np.m_x);
			el.setY(LabelType::Mult1, np.m_y);
		}

		if (el.usedLabel(LabelType::Name)) {
			DPoint np = m_midOnEdge ? midPoint : leftOfSegment(midLine, midPoint, m_edgeDistance, true);
			el.setX(LabelType::Name, np.m_x);
			el.setY(LabelType::Name, np.m_y);
		}

		if (el.usedLabel(LabelType::End2)) {
			DPoint np = leftOfSegment(endLine, endPoint, m_edgeDistance, true);
			el.setX(LabelType::End2, np.m_x);
			el.setY(LabelType::End2, np.m_y);
		}

		if (el.usedLabel(LabelType::Mult2)) {
			DPoint np = leftOfSegment(endLine, endPoint, m_edgeDistance, false);
			el.setX(LabelType::Mult2, np.m_x);
			el.setY(LabelType::Mult2, np.m_y);
		}
	}
}

}
