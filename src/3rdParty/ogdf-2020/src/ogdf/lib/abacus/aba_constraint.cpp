/*!\file
 * \author Matthias Elf
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

#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/row.h>
#include <ogdf/lib/abacus/active.h>

namespace abacus {

int Constraint::genRow(Active<Variable, Constraint> *var,
						   Row &row) const
{
	double eps      = master_->machineEps();
	double minusEps = -eps;
	int    n        = var->number();

	expand();

	for (int e = 0; e < n; e++) {
		double c = coeff((*var)[e]);
		if (c > eps || c < minusEps) row.insert(e, c);
	}

	row.rhs(rhs());
	row.sense(sense_.sense());
	compress();
	return row.nnz();
}


double Constraint::slack(Active<Variable, Constraint> *variables,
							 double *x) const
{
	double eps      = master_->machineEps();
	double minusEps = -eps;
	double c;
	double lhs = 0.0;
	int    n   = variables->number();

	expand();

	for (int i = 0; i < n; i++) {
		double xi = x[i];
		if (xi > eps || xi < minusEps) {
			c = coeff((*variables)[i]);
			if (c > eps || c < minusEps)
				lhs += c * xi;
		}
	}

	compress();

	return rhs() - lhs;
}


bool Constraint::violated(Active<Variable, Constraint> *variables,
							  double *x,
							  double *sl) const
{
	double s = slack(variables, x);

	if (sl) *sl = s;

	return violated(s);
}


bool Constraint::violated(double slack) const
{
	switch (sense_.sense()) {
	case CSense::Equal:
		if (fabs(slack) > master_->eps()) return true;
		else                              return false;
	case CSense::Less:
		if (slack < -master_->eps()) return true;
		else                         return false;
	case CSense::Greater:
		if (slack > master_->eps()) return true;
		else                        return false;
	default:
		Logger::ifout() << "Constraint::violated(): unknown sense\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Constraint);
	}
}


InfeasCon::INFEAS Constraint::voidLhsViolated(double newRhs) const
{
	switch (sense_.sense()) {
	case CSense::Equal:
		if(newRhs > master_->eps())  return InfeasCon::TooLarge;
		if(newRhs < -master_->eps()) return InfeasCon::TooSmall;
		else                      return InfeasCon::Feasible;
	case CSense::Less:
		return newRhs < -master_->eps() ? InfeasCon::TooLarge : InfeasCon::Feasible;
	case CSense::Greater:
		return   newRhs > master_->eps() ? InfeasCon::TooSmall : InfeasCon::Feasible;
	default:
		Logger::ifout() << "Constraint::voidLhsViolated(): unknown sense\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Constraint);
	}
}


void Constraint::printRow(std::ostream &out,
							  Active<Variable, Constraint> *var) const
{
	Row row(master_, var->number());

	genRow(var, row);

	out << row;
}


double Constraint::distance(double *x,
								Active<Variable, Constraint> *actVar) const
{

	Row a(master_, actVar->number());

	int nnz = genRow(actVar, a);

	double ax = 0.0;

	for (int i = 0; i < nnz; i++)
		ax += a.coeff(i) * x[a.support(i)];

	return fabs((rhs() - ax)/a.norm());

}


ConClass *Constraint::classification(Active<Variable, Constraint> *var) const
{
	if (conClass_ == nullptr || var) {
		if (var == nullptr) {
			Logger::ifout() << "Constraint::classification(): Fatal error.\nNeither classification nor variable set specified.\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Constraint);
		}
		conClass_ = classify(var);
	}
	return conClass_;
}

}
