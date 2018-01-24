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

#include <ogdf/lib/abacus/lpsub.h>
#include <ogdf/lib/abacus/infeascon.h>
#include <ogdf/lib/abacus/row.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/column.h>

namespace abacus {


LpSub::~LpSub()
{
	const int nInfeasCons = infeasCons_.size();

	for(int c = 0; c < nInfeasCons; c++)
		delete infeasCons_[c];
}


void LpSub::initialize()
{
	// LpSub::initialize(): local variables
	Array<double> obj(sub_->nVar());
	Array<double> lBound(sub_->nVar());
	Array<double> uBound(sub_->nVar());
	Array<Row*>   rows(sub_->nCon());

	Array<LPVARSTAT::STATUS> lpVarStat(sub_->nVar());
	Array<SlackStat::STATUS> slackStat(sub_->nCon());

	Row row(master_, sub_->nVar());	//!< buffer to store generated row
	int c;							//!< loop index

	// generate the row format of the active constraints
	/* After the generation of the row format we allocate a row of
	*   the correct length and make a copy in order to safe memory.
	*/
	const int nCon = sub_->nCon();

	for (c = 0; c < nCon; c++) {
		// number of nonzeros of constraint \a c
		int conNnz  = sub_->constraint(c)->genRow(sub_->actVar(), row);
		rows[c] = new Row(master_, conNnz);
		rows[c]->copy(row);
		slackStat[c] = sub_->slackStat(c)->status();
		row.clear();
	}

	// eliminate set and fixed variables and initialize the columns
	Variable   *v;                           //!< pointer to variable of subproblem
	Array<bool> marked(0,sub_->nVar()-1, false);  //!< \a true if variable can be eliminated

	nOrigVar_ = sub_->nVar();
	valueAdd_ = 0.0;

	// LpSub: mark variables to eliminate, build objective function and bounds
	/* We mark all variables which can be eliminated, add them to the
	*   ArrayBuffer \a delVar, compute the mappings from the original variable
	*   set to the actual variable set in the \a LP, and vice versa, and
	*   determine the correction term for the LP-value.

	*   If all variables can be eliminated then we do not eliminate the last
	*   variable for simpification. Otherwise it would be necessary to load
	*   an problem with 0 variables to the LP-solver which is, e.g., for
	*   Cplex not possible. Although the emulation of the optimization would
	*   still be simple, but extra work would have to be performed if later
	*   constraints were added.
	*/
	const int nVar = sub_->nVar();
	int nCol = 0;
	for (int i = 0; i < nVar; i++) {
		v = sub_->variable(i);
		if(sub_->fsVarStat(i)->fixedOrSet()) {
			if (eliminable(i) && (nCol || (i != sub_->nVar() - 1))) {

				//! eliminate variable \a i from the LP
				marked[i]  = true;
				valueAdd_  += v->obj() * elimVal(i);
				orig2lp_[i] = -1;
			}
			else {

				// fix variable \a i in the LP
				/* As variable \a i could not be eliminated we set both its upper and lower
				*   bound to the value it is fixed or set to.
				*/
				orig2lp_[i]     = nCol;
				lp2orig_[nCol]  = i;
				obj[nCol]       = v->obj();
				lBound[nCol]    = elimVal(i);
				uBound[nCol]    = elimVal(i);
				lpVarStat[nCol] = sub_->lpVarStat(i)->status();
				++nCol;
			}
		}
		else {

			// add variable \a i to the LP
			orig2lp_[i]     = nCol;
			lp2orig_[nCol]  = i;
			obj[nCol]       = v->obj();
			lBound[nCol]    = sub_->lBound(i);
			uBound[nCol]    = sub_->uBound(i);
			lpVarStat[nCol] = sub_->lpVarStat(i)->status();
			++nCol;
		}
	}

	// LpSub: update the constraints
	/* If all active variables of a constraint are eliminated then
	*   its left hand side is void (implicitly 0), but its right hand side
	*   can be nonzero. Depending on the sense of the constraint it can be
	*   infeasible.
	*   If the elimination of variables from constraints causes an infeasible
	*   \a LP, the constraint is memorized in \a infeasCons_.
	*/
	ArrayBuffer<int> delVar(sub_->nVar(),false); //!< buffer of deletable components of row format
	InfeasCon::INFEAS infeas;    //!< infeasibility mode (TooLarge, TooSmall)

	for (c = 0; c < nCon; c++) {

		// eliminate the variables from the constraint
		delVar.clear();
		double rhsDelta = 0.0; // correction of right hand side due to eliminations
		const int rNnz = rows[c]->nnz();
		for(int i = 0; i < rNnz; i++) {
			if(marked[rows[c]->support(i)]) {
				delVar.push(i);
				rhsDelta += rows[c]->coeff(i)*elimVal(rows[c]->support(i));
			}
		}

		rows[c]->delInd(delVar, rhsDelta);

		// check if the constraint is now infeasible
		if (rows[c]->nnz() == 0) {
			infeas = sub_->constraint(c)->voidLhsViolated(rows[c]->rhs());
			if (infeas != InfeasCon::Feasible)
				infeasCons_.push(new InfeasCon(master_, sub_->constraint(c), infeas));
		}
		rows[c]->rename(orig2lp_);
	}

	// initialize the LP-solver and clean up
	LP::initialize(*master_->optSense(), nCon, sub_->maxCon(), nCol,
		sub_->maxVar(), obj, lBound, uBound, rows,
		lpVarStat, slackStat);

	for (c = 0; c < nCon; c++)
		delete rows[c];
}


void LpSub::constraint2row(
	ArrayBuffer<Constraint*> &cons,
	ArrayBuffer<Row*> &rows)
{
	Row   rowBuf(master_, sub_->nVar());  //!< dummy to generate row

	const int nCons = cons.size();

	for (int c = 0; c < nCons; c++) {
		// number of nonzero elements in constraint
		int conNnz = cons[c]->genRow(sub_->actVar(), rowBuf);
		// pointer to the new row
		Row  *row = new Row(master_, conNnz);
		row->copy(rowBuf);
		rows.push(row);
		rowBuf.clear();
	}
}


bool LpSub::eliminable(int i) const
{
	if (master_->eliminateFixedSet()) {
		return !sub_->lpVarStat(i)->basic();
	}
	else
		return false;
}


double LpSub::elimVal(int i) const
{
	switch (sub_->fsVarStat(i)->status()) {
	case FSVarStat::SetToLowerBound:   return sub_->lBound(i);
	case FSVarStat::FixedToLowerBound: return sub_->variable(i)->lBound();
	case FSVarStat::SetToUpperBound:   return sub_->uBound(i);
	case FSVarStat::FixedToUpperBound: return sub_->variable(i)->uBound();
	case FSVarStat::Set:               return sub_->fsVarStat(i)->value();
	case FSVarStat::Fixed:             return sub_->variable(i)->fsVarStat()->value();

	default:
		Logger::ifout() << "LpSub::elimVal(): variable neither fixed nor set\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpSub);
	}
}


double LpSub::elimVal(FSVarStat *stat, double lb, double ub) const
{
	switch (stat->status()) {
	case FSVarStat::SetToLowerBound:   return lb;
	case FSVarStat::FixedToLowerBound: return lb;
	case FSVarStat::SetToUpperBound:   return ub;
	case FSVarStat::FixedToUpperBound: return ub;
	case FSVarStat::Set:               return stat->value();
	case FSVarStat::Fixed:             return stat->value();

	default:
		Logger::ifout() << "LpSub::elimVal(): variable neither fixed nor set\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpSub);
	}
}


LP::OPTSTAT LpSub::optimize(METHOD method)
{
	OPTSTAT status;

	if (infeasCons_.size()) {
		Logger::ifout() << "LpSub::optimize(): there are infeasible constraints\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpSub);
	}
	else {
		status = LP::optimize(method);
		if (status == Infeasible && method != Dual) return optimize(Dual);
		else                                        return status;
	}
}


void LpSub::removeVars(ArrayBuffer<int> &vars)
{
	// LpSub::removeVars(): local variables
	ArrayBuffer<int>       lpVars(vars.size(),false);   //!< indices in \a LP of removed variables
	Array<double>     rhsDelta(0,sub_->nCon()-1, 0.0);  //!< changes of right hand side
	double            coeff;
	Variable      *v;
	bool              modifyRhs = false;
	double            eps = master_->eps();

	// LpSub::removeVars(): update the number of original variables
	int oldNOrigVar = nOrigVar_;

	nOrigVar_ -= vars.size();

	// divide removed variables in eliminated and non-eliminated ones
	/* If a removed variable has earlier been eliminated from the LP, then
	*   we might have to adapt the right hand side again, if earlier
	*   the elimination changed the right and side. Otherwise,
	*   we add the variable to the buffer \a lpVars in order to remove
	*   it explicitly later.
	*/
	const int nVars = vars.size();

	for (int i = 0; i < nVars; i++) {
		// name of variable in the \a LP
		int lpName = orig2lp_[vars[i]];
		if (lpName == -1) {
			//! remove eliminated variable
			valueAdd_  += sub_->variable(i)->obj() * elimVal(i);

			const int nCon = sub_->nCon();

			v = sub_->variable(i);
			for (int c = 0; c < nCon; c++) {
				coeff = sub_->constraint(c)->coeff(v);
				if (fabs(coeff) > eps) {
					rhsDelta[c] += coeff * elimVal(i);
					modifyRhs    = true;
				}
			}
		}
		else lpVars.push(lpName);
	}

	// adapt the right hand side if eliminated variables are removed
	if (modifyRhs) {
		Array<double> newRhs(sub_->nCon());
		const int nCon = sub_->nCon();

		for (int c = 0; c < nCon; c++)
			newRhs[c] = rhs(c) - rhsDelta[c];
		LP::changeRhs(newRhs);
	}

	// remove the non-eliminated variables
	/* Here, we also should check for constraints getting a void left hand side
	*   and becoming infeasible. However, on the one hand this is computationally
	*   expensive (using the member function \a row())
	*   as most LP-solvers (as, e.g., Cplex) work in a column oriented
	*   form, and second, if immediately afterwards variables are added then
	*   the linear program could become again feasible.
	*
	*   Moreover, if only inequalities with void left hand side become infeasible,
	*   then these infeasaibilities are recognized by the LP-solver and resolved
	*   in \a makeFeas(). Only equations can cause some trouble as there is no
	*   slack variable.
	*
	*   Therefore,  unfortunately, taking care that no equation becomes infeasible has to be
	*   left to the user.
	*/
	LP::remCols(lpVars);

	// update mappings of original variables and LP variables
	// sort the variables being removed
	// check if sorting is required
	bool unordered = false;

	for (int i = 0; i < nVars - 1; i++) {
		if (vars[i] > vars[i+1]) {
			unordered = true;
			break;
		}
	}

	// if yes, sort the variables
	ArrayBuffer<int> varsSorted(oldNOrigVar,false);

	if (unordered) {
		Array<bool> marked(0,oldNOrigVar-1, false);

		for (int i = 0; i < nVars; i++)
			marked[vars[i]] = true;

		for (int i = 0; i < oldNOrigVar; i++)
			if (marked[i])
				varsSorted.push(i);
	}
	else
		for (int i = 0; i < nVars; i++)
			varsSorted.push(vars[i]);

	// update mapping of original variables to LP variables
	/* In order to update the mapping of the original variables to the LP-variables
	*  we have to eliminate the removed variables from the array \a orig2lp_ by a
	*   leftshift. Moreover, if the variable \a i is not removed then we have to
	*   reduce \a orig2lp_ by the number of variables that have been removed with a
	*   index than \a i that have not been eliminated.
	*/
	int current = varsSorted[0];
	int nNotEliminatedRemoved = 0;

	for (int i = 0; i < nVars - 1; i++) {
		if (orig2lp_[varsSorted[i]] != -1)
			nNotEliminatedRemoved++;

		const int last = varsSorted[i+1];
		for(int j = varsSorted[i]+1; j < last; j++)
			if (orig2lp_[j] == -1)
				orig2lp_[current++] = -1;
			else
				orig2lp_[current++] = orig2lp_[j] - nNotEliminatedRemoved;
	}

	if (orig2lp_[varsSorted[nVars-1]] != -1)
		nNotEliminatedRemoved++;

	for (int j = varsSorted[nVars - 1] + 1; j < oldNOrigVar; j++)
		if (orig2lp_[j] == -1)
			orig2lp_[current++] = -1;
		else
			orig2lp_[current++] = orig2lp_[j] - nNotEliminatedRemoved;

	// update mapping of LP variables to original variables
	/* Since \a orig2lp_ is updated already we can update the reverse
	*   mapping \a lp2orig_ in a straight forward way by scanning \a orig2lp_.
	*/
	int nVarLp = 0;

	for (int i = 0; i < nOrigVar_; i++)
		if (orig2lp_[i] != -1)
			lp2orig_[nVarLp++] = i;
}


void LpSub::addCons(ArrayBuffer<Constraint*> &newCons)
{
	// LpSub::addCons(): local variables
	ArrayBuffer<Row*> newRows(newCons.size(),false);  //!< the new constraints in row format
	ArrayBuffer<int> delVar(sub_->nVar(),false); //!< buffer of deletable components of row format
	InfeasCon::INFEAS infeas;    //!< infeasibility mode (TooLarge, TooSmall)

	Row *nr;

	constraint2row(newCons, newRows);

	// eliminate variables in added constraints
	/* Also the elimination of variables in an added constraint might
	*   cause a void left hand side (interpreted as 0) violated the right hand
	*   side of the constraint. These infeasible constraints are recognized,
	*   but the resolution is currently not implemented.
	*/
	const int nNewRows = newRows.size();

	for (int c = 0; c < nNewRows; c++) {
		//! eliminate variables in constraint \a c
		delVar.clear();
		double rhsDelta = 0.0; //!< correction of right hand side
		nr       = newRows[c];
		const int nrNnz = nr->nnz();
		for(int i = 0; i < nrNnz; i++) {
			if(eliminated(nr->support(i))) {
				delVar.push(i);
				rhsDelta += nr->coeff(i)*elimVal(nr->support(i));
			}
		}
		nr->delInd(delVar,rhsDelta);
		nr->rename(orig2lp_);

		// check if constraint \a c has become infeasible
		if(nr->nnz() == 0) {
			infeas = newCons[c]->voidLhsViolated(nr->rhs());
			if (infeas != InfeasCon::Feasible) {
				infeasCons_.push(new InfeasCon(master_, newCons[c], infeas));
				Logger::ifout() << "LpSub::addCons(): infeasible constraint added.\nAll variables with nonzero coefficients are eliminated and constraint is violated.\nSorry, resolution not implemented yet.\n";
				OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpSub);
			}
		}
	}

