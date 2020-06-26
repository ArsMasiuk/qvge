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

#include <ogdf/lib/abacus/variable.h>
#include <ogdf/lib/abacus/column.h>

namespace abacus {


int Variable::genColumn(
	Active<Constraint, Variable> *actCon,
	Column &col) const
{
	double eps      = master_->machineEps();
	double minusEps = -eps;
	int    n        = actCon->number();

	expand();

	for (int i = 0; i < n; i++) {
		double co = (*actCon)[i]->coeff(this);
		if (co > eps || co < minusEps) col.insert(i,co);
	}

	col.obj(obj());
	col.lBound(lBound());
	col.uBound(uBound());

	compress();

	return col.nnz();

}


bool Variable::violated(double rc) const
{
	if (master_->optSense()->max()) {
		return rc > master_->eps();
	}
	else {
		return rc < -master_->eps();
	}
}


bool Variable::violated(
	Active<Constraint, Variable> *constraints,
	double *y,
	double *r) const
{
	double rc = redCost(constraints, y);

	if (r) *r = rc;

	return violated(rc);
}


double Variable::redCost(
	Active<Constraint, Variable> *actCon,
	double *y) const
{
	double eps = master_->machineEps();
	double minusEps = -eps;
	double rc = obj();
	int    n  = actCon->number();

	expand();

	for (int i = 0; i < n; i++) {
		double c = (*actCon)[i]->coeff(this);
		if (c > eps || c < minusEps)
			rc -= y[i] * c;
	}

	compress();

	return rc;
}


bool Variable::useful(
	Active<Constraint, Variable> *actCon,
	double *y,
	double lpVal) const
{
	if (!discrete()) return true;

	double rc = redCost(actCon, y);

	if (master_->optSense()->max())
		return (lpVal + rc > master_->primalBound());
	else
		return (lpVal + rc < master_->primalBound());
}


void Variable::printCol(
	std::ostream &out,
	Active<Constraint, Variable> *constraints) const
{
	Column col(master_, constraints->number());

	genColumn(constraints, col);

	out << col;
}

}
