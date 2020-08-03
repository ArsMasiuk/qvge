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
 *
 */

#include <ogdf/lib/abacus/conbranchrule.h>
#include <ogdf/lib/abacus/sub.h>

namespace abacus {


std::ostream &operator<<(std::ostream &out, const ConBranchRule &rhs)
{
	return out << rhs.poolSlotRef_;
}


int ConBranchRule::extract(Sub *sub)
{

	if (poolSlotRef_.conVar() == nullptr) {
		Logger::ifout() << "ConBranchRule::extract(): branching constraint not available\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::ConBranchRule);
	}

	if (sub->addBranchingConstraint(poolSlotRef_.slot())) {
		Logger::ifout() << "ConBranchRule::extract(): addition of branching constaint to subproblem failed.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::ConBranchRule);
	}

	return 0;
}


void ConBranchRule::extract(LpSub *lp)
{
	ArrayBuffer<Constraint*> newCon(1,false);

	newCon.push(poolSlotRef_.conVar());
	lp->addCons(newCon);
}


void ConBranchRule::unExtract(LpSub *lp)
{
	ArrayBuffer<int> remove(1,false);

	remove.push(lp->nRow() - 1);

	// pivot the slack variable associated with the removed row in

	int status = lp->pivotSlackVariableIn(remove);

	if (status) {
		Logger::ifout() << "WARNING: ";
		Logger::ifout() << "ConBranchRule::unExtract(): pivoting in ";
		Logger::ifout() << "slack variable failed." << std::endl;
	}

	lp->removeCons(remove);
}


void ConBranchRule::initialize(Sub* sub)
{
	if (poolSlotRef_.conVar() == nullptr) {
		Logger::ifout() << "ConBranchRule::initialize(): branching constraint not available\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::ConBranchRule);
	}

	poolSlotRef_.conVar()->sub(sub);
}
}