	LP::addRows(newRows);

	for (auto &newRow : newRows) {
		delete newRow;
	}
}


void LpSub::addVars(
	ArrayBuffer<Variable*> &vars,
	ArrayBuffer<FSVarStat*> &fsVarStat,
	ArrayBuffer<double> &lb,
	ArrayBuffer<double> &ub)
{
	// LpSub::addVars(): local variables
	ArrayBuffer<int> delVar(vars.size(),false);  //!< the eliminated variables
	Array<double> rhsDelta(0,sub_->nCon()-1, 0.0);  //!< the correction of the rhs
	double vValue;
	double coeff;
	bool modifyRhs = false;  //!< if \a true the modification of rhs required
	int oldNCol = trueNCol();
	int n = trueNCol();

	// divide the added variables in eliminable and non-eliminable ones
	int nVariables = vars.size();

	for (int i = 0; i < nVariables; i++) {
		Variable *v = vars[i];
		if(fsVarStat[i]->fixedOrSet()) {
			if (eliminable(i)) {
				//! the new variable is eliminated
				delVar.push(i);

				vValue = elimVal(fsVarStat[i], lb[i], ub[i]);

				valueAdd_ += v->obj() * vValue;
				orig2lp_[nOrigVar_++] = -1;

				const int nCon = sub_->nCon();

				for (int c = 0; c < nCon; c++) {
					coeff = sub_->constraint(c)->coeff(v);
					if (fabs(coeff) > master_->eps()) {
						rhsDelta[c] += vValue * coeff;
						modifyRhs    = true;
					}
				}
			}
			else {
				// the new variable is fixed in the LP
				orig2lp_[nOrigVar_++] = n;
				lp2orig_[n] = oldNCol + i;
				++n;
				lb[i] = ub[i] = elimVal(fsVarStat[i], lb[i], ub[i]);
			}
		}
		else {
			// the new variable will be added to the LP explicitly
			orig2lp_[nOrigVar_++] = n;
			lp2orig_[n] = oldNCol + i;
			++n;
		}
	}

	// remove the fixed and set added variables
	if (delVar.size()) {
		vars.leftShift(delVar);
		fsVarStat.leftShift(delVar);
		lb.leftShift(delVar);
		ub.leftShift(delVar);
	}

	// generate the column of the added variable and add them to the LP
	ArrayBuffer<Column*> newCols(vars.size(),false);
	//!< new columns added to the constraint matrix
	Column colBuf(master_, nRow());  //!< buffer for generated columns

	nVariables = vars.size();
	for(int i = 0; i < nVariables; i++) {
		vars[i]->genColumn(sub_->actCon(), colBuf);
		Column *col = new Column(master_, colBuf.nnz());
		col->copy(colBuf);
		col->obj(colBuf.obj());
		col->uBound(colBuf.uBound());
		col->lBound(colBuf.lBound());
		newCols.push(col);
		colBuf.clear();
	}

	LP::addCols(newCols);

	// modify the right hand side if fixed or set variables are added
	if (modifyRhs) {
		const int nCon = sub_->nCon();

		Array<double> newRhs(nCon);
		for(int c = 0; c < nCon; c++)
			newRhs[c] = rhs(c) - rhsDelta[c];

		changeRhs(newRhs);
	}

	// LpSub::addVars(): clean up
	for(int i = 0; i < nVariables; i++)
		delete newCols[i];
}


