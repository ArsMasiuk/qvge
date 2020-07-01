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

#pragma once

#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/lpsolution.h>
#include <ogdf/lib/abacus/active.h>
#include <ogdf/lib/abacus/sub.h>

namespace abacus {


template <class BaseType, class CoType>
LpSolution<BaseType, CoType>::LpSolution(
	Sub *sub,
	bool primalVariables, Active<BaseType, CoType> *active )
	:
	master_(sub->master_),
	nVarCon_(primalVariables ? sub->nVar() : sub->nCon()),
	idSub_(sub->id_),
	idLp_(sub->nIter_),
	zVal_(master_, nVarCon_)
{
	if(active == 0)
		active_ = 0;
	else
		active_ = new Active<BaseType, CoType>(*active);

	double *p;
	if(primalVariables==true)
		p = sub->xVal_;
	else
		p = sub->yVal_;

	for(int i = 0; i < nVarCon_; i++)
		zVal_[i] = p[i];
}


template <class BaseType, class CoType>
LpSolution<BaseType, CoType>::LpSolution(Master *master)
	:
	master_(master),
	nVarCon_(0),
	idSub_(0),
	idLp_(0),
	zVal_(master_, 0)
{
	active_=0;
}


template <class BaseType, class CoType>
LpSolution<BaseType, CoType>::LpSolution(const LpSolution<BaseType, CoType> &rhs)
	:
	master_(rhs.master_),
	nVarCon_(rhs.nVarCon_),
	idSub_(rhs.idSub_),
	idLp_(rhs.idLp_),
	zVal_(rhs.zVal_)
{
	if(rhs.active_)
		active_=new Active<BaseType,CoType>(*rhs.active_);
}


template <class BaseType, class CoType>
LpSolution<BaseType, CoType>::~LpSolution()
{
	delete active_;
}


template <class BaseType, class CoType>
std::ostream &operator<<(std::ostream &os, const LpSolution<BaseType, CoType> &rhs)
{
	double machEps=rhs.master_->machineEps();
	os << rhs.idSub_ << " ";
	os << rhs.idLp_ << " ";
	os << rhs.nVarCon_ << " ";
	for(int i = 0; i < rhs.nVarCon_; i++)
		if(fabs(rhs.zVal_[i]) < machEps)
			os << "0.0 ";
		else
			os << rhs.zVal_[i] << " " ;
	os << std::endl;
	return os;
}


template <class BaseType, class CoType>
int LpSolution<BaseType, CoType>::nVarCon() const
{
	return nVarCon_;
}


template <class BaseType, class CoType>
double* LpSolution<BaseType, CoType>::zVal()
{
	return &(zVal_[0]);
}


template <class BaseType, class CoType>
Active<BaseType, CoType> * LpSolution<BaseType, CoType>::active()
{
	return active_;
}


template <class BaseType, class CoType>
int LpSolution<BaseType, CoType>::idSub() const
{
	return idSub_;
}


template <class BaseType, class CoType>
int LpSolution<BaseType, CoType>::idLp() const
{
	return idLp_;
}

}
