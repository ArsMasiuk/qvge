/** \file
 * \brief Declaration of class MFUPS, which implements
 *        the maximal feasible upward planar subgraph computation based on
 * 		  satisfiability (Chimani, Zeranski, 2012+)
 *
 * \author Robert Zeranski, Markus Chimani
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

#include <ogdf/upward/FUPSModule.h>

namespace ogdf {

class MaximalFUPS : public FUPSModule {
public:
	//constructor
	MaximalFUPS() : m_timelimit(0) {};

private:
	int m_timelimit;

protected:
	Module::ReturnType doCall(UpwardPlanRep &UPR, List<edge> &delEdges) override;

public:
#if 0
	int computeMFUPS(GraphCopy &GC);
#endif
	int getTimelimit()                 { return m_timelimit;      }
	void setTimelimit(int timelimit)   { m_timelimit = timelimit; }
};

}