void LpSub::changeLBound(int i, double newLb)
{
	int lpVar = orig2lp_[i];

	if (lpVar == -1) {
		Logger::ifout() << "LpSub::changeLBound(" << i << " ," << newLb << ")\nvariable " << i << " is eliminated, cannot change bounds!\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpSub);
	}
	else LP::changeLBound(lpVar, newLb);
}


void LpSub::changeUBound(int i, double newUb)
{
	int lpVar = orig2lp_[i];

	if (lpVar == -1) {
		Logger::ifout() << "LpSub::changeUBound(" << i << " ," << newUb << ")\nvariable " << i << " is eliminated, cannot change bounds!\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpSub);
	}
	else LP::changeUBound(lpVar, newUb);
}


double LpSub::lBound(int i) const
{
	int lpVar = orig2lp_[i];

	if (lpVar != -1) return LP::lBound(lpVar);
	else             return elimVal(i);
}


double LpSub::uBound(int i) const
{
	int lpVar = orig2lp_[i];

	if (lpVar != -1) return LP::uBound(lpVar);
	else             return elimVal(i);
}


double LpSub::xVal(int i) const
{
	int lpVar = orig2lp_[i];

	if (lpVar != -1) return LP::xVal(lpVar);
	else             return elimVal(i);
}


double LpSub::barXVal(int i) const
{
	int lpVar = orig2lp_[i];

	if (lpVar != -1) return LP::barXVal(lpVar);
	else             return elimVal(i);
}


