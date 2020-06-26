/*!\file
 * \author Matthias Elf
 * \brief lpsolution.
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

#include <ogdf/lib/abacus/abacusroot.h>

namespace abacus {


class Master;
class Sub;
template<class BaseType, class CoType> class Active;
template<class CoType,class  BaseType> class Separator;

template <class BaseType, class CoType>
class  LpSolution;

template <class BaseType, class CoType>
std::ostream &operator<< (std::ostream &out, const LpSolution<BaseType, CoType> &rhs);


//! LP solutions.
/**
 * This template class implements an LP solution. This class is
 * necessary when using the class Separator for separation.
 *
 * If constraints are to be generated in the separation then the \a BaseType
 * must be Variable and the \a CoType must be Constraint.
 * In this case an objects of that class stores the primal variables of
 * a linear program.
 * Otherwise, if variables are to be generated, then \a BaseType
 * must be Constraint and the \a CoType must be Variable.
 * In this case an objects of that class stores the dual variables of
 * a linear program.
 */
template <class BaseType, class CoType>
class  LpSolution :  public AbacusRoot  {

	friend class Separator< CoType, BaseType>;

public:

	//! The constructor.
	/**
	 * \param sub A pointer to the subproblem in which the LP solution is generated.
	 * \param primalVariables True if LpSolution contains the primal variables.
	 *	                      In this case \a BaseType must be Variable.
	 *	                      If \a primaVariables is false, then \a BaseType must be Constraint.
	 * \param active The active variables/constraints that are associated with the LP solution.
	 *	             The default argument is 0. Then the set of active
	 *               variables/constraints is not stored, but is assumed to be fixed and known.
	 */
	LpSolution(Sub *sub, bool primalVariables, Active<BaseType, CoType> *active);

	//! The constructor.
	/**
	 * \param master A pointer to Master.
	 */
	LpSolution(Master *master);

	//! The copy constructor.
	/**
	 * \param rhs The LP solution that is copied.
	 */
	LpSolution(const LpSolution<BaseType, CoType> &rhs);

	//! The destructor.
	~LpSolution();

	//! The output operator writes the lpsolution to an output stream.
	/**
	 * \param out The output stream.
	 * \param rhs The lpsolution being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<< <> (std::ostream &out, const LpSolution<BaseType, CoType> &rhs);

	//! Returns the number of variables (if \a BaseType is Variable) or the number of constraints (if \a BaseType is Constraint).
	int nVarCon() const;

	//! Returns the primal/dual variables of the LP solution.
	double *zVal();

	//! Returns the primal/dual variables of the LP solution.
	const double *zVal() const;

	//! Returns the active variables/constraints.
	Active<BaseType, CoType> *active();

	//! Returns the Id of the subproblem in which the LP solution was generated.
	int idSub() const;

	//! Returns the Id of the LP in which the LP solution was generated.
	int idLp() const;


protected:
	Master *master_; //!< A pointer to the corresponding master of the optimization.

	int nVarCon_; //!< The number of variables/constraints.
	int idSub_;   //!< The Id of the subproblem in which the LP solution was generated.
	int idLp_;    //!< The Id of the LP in which the LP solution was generated.

	Array<double>  zVal_; //!< The primal/dual variables of the LP solution.
	Active<BaseType, CoType> *active_; //!< The active variables/constraints.

private:
	const LpSolution<BaseType, CoType>
		&operator=(const LpSolution<BaseType, CoType> & rhs);
};


}

#include <ogdf/lib/abacus/lpsolution.inc>
