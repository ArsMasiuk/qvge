/*!\file
 * \author Frank Baumann
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#ifdef _MSC_VER
// disable wrong warnings (VS compiler bug regarding virtual base classes)
#pragma warning(disable:4250)
#endif

#pragma once

#include <ogdf/lib/abacus/lpsub.h>
#include <ogdf/lib/abacus/osiif.h>

namespace abacus {


class Master;


class OGDF_EXPORT LpSubOsi : public LpSub, public OsiIF {
public:

	//! The constructor.
	/**
	 * Calls the function \a initialize() of the base
	 * classLpSub, which sets up the
	 * linear program and passes the data to the LP-solver.
	 *
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param sub    The subproblem of which the LP-relaxation is solved.
	 */
	LpSubOsi(Master *master, Sub *sub) :
		LP(master),
		LpSub(master, sub),
		OsiIF(master)
	{
		initialize();
	}

	//! The destructor.
	virtual ~LpSubOsi() { }

private:
	LpSubOsi(const LpSubOsi &rhs);
	const LpSubOsi &operator=(const LpSubOsi &rhs);
};

}