double LpSub::reco(int i) const
{
	int lpVar = orig2lp_[i];

	if (lpVar != -1) return LP::reco(lpVar);
	else             return 0.0;
}


LPVARSTAT::STATUS LpSub::lpVarStat(int i) const
{
	int lpVar = orig2lp_[i];

	if (lpVar != -1) return LP::lpVarStat(lpVar);
	else             return LPVARSTAT::Eliminated;
}


int LpSub::getInfeas(int &infeasCon, int &infeasVar, double *bInvRow) const
{
	int status = LP::getInfeas(infeasCon, infeasVar, bInvRow);

	if (status) {
		Logger::ifout() << "LpSub::getInfeas(): LP::getInfeas() failed\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpSub);
	}

	if (infeasVar >= 0) infeasVar = lp2orig_[infeasVar];

	return 0;
}


void LpSub::loadBasis(Array<LPVARSTAT::STATUS> &lpVarStat,
						  Array<SlackStat::STATUS> &slackStat)
{
	Array<LPVARSTAT::STATUS> colStat(trueNCol());
	int n = 0;

	const int nVar = sub_->nVar();

	for (int i = 0; i < nVar; i++)
		if (!eliminated(i))
			colStat[n++] = lpVarStat[i];

	LP::loadBasis(colStat, slackStat);
}


void LpSub::varRealloc(int newSize)
{
	LP::colRealloc(newSize);

	orig2lp_.resize(newSize);
	lp2orig_.resize(newSize);
}


void LpSub::conRealloc(int newSize)
{
	LP::rowRealloc(newSize);
	infeasCons_.setCapacity(newSize);
}
}
