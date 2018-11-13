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

#include <ogdf/lib/abacus/fixcand.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/sub.h>

namespace abacus {


void FixCand::saveCandidates(Sub *sub)
{
	// count the candidates and allocate memory
	/* All variables which have status \a AtUpperBound or \a AtLowerBound
	*  are candidates. Only discrete variables can be candidates. To avoid
	*  memory leaks we delete the memory used to store older candidate sets
	*  before we allocate the necessary memory.
	*/
	int nCand = 0;  // number of candidates

	const int nVar = sub->nVar();

	for (int i = 0; i < nVar; i++)
		if (sub->lpVarStat(i)->atBound() && sub->variable(i)->discrete()) nCand++;

	deleteAll();
	allocate(nCand);

	// collect the candidates
	/* For each candidate we memorize the left hand side of the condition
	*  tested for fixing. Then later we only have to compare if this
	*  value is greater than the primal bound for minimization problems or
	*  less than the primal bound for maximization problems, respectively.
	*/
	LpSub *lp = sub->lp();

	for (int i = 0; i < nVar; i++)
		if (sub->lpVarStat(i)->atBound() && sub->variable(i)->discrete()) {
			candidates_->push(
				new PoolSlotRef<Variable,Constraint>(*(sub->actVar()->poolSlotRef(i))));

			if (sub->lpVarStat(i)->status() == LPVARSTAT::AtLowerBound) {
				lhs_->push(lp->value() + lp->reco(i));
				fsVarStat_->push(new FSVarStat(master_,FSVarStat::FixedToLowerBound));
			}
			else {
				lhs_->push(lp->value() - lp->reco(i));
				fsVarStat_->push(new FSVarStat(master_,FSVarStat::FixedToUpperBound));
			}
		}
}


void FixCand::fixByRedCost(CutBuffer<Variable, Constraint> *addVarBuffer)
{
	if (candidates_ == nullptr) return;

	Logger::ilout(Logger::Level::Minor) << std::endl << "Fixing Variables by Reduced Costs:     ";

	const int nCandidates = candidates_->size();

	ArrayBuffer<int> fixed(nCandidates,false);  // fixed variables
	Variable    *v;                                // variable being fi0xed

	for (int i = 0; i < nCandidates; i++) {
		if ((master_->optSense()->max() &&
			((*lhs_)[i] + master_->eps() < master_->primalBound())) ||
			(master_->optSense()->min() &&
			((*lhs_)[i] - master_->eps() > master_->primalBound())))
		{
			v = (Variable *) (*candidates_)[i]->conVar();
			if (v) {
				if (!v->fsVarStat()->fixed())
					master_->newFixed(1);
				v->fsVarStat()->status((*fsVarStat_)[i]);

				// should a fixed inactive variable be activated?
				/* If an inactive variable is fixed to a value different from 0, then
				*  we activate it.
				*/
				if (!v->active()) {
					switch (v->fsVarStat()->status()) {
					case FSVarStat::FixedToLowerBound:
						if (fabs(v->lBound()) > master_->eps())
							addVarBuffer->insert((*candidates_)[i]->slot(), true);
						break;

					case FSVarStat::FixedToUpperBound:
						if (fabs(v->uBound()) > master_->eps())
							addVarBuffer->insert((*candidates_)[i]->slot(), true);
						break;

					case FSVarStat::Fixed:
						if (fabs(v->fsVarStat()->value()) > master_->eps())
							addVarBuffer->insert((*candidates_)[i]->slot(), true);
						break;

					default:
						Logger::ifout() << "FixCand::fixByRedCost(): activated variable not fixed\n";
						OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::FixCand);
					}
				}

				fixed.push(i);
			}
		}
	}
	// remove fixed candidates
	/* We delete allocated memory of \a fsVarStat_ for the fixed variables
	*  und remove the fixed variables from the three buffers.
	*/
	const int nFixed = fixed.size();

	for (int i = 0; i < nFixed; i++) {
		delete (*candidates_)[fixed[i]];
		delete (*fsVarStat_)[fixed[i]];
	}

	candidates_->leftShift(fixed);
	fsVarStat_->leftShift(fixed);
	lhs_->leftShift(fixed);

	Logger::ilout(Logger::Level::Minor) << "\t" << fixed.size() << " variables fixed" << std::endl;
}


void FixCand::deleteAll()
{
	int i;

	if (candidates_) {
		const int nCandidates = candidates_->size();

		for (i = 0; i < nCandidates; i++)
			delete (*candidates_)[i];
		delete candidates_;
		candidates_ = nullptr;
	}

	if (fsVarStat_) {
		const int nFsVarStat = fsVarStat_->size();
		for (i = 0; i < nFsVarStat; i++) delete (*fsVarStat_)[i];
		delete fsVarStat_;
		fsVarStat_ = nullptr;
	}

	if (lhs_) {
		delete lhs_;
		lhs_ = nullptr;
	}
}


void FixCand::allocate(int nCand)
{
	candidates_ = new ArrayBuffer<PoolSlotRef<Variable,Constraint>*>(nCand,false);
	fsVarStat_  = new ArrayBuffer<FSVarStat*>(nCand,false);
	lhs_        = new ArrayBuffer<double>(nCand,false);
}
}
