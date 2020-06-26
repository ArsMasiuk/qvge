/*!\file
 * \author Matthias Elf
 * \brief tailing off manager.
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

#include <ogdf/lib/abacus/ring.h>
#include <ogdf/lib/abacus/master.h>

namespace abacus {


//! Tailing off manager.
/**
 * During the cutting plane phase of the optimization of a single
 * subproblem it can be quite often observed that during the first
 * iterations a significant decrease of the optimum value of the
 * LP occurs, yet, this decrease becomes smaller and
 * smaller in later iterations. This effect is called
 * <i>tailing off</i> (see M. Padberg, G. Rinaldi, A branch-and-cut
 * algorithm for the resolution of large-scale symmetric traveling
 * salesman problems, SIAM Review 33, pp. 60-100).
 * Experimental results show that it might
 * be better to branch, although violated constraints can still
 * be generated, than to continue the cutting plane phase.
 *
 * This class stores the history of the values of the last
 * LP-solutions and implements all functions to control this
 * tailing-off effect.
 * The parameters are taken from the associated master.
 */
class  TailOff :  public AbacusRoot  {
	friend class Sub;
public:

	//! The constructor takes the length of the tailing off history from Master::tailOffNLp().
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	TailOff(Master *master) : master_(master)
	{
		if (master->tailOffNLp() > 0)
			lpHistory_ = new AbaRing<double>(master->tailOffNLp());
		else
			lpHistory_ = nullptr;
	}

	//! An alternative constructor takes the length of the tailing off history from the parameter NLp.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param NLp    The length of the tailing off history.
	 */
	TailOff(Master *master, int NLp) : master_(master)
	{
		if (NLp > 0)
			lpHistory_ = new AbaRing<double>(NLp);
		else
			lpHistory_ = nullptr;
	}

	//! The destructor.
	~TailOff() { delete lpHistory_; }


	//! The output operator
	/**
	 * Writes the memorized LP-values on an output stream.
	 *
	 * \param out The output stream.
	 * \param rhs The tailing-off manager  being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &out, const TailOff &rhs);

	//! Checks whether there is a tailing-off effect.
	/**
	 * We assume a tailing-off effect if during the last Master::tailOffNLps()
	 * iterations of the cutting plane algorithms
	 * the dual bound changed at most Master::tailOffPercent() percent.
	 *
	 * \return true if a tailing off effect is observed, false otherwise.
	 */
	virtual bool tailOff() const;

	//! Can be used to retrieve the difference between the last and a previous LP-solution in percent.
	/**
	 * \param nLps The number of LPs before the last solved linear program
	 *             with which the last solved LP-value should be compared.
	 * \param d    Contains the absolute difference bewteen the value of the
	 *             last solved
	 *             linear program and the value of the linear program solved
	 *             \a nLps before in percent relative to the older value.
	 *
	 * \return 0 if the difference could be computed, i.e., the old
	 *           LP-value \a nLPs before the last one is store in the history,
	 *           1 otherwise.
	 */
	int diff(int nLps, double &d) const;

protected:

	//! A new LP-solution value can be stored by calling the function \a update().
	/**
	 * This update should be performed after every solution
	 * of an LP in the cutting plane generation phase of the subproblem
	 * optimization process.
	 *
	 * \param value The LP-solution value.
	 */
	void update(double value) {
		if (lpHistory_)
			lpHistory_->insert(value);
	}


	//! Clears the solution history.
	/**
	 * This function
	 * should be called if variables are added, because
	 * normally the solution value of the LP-relaxation gets worse
	 * after the addition of variables. Such a
	 * change could falsely indicate a tailing-off effect if the
	 * history of LP-values is not reset.
	 */
	void reset() {
		if (lpHistory_)
			lpHistory_->clear();
	}


	//! A pointer to the corresponding master of the optimization.
	Master   *master_;

	//! The LP-values considered in the tailing off analysis.
	AbaRing<double> *lpHistory_;
};

}
