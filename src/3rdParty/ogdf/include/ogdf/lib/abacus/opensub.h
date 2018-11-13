/*!\file
 * \author Matthias Elf
 * \brief open subproblems.
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

#pragma once

#include <ogdf/basic/List.h>
#include <ogdf/lib/abacus/abacusroot.h>

namespace abacus {


class Master;
class Sub;


//! Maintains open subproblems.
/**
 * During a branch-and-bound algorithm a set of open subproblems has to be maintained.
 * New subproblems are inserted in this set after a branching step, or when a subproblem
 * becomes dormant. A subproblem is extracted from this list if it becomes the active
 * subproblem which is optimized.
 */
class OGDF_EXPORT OpenSub : public AbacusRoot {

	friend class Sub;
	friend class Master;

public:

	//! Creates an empty list of open subproblems.
	/**
	 * Does not initialize the member \a dualBound_ since
	 * this can only be done if we know the sense of the objective function
	 * which is normally unknown when the constructor of the class Master
	 * is called which again calls this constructor.
	 *
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	OpenSub(Master *master) : master_(master) { }

	//! Returns the current number of open subproblems contained in this set.
	int number() const {
		return list_.size();
	}

	//! Returns true if there is no subproblem in the set of open subproblems, false otherwise.
	bool empty() const {
		return list_.empty();
	}

	//! Returns the value of the dual bound for all subproblems in the list.
	double dualBound() const;

private:

	//! Selects a subproblem according to the master's strategy and removes it from the list of open subproblems.
	/**
	 * This function scans the list of open subproblems and selects the subproblem with highest priority.
	 * Dormant subproblems are ignored if possible.
	 *
	 * \return The selected subproblem.
	 *         If the set of open subproblems is empty, 0 is returned.
	 */
	Sub *select();

	//! Adds a subproblem to the set of open subproblems.
	/**
	 * \param sub The subproblem that is inserted.
	 */
	void insert(Sub *sub);

	//! Removes subproblem from the set of open subproblems.
	/**
	 * \param sub The subproblem that is removed.
	 */
	void remove(Sub *sub) {
		if(list_.removeFirst(sub))
			updateDualBound();
	}

	//! Removes all elements from the set of opens subproblems.
	void prune() {
		list_.clear();
	}

	//! Updates \a dualBound_ according to the dual bounds of the subproblems contained in this set.
	void updateDualBound();

	Master *master_;		//!< A pointer to corresponding master of the optimization.
	ogdf::List<Sub*> list_;	//!< The list storing the open subproblems.
	double dualBound_;			//!< The dual bound of all open subproblems.

	OpenSub(const OpenSub &rhs);
	const OpenSub &operator=(const OpenSub &rhs);
};

}
