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
* $Id: sub.cc,v 2.17 2008-10-10 09:33:04 sprenger Exp $
*/

#include <ogdf/lib/abacus/sub.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/tailoff.h>
#include <ogdf/lib/abacus/branchrule.h>
#include <ogdf/lib/abacus/lpvarstat.h>
#include <ogdf/lib/abacus/bprioqueue.h>
#include <ogdf/lib/abacus/column.h>
#include <ogdf/lib/abacus/poolslot.h>
#include <ogdf/lib/abacus/cutbuffer.h>
#include <ogdf/lib/abacus/opensub.h>
#include <ogdf/lib/abacus/fixcand.h>
#include <ogdf/lib/abacus/setbranchrule.h>
#include <ogdf/lib/abacus/boundbranchrule.h>
#include <ogdf/lib/abacus/standardpool.h>

#ifdef TTT
extern "C" {
#include <cplex.h>
}
#endif

namespace abacus {


Sub::Sub(
	Master *master,
	double conRes,
	double varRes,
	double nnzRes,
	bool relativeRes,
	ArrayBuffer<PoolSlot<Constraint, Variable> *> *constraints,
	ArrayBuffer<PoolSlot<Variable, Constraint> *> *variables)
	:
master_(master),
	actCon_(nullptr),
	actVar_(nullptr),
	father_(nullptr),
	lp_(nullptr),
	fsVarStat_(nullptr),
	lpVarStat_(nullptr),
	lBound_(nullptr),
	uBound_(nullptr),
	slackStat_(nullptr),
	tailOff_(nullptr),
	dualBound_(master->dualBound()),
	nIter_(0),
	lastIterConAdd_(0),
	lastIterVarAdd_(0),
	branchRule_(nullptr),
	allBranchOnSetVars_(true),
	lpMethod_(LP::Primal),
	addVarBuffer_(nullptr),
	addConBuffer_(nullptr),
	removeVarBuffer_(nullptr),
	removeConBuffer_(nullptr),
	xVal_(nullptr),
	yVal_(nullptr),
	genNonLiftCons_(false),
	level_(1),
	id_(1),
	status_(Unprocessed),
	sons_(nullptr),
	maxIterations_(master->maxIterations()),
	nOpt_(0),
	relativeReserve_(relativeRes),
	varReserve_(varRes),
	conReserve_(conRes),
	nnzReserve_(nnzRes),
	activated_(false),
	ignoreInTailingOff_(false),
	//lastLP_(LP::BarrierAndCrossover),
	lastLP_(LP::Primal),
	forceExactSolver_(false)
{
	// initialize the active constraints of the root node
	int maxCon;

	if (constraints) {
		// initialize the active constraints with \a constraints
		if (relativeRes)
			maxCon   = (int) (constraints->size() * (1.0 + conRes/100.0));
		else
			maxCon   = constraints->size() + (int) conRes;

		actCon_ = new Active<Constraint, Variable>(master_, maxCon);

		actCon_->insert(*constraints);

	}
	else  {
		// initialize the active constraints with the default constraint pool
		if (relativeRes)
			maxCon  = (int) (master_->conPool()->number() * (1.0 + conRes/100.0));
		else
			maxCon = master_->conPool()->number() + (int) conRes;

		actCon_ = new Active<Constraint, Variable>(master_, maxCon);

		const int nConPool = master_->conPool()->number();

		for (int i = 0; i < nConPool; i++)
			actCon_->insert(master_->conPool()->slot(i));

	}

	slackStat_ = new Array<SlackStat*>(maxCon);

	const int nConstraints = nCon();

	for (int i = 0; i < nConstraints; i++)
		(*slackStat_)[i] = new SlackStat();

	// initialize the active variables of the root node
	int maxVar;
	if (variables) {
		// initialize the active variables with \a variables
		if (relativeRes)
			maxVar  = (int) (variables->size() * (1.0 + varRes/100.0));
		else
			maxVar = variables->size() + (int) varRes;
		actVar_ = new Active<Variable, Constraint>(master_, maxVar);

		actVar_->insert(*variables);

	}
	else  {
		// initialize the active variables with the default variable pool
		if (relativeRes)
			maxVar  = (int) (master_->varPool()->number() * (1.0 + varRes/100.0));
		else
			maxVar = master_->varPool()->number() + (int) varRes;
		actVar_ = new Active<Variable, Constraint>(master_, maxVar);

		const int nVarPool = master_->varPool()->number();
		for (int i = 0; i < nVarPool; i++)
			actVar_->insert(master_->varPool()->slot(i));

	}

	// initializes the local variables statuses and the bounds
	/* By initializing \a *fsVarStat_ with the global status we
	*   can both handle prefixed variables and enable automatically
	*   that all fixed variables stay fixed in the case of a restart
	*   of the optimization process.
	*/
	fsVarStat_  = new Array<FSVarStat*>(maxVar);
	lpVarStat_  = new Array<LPVARSTAT*>(maxVar);
	lBound_     = new Array<double>(maxVar);
	uBound_     = new Array<double>(maxVar);

	Variable *v;

	const int nVariables = nVar();
	for (int i = 0; i < nVariables; i++) {
		v                 = variable(i);
		(*fsVarStat_)[i]  = new FSVarStat(v->fsVarStat());
		(*lpVarStat_)[i]  = new LPVARSTAT;
		(*lBound_)[i]     = v->lBound();
		(*uBound_)[i]     = v->uBound();
	}

	//! register the subproblem at the master
	master_->newSub(level_);
	master_->treeInterfaceNodeBounds(id_, lowerBound(), upperBound());
}


Sub::Sub(Master *master, Sub *father, BranchRule *branchRule)
	:
	master_(master),
	actCon_(nullptr),
	actVar_(nullptr),
	father_(father),
	lp_(nullptr),
	fsVarStat_(nullptr),
	lpVarStat_(nullptr),
	lBound_(nullptr),
	uBound_(nullptr),
	slackStat_(nullptr),
	tailOff_(nullptr),
	dualBound_(father->dualBound_),
	nIter_(0),
	lastIterConAdd_(0),
	lastIterVarAdd_(0),
	branchRule_(branchRule),
	lpMethod_(LP::Dual),
	addVarBuffer_(nullptr),
	addConBuffer_(nullptr),
	removeVarBuffer_(nullptr),
	removeConBuffer_(nullptr),
	xVal_(nullptr),
	yVal_(nullptr),
	bInvRow_(nullptr),
	genNonLiftCons_(false),
	level_(father->level() + 1),
	id_(master->nSub() + 1),
	status_(Unprocessed),
	sons_(nullptr),
	maxIterations_(master->maxIterations()),
	nOpt_(0),
	relativeReserve_(father_->relativeReserve_),
	varReserve_(father_->varReserve_),
	conReserve_(father_->conReserve_),
	nnzReserve_(father_->nnzReserve_),
	activated_(false),
	ignoreInTailingOff_(false) ,
	//lastLP_(LP::BarrierAndCrossover),
	lastLP_(LP::Primal),
	forceExactSolver_(false)
{
	branchRule_->initialize(this);
	allBranchOnSetVars_ = ( father_->allBranchOnSetVars_ && branchRule_->branchOnSetVar() );
	master_->newSub(level_);
	master_->treeInterfaceNodeBounds(id_, lowerBound(), upperBound());
}


Sub::~Sub()
{
	if (sons_) {
		const int nSons = sons_->size();

		for (int i = 0; i < nSons; i++)
			delete (*sons_)[i];
		delete sons_;
	}
	else if (status_ == Unprocessed || status_ == Dormant)
		master_->openSub()->remove(this);
}


int Sub::optimize()
{
	PHASE phase;  //!< current phase of the subproblem optimization

	// update the global dual bound
	/* The global dual bound is the maximum (minimum) of the
	*   dual bound of the subproblem and the dual bounds of the
	*   subproblems which still have to be processed if this
	*   is a maximization (minimization) problem.
	*/
	double newDual = dualBound_;

	if (master_->optSense()->max()) {
		if (master_->openSub()->dualBound() > newDual)
			newDual = master_->openSub()->dualBound();
	}
	else
		if (master_->openSub()->dualBound() < newDual)
			newDual = master_->openSub()->dualBound();

	if (master_->betterDual(newDual)) master_->dualBound(newDual);


	// output a banner for the subproblem
	if(Logger::is_ilout(Logger::Level::Medium)) {
		Logger::ifout() << std::endl << "************************************************" << std::endl
		 << "Subproblem " << id_ << " on Level " << level_ << ":" << std::endl << std::endl;

		if (master_->optSense()->max()) {
			Logger::ifout() << "\tGlobal Lower Bound: " << lowerBound()       << std::endl
			 << "\tLocal  Upper Bound: " << upperBound()       << std::endl
			 << "\tGlobal Upper Bound: " << master_->upperBound() << std::endl;
		}
		else {
			Logger::ifout() << "\tLocal  Lower Bound: " << lowerBound()       << std::endl
			 << "\tGlobal Lower Bound: " << master_->lowerBound() << std::endl
			 << "\tGlobal Upper Bound: " << upperBound()       << std::endl;
		}

		Logger::ifout() << "\tCurrent Guarantee : ";
		master_->printGuarantee();
		Logger::ifout() << std::endl << std::endl;
	}

	++nOpt_;

	phase = _activate ();

	while (phase != Done) {
		switch (phase) {
		case Cutting:   phase = cutting();
			break;
		case Branching: phase = branching();
			break;
		case Fathoming: phase = fathoming ();
			break;
		default:
			Logger::ifout() << "Sub::optimize(): unknown phase " << phase << "\nFurther processing not possible.\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Phase);
		}
	}
	_deactivate ();

	if(Logger::is_ilout(Logger::Level::Medium)) {
		// output a line about the subproblem optimization
		/* We output the total number of subproblems, the number of open subproblems,
		*   the number of iterations in the cutting plane phase, the dual bound,
		*   and the primal bound.
		*/
		Logger::ifout() << setw(7) << master_->nSub() << " "
		 << setw(7) << master_->openSub()->number() << "  "
		 << setw(8) << id_ << " "
		 << setw(7) << nIter_ << " ";
		if (infeasible())
			Logger::ifout() << setw(10) << "infeas" << " ";
		else
			Logger::ifout() << setw(10) << dualBound() << " ";
		Logger::ifout() << setw(10) << master_->dualBound() << " ";
		if (master_->feasibleFound())
			Logger::ifout() << setw(10) << master_->primalBound() << std::endl;
		else
			Logger::ifout() << setw(10) << "---" << std::endl;

	} else {
		Logger::ilout(Logger::Level::Default) << "Enumeration Tree" << std::endl
		 << "\tNumber of Subproblems:   " << master_->nSub() << std::endl
		 << "\tNumber of Open Problems: " << master_->openSub()->number() << std::endl;
	}

	return 0;
}


Sub::PHASE Sub::_activate()
{
	// activate the subproblem in the VBC-Tool
	master_->treeInterfacePaintNode(id_, 6);

	// can the subproblem be fathomed without processing?
	/* If during the subproblem was waiting for further processing
	*   in the list of open subproblems,
	*   a primal bound better than its dual bound has been found, then
	*   we can fathom the subproblem immediately.
	*/
	if (boundCrash()) return Fathoming;

	// determine the initial maximal number of constraints and variables
	/* We overestimate the initial number of constraints and variables in
	*   order to avoid too many reallocations during the optimization process.
	*
	*   If the subproblem is the root node or has been processed already, then
	*   this overestimation is not required, because extra memory has either
	*   been allocated already in the constructor or is available from the
	*   last optimization respectively.
	*/
	int initialMaxVar;
	int initialMaxCon;

	if (status_ == Unprocessed && this != master_->root()) {
		if (relativeReserve_) {
			initialMaxVar = (int) (father_->nVar() *(1.0 + varReserve_/100.0));
			initialMaxCon = (int) (father_->nCon() *(1.0 + conReserve_/100.0));
		}
		else {
			initialMaxVar = father_->nVar() + (int) varReserve_;
			initialMaxCon = father_->nCon() + (int) conReserve_;
		}
	}
	else {
		initialMaxVar = maxVar();
		initialMaxCon = maxCon();
	}

	// allocate local members of the subproblem
	tailOff_  = new TailOff(master_);
	addVarBuffer_    = new CutBuffer<Variable, Constraint>(master_,
		master_->maxVarBuffered());
	addConBuffer_    = new CutBuffer<Constraint, Variable>(master_,master_->maxConBuffered());
	removeVarBuffer_ = new ArrayBuffer<int>(initialMaxVar,false);
	removeConBuffer_ = new ArrayBuffer<int>(initialMaxCon,false);
	xVal_            = new double[initialMaxVar];
	yVal_            = new double[initialMaxCon];

	// perform activations for unprocessed non-root nodes
	/* The initialization of constraints and variables is performed by
	*   virtual functions such that easily other initialization methods
	*   can be applied.

	*   If there is a contradiction between a branching rule and the local
	*   information of the subproblem we can immediately fathom the subproblem.
	*/
	if (status_ == Unprocessed && this != master_->root()) {
		initializeVars(initialMaxVar);
		initializeCons(initialMaxCon);
		if (branchRule_->extract(this))
			return Fathoming;
	}

	// remove missing variables and constraints from the active sets
	/* After the function \a _activate() is performed we assume during the
	*   complete subproblem optimization that all active
	*   variables and constraints are available in some pool! Therefore, we
	*   remove now all missing variables and constraints from their active sets.
	*/

	// remove missing variables from the active variables
	/* It is a fatal error if a fixed or set variable is missing.
	*/
	ArrayBuffer<int> removeVars(nVar(),false);

	int nVariables = nVar();

	for (int i = 0; i < nVariables; i++) {
		if ((*actVar_)[i] == nullptr) {
			removeVars.push(i);
			if ((*fsVarStat_)[i]->fixedOrSet()) {
				Logger::ifout() << "Sub::_activate(): active fixed or set variable not available in pool\n";
				OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Active);
			}
			delete (*fsVarStat_)[i];
			delete (*lpVarStat_)[i];
		}
	}

	if (removeVars.size()) {
		Logger::ilout(Logger::Level::Medium) << removeVars.size() << " variables missing for initialization" << std::endl;

		actVar_->remove(removeVars);
		fsVarStat_->leftShift(removeVars);
		lpVarStat_->leftShift(removeVars);
		lBound_->leftShift(removeVars);
		uBound_->leftShift(removeVars);
	}

	// remove missing constraints from the active variables
	ArrayBuffer<int> removeCons(nCon(),false);

	const int nConstraints = nCon();
	for (int i = 0; i < nConstraints; i++) {
		if ((*actCon_)[i] == nullptr) {
			removeCons.push(i);
			delete (*slackStat_)[i];
		}
	}
	if (removeCons.size())
		Logger::ilout(Logger::Level::Medium) << removeCons.size() << " constraints missing for initialization" << std::endl;

	actCon_->remove(removeCons);
	slackStat_->leftShift(removeCons);


	// set the active flags of variables and constraints
	/* Setting \a status_ to \a Active at this point is necessary, since if the
	*   subproblem turns out to be fathomed already during processing
	*   this function, then in \a fathom() the variables and constraints
	*   have to be deactivated.
	*/
	const int nActVar = actVar_->number();
	for (int i = 0; i < nActVar; i++)
		(*actVar_)[i]->activate();

	const int nActCon = actCon_->number();
	for (int i = 0; i < nActCon; i++)
		(*actCon_)[i]->activate();

	status_  = ActiveSub;

	// perform problem specific activations
	/* We have to memorize if activate() has been called such that in
	*   \a _deactivate() only \a deactivate() is called when \a activate() has
	*   been performed. This is necessary because these lines are only
	*   reached if the dual bound is still better than the primal bound.
	*/
	activate();
	activated_ = true;

	// update fixed and set variables and set by logical implications
	/* We update global variable fixings which have been performed
	*   while the subproblem was sleeping. If there is a contradiction to
	*   set variables we can fathom the node.

	*   The adaption of branching variables may allow us to set further
	*   variables by logical implications. Again contradictions to
	*   already fixed variables can lead to an immediate \a Fathoming
	*   of the node.
	*/
	double     newBound;   //!< the new local bound

	nVariables = nVar();
	for (int i = 0; i < nVariables; i++) {
		FSVarStat *global = variable(i)->fsVarStat();     //!< global status of a variable
		FSVarStat *local = (*fsVarStat_)[i];      //!< local status of a variable
		if (global->fixed()) {
			if (global->contradiction(local)) {
				infeasibleSub();
				return Fathoming;
			}
			local->status(global);
			newBound      = fixSetNewBound(i);
			(*lBound_)[i] = newBound;
			(*uBound_)[i] = newBound;
		}
	}

	bool newValues;  //!< in this context only required as a dummy

	if (_setByLogImp(newValues)) {
		infeasibleSub();
		return Fathoming;
	}

	if (Logger::is_ilout(Logger::Level::Medium)) {

		// output number of active constraints and variables
		// output number of fixed and set variables
		int nFixed = 0;  //!< number of fixed variables
		int nSet   = 0;  //!< number of set variables

		const int numberVars = nVar();
		for (int i = 0; i < numberVars; i++)
			if ((*fsVarStat_)[i]->fixed()) ++nFixed;
			else if ((*fsVarStat_)[i]->set()) ++nSet;

		Logger::ifout() << std::endl
		 << "Subproblem Size" << std::endl
		 << "\tNumber of Active Constraints : " << nCon() << std::endl
		 << "\tNumber of Active Variables   : " << nVar() << std::endl
		 << "\tNumber of Fixed Variables    : " << nFixed << std::endl
		 << "\tNumber of Set Variables      : " << nSet   << std::endl;

	}

	// initialize the linear program of the subproblem
	/* If the \a LP turns out to be infeasible already in
	*   the initialization phase, we can again fathom the \bac\ node.
	*/
	if (initializeLp()) {
		infeasibleSub();
		return Fathoming;
	}

	return Cutting;
}


void Sub::initializeVars(int maxVar)
{
	actVar_ = new Active<Variable, Constraint>(master_,	father_->actVar_, maxVar);

	fsVarStat_ = new Array<FSVarStat*>(maxVar);
	lpVarStat_ = new Array<LPVARSTAT*>(maxVar);
	lBound_    = new Array<double>(maxVar);
	uBound_    = new Array<double>(maxVar);

	const int nVariables = nVar();

	for (int i = 0; i < nVariables; i++) {
		(*lpVarStat_)[i]  = new LPVARSTAT(father_->lpVarStat(i));
		(*fsVarStat_)[i]  = new FSVarStat(father_->fsVarStat(i));
		(*lBound_)[i]     = father_->lBound(i);
		(*uBound_)[i]     = father_->uBound(i);
	}
}


void Sub::initializeCons(int maxCon)
{
	actCon_ = new Active<Constraint, Variable>(master_, father_->actCon_, maxCon);

	slackStat_ = new Array<SlackStat*>(maxCon);

	const int nConstraints = nCon();

	for (int i = 0; i < nConstraints; i++)
		(*slackStat_)[i] = new SlackStat(*(father_->slackStat(i)));
}


void Sub::_deactivate()
{
	if (activated_) deactivate();
	// deactivate the subproblem in the VBC-Tool
	master_->treeInterfacePaintNode(id_, 1);

	// delete members redundant for inactive subproblems
	delete tailOff_;
	tailOff_ = nullptr;

	localTimer_.start(true);

	delete lp_;

	master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

	lp_ = nullptr;

	delete addVarBuffer_;
	addVarBuffer_ = nullptr;
	delete addConBuffer_;
	addConBuffer_ = nullptr;
	delete removeVarBuffer_;
	removeVarBuffer_ = nullptr;
	delete removeConBuffer_;
	removeConBuffer_ = nullptr;
	delete [] xVal_;
	xVal_ = nullptr;
	delete [] yVal_;
	yVal_ = nullptr;

	// reset the active flags of variables and constraints
	/* If the node being deactivated has just been fathomed then
	*   \a actVar_ and \a actCon_ are 0. In this case the deactivation
	*   has been performed already in the function \a fathom().
	*/
	int i;

	if (actVar_) {
		const int nActVar = actVar_->number();
		for (i = 0; i < nActVar; i++)
			(*actVar_)[i]->deactivate();
	}

	if (actCon_) {
		const int nActCon = actCon_->number();
		for (i = 0; i < nActCon; i++)
			(*actCon_)[i]->deactivate();
	}

	// deactive the root node
	if (this == master_->root())
		master_->rootDualBound(dualBound_);
}


int Sub::_setByLogImp(bool &newValues)
{
	Logger::ilout(Logger::Level::Minor) << "Setting Variables by Logical Implications: " << std::flush;
	// call the virtual function to set variables by logical implications
	ArrayBuffer<int>        variables(nVar(),false);
	ArrayBuffer<FSVarStat*> status(nVar(),false);

	setByLogImp(variables, status);

	// check for contradictions and variables set to new values
	int contra = 0;
	bool lNewValues;

	newValues = false;

	const int nVariables = variables.size();

	for (int i = 0; i < nVariables; i++) {
		contra = set(variables[i], status[i], lNewValues);
		if (contra) break;
		if (lNewValues) newValues = true;
	}

	// Sub::_setByLogImp(): clean up and return
	for (int i = 0; i < nVariables; i++)
		delete status[i];

	if (contra)
		Logger::ilout(Logger::Level::Minor) << "contradiction found" << std::endl;
	else
		Logger::ilout(Logger::Level::Minor) << nVariables << " variables set" << std::endl;

	return contra;
}


Sub::PHASE Sub::cutting ()
{
	// Sub::cutting(): local variables
	/* Before we are going to branch we would like to remove (e.g., non-binding)
	*   constraints. Such
	*   final modifications can be performed in the function
	*   \a prepareBranching(). If in this function the problem is modified,
	*   the variable \a lastIteration becomes \a true and now we perform the
	*   branching indeed. Only for convenience we modify the active constraints
	*   at the beginning of the cutting plane algorithm in this case.
	*/
	bool newValues;  //!< \a true if variable fix or set to new value
	bool lastIteration = false;

	int nVarRemoved;
	int nConRemoved;
	int nVarAdded;
	int nConAdded;
	//bool doFixSet = true;

	for(;;) {
		// add and remove variables and constraints
		/* Added/removed variables/constraints are not directly added/removed but
		*   stored in a buffer. Here, at the beginning of the inner loop of the
		*   cutting plane algorithm we update the active constraints and variables
		*   and the linear program. If more constraints/variables are buffered than
		*   actually should be added, we select the best ones if possible.
		*/

		// test if feasibility of basis could be destroyed
		/* This test does not check all possibilities, e.g., variables
		*   might have been fixed, and hence, destroy the feasibility of the basis.
		*/

		if (addVarBuffer_->number() && addConBuffer_->number()) {
			Logger::ilout(Logger::Level::Minor) << "Sub::cutting(): WARNING: adding variables and constraints" << std::endl
			 << "                         basis might become infeasible" << std::endl;
		}

		if (removeVarBuffer_->size() && removeConBuffer_->size()) {
			Logger::ilout(Logger::Level::Minor) << "Sub::cutting(): WARNING: removing variables and constraints" << std::endl
			 << "                         basis might become infeasible" << std::endl;
		}

		Logger::ilout(Logger::Level::Medium) << std::endl << "Update the Problem:" << std::endl;

		// remove all buffered constraints
		if (removeConBuffer_->size()) {
			nConRemoved = _removeCons(*removeConBuffer_);
			removeConBuffer_->clear();
			Logger::ilout(Logger::Level::Medium) << "\tremoved constraints: " << nConRemoved << std::endl;
		}
		else
			nConRemoved = 0;

		// remove all buffered variables
		if (removeVarBuffer_->size()) {
			nVarRemoved = _removeVars(*removeVarBuffer_);
			removeVarBuffer_->clear();
			Logger::ilout(Logger::Level::Medium) << "\tremoved variables:   " << nVarRemoved << std::endl;
		}
		else
			nVarRemoved = 0;

		// select constraints from the buffer and add them
		/* The function \a _selectCons() tries to select the best constraints
		*   of the buffered ones if more constraints have been generated than should be
		*   added.
		*/
		if (addConBuffer_->number()) {
			ArrayBuffer<PoolSlot<Constraint, Variable>*> newCons(addConBuffer_->number(),false);

			_selectCons(newCons);
			nConAdded = addCons(newCons);
			lastIterConAdd_ = nIter_;
			Logger::ilout(Logger::Level::Medium) << "\tadded constraints:   " << nConAdded << std::endl;
		}
		else
			nConAdded = 0;

		// select variables from the buffer and add them
		/* The function \a _selectVars() tries to select the best variables
		*   of the buffered ones if more variables have been generated than should be
		*   added.
		*/

		if (addVarBuffer_->number()) {
			// check if there are non-liftable constraints
			/* If variables are added but non-liftable constraints are present, then
			*   we cannot generate the columns correctly.
			*/
#ifdef OGDF_HEAVY_DEBUG
			const int nConstraints = nCon();

			for (int i = 0; i < nConstraints; i++) {
				if (!constraint(i)->liftable()) {
					Logger::ifout() << "Sub::cutting(): adding variables, where constraint ";
					Logger::ifout() << i << " cannot be lifted" << std::endl;
				}
			}
#endif

			ArrayBuffer<PoolSlot<Variable, Constraint> *> newVars(addVarBuffer_->number(),false);

			_selectVars(newVars);
			nVarAdded = addVars(newVars);
			lastIterVarAdd_ = nIter_;
			Logger::ilout(Logger::Level::Medium) << "\tadded variables:     " << nVarAdded << std::endl;
		}
		else
			nVarAdded = 0;


		// decide whether to use an approximate solver or not
		if (master_->solveApprox() && solveApproxNow() && !forceExactSolver_)
			lpMethod_ = LP::Approximate;
		else
			lpMethod_ = chooseLpMethod(nVarRemoved, nConRemoved, nVarAdded, nConAdded);


		// is this the last iteration before \a Branching?
		/* If we entered the cutting plane algorithm only to remove constraints
		*   before the branching is performed, we do not solve the LP-relaxation.
		*/
		if (lastIteration) return Branching;

		// solve the LP-relaxation
		/* If the function \a solveLp() returns 1, then the linear program is infeasible,
		*   also in respect to possibly inactive variables. If it returns 2, then
		*   the linear program is infeasible, but inactive variables have been
		*   generated. Therefore we iterate.

		*   If the function \a _pricing() returns a nonzero \a status, then the variables
		*   have been added. Hence, we iterate. Otherwise, the LP-solution is a dual
		*   bound for the subproblem and we can check the guarantee requirements.
		*/
		++nIter_;

		//!< return status of some called functions
		int status = solveLp();
		if (status == 1) return Fathoming;
		if (status == 2) continue;

		if (Logger::is_ilout(Logger::Level::Minor))
		{
			// output a line about the linear program
			Logger::ifout() << setw(7) << master_->nSub() << " "
				<< setw(7) << master_->openSub()->number() << "  "
				<< setw(8) << id_ << " "
				<< setw(7) << nIter_ << " "
				<< setw(10) << lp_->value() << " "
				<< setw(10) << master_->dualBound() << " ";
			if (master_->feasibleFound())
				Logger::ifout() << setw(10) << master_->primalBound() << std::endl;
			else
				Logger::ifout() << setw(10) << "---" << std::endl;

		}


		if (master_->primalViolated(dualRound(lp_->value()))) {
			status = _pricing(newValues);
			if (status)
				continue;
			// if the last LP was solved approximate, switch to the exact solver
			// and iterate
			if( lastLP_ == LP::Approximate ) {
				forceExactSolver_ = true;
				lpMethod_ = LP::Dual;
				continue;
			}
			return Fathoming;
		}

		// count the number of discrete variables being fractional
		int nFractional = 0;
		int nDiscrete   = 0;
		double frac;

		const int nVariables = nVar();

		for (int i = 0; i < nVariables; i++) {
			if (variable(i)->discrete()) {
				++nDiscrete;
				frac = fracPart(xVal_[i]);
				if ((frac > master_->eps()) && (frac < 1.0 - master_->machineEps()))
					++nFractional;
			}
		}

		Logger::ilout(Logger::Level::Minor)  << std::endl << "\t" << nFractional << " of " << nDiscrete << " discrete variables are fractional" << std::endl;


		// make a feasibility test
		/* The function \a betterPrimal() might return \a false although we have
		*   a better feasible solution, because the primal bound might have been
		*   updated already in the function \a feasible(). This is an optional
		*   feature for the user of the framework in order to simplify the bookkeeping
		*   according to his needs. If no variables are added by the function
		*   \a _pricing(), then the LP solution is also dual feasible and we can
		*   fathom the subproblem, otherwise we continue the cutting plane algorithm.
		*/
		if (feasible()) {
			Logger::ilout(Logger::Level::Medium) << "LP-solution is feasible" << std::endl;
			if (master_->betterPrimal(lp_->value()))
				master_->primalBound(lp_->value());

			status = _pricing(newValues);
			if (status)     continue;
			return Fathoming;
		}

		// improve the primal solution
		/* Even if the function \a _improve() returns a nonzero value indicating
		*   that a better solution has been found, we check if it is better than
		*   the current primal bound, as the primal bound might have been
		*   already updated during the application of the primal heuristics.
		*   Like in the function \a feasible() this is an optional feature to simplify
		*   the bookkeeping of the user.

		*   If we have found a better solution we reset the tailing off control
		*   because the subproblem seems to be promising.

		*   It is not unusual that inactive variables are added during the
		*   application of primal heuristics. In this case we go immediately to
		*   the beginning of the cutting plane loop without separating variables
		*   are constraints.
		*/
		double primalValue;  //!< value of a feasible solution found by primal heuristics

		status = _improve(primalValue);

		if (status && master_->betterPrimal(primalValue))
			master_->primalBound(primalValue);

		if (status) {
			tailOff_->reset();
			if (master_->primalViolated(dualRound(lp_->value()))) {
				status = _pricing(newValues);
				if (status)       continue;
				return Fathoming;
			}
		}

		if (addVarBuffer_->number()) continue;


		// test some minor termination criteria
		/* Note, if \a pausing() returns \a true, then we enter the
		*   \a Branching phase but there no subproblems are generated.
		*/
		bool terminate = false;  //!< becomes \a true if one of the criteria is satisfied
		bool forceFathom = false;  //!< becomes \a true if fathoming should be forced

		// check if problem specific fathoming criteria is satisfied
		/* The default implementation of \a exceptionFathom() returns always \a false.
		*/
		if (exceptionFathom()) {
			Logger::ilout(Logger::Level::Medium) << "exceptionFathom(): try fathoming.";
			terminate = true;
			forceFathom = true;
			master_->status(Master::ExceptionFathom);
		}

		// check if problem specific branching criteria is satisfied
		/* The default implementation of \a exceptionBranch() returns always \a false.
		*/
		if (exceptionBranch()) {
			Logger::ilout(Logger::Level::Medium) << "exceptionBranch(): try branching." << std::endl;
			terminate = true;
		}

		// check if maximal CPU time is exceeded
		if (!terminate &&
			master_->totalTime_.exceeds(master_->maxCpuTime()))
		{
			Logger::ilout(Logger::Level::Medium) << "Maximal CPU time " << master_->maxCpuTimeAsString() << " exceeded" << std::endl
			 << "Stop subproblem optimization." << std::endl;
			master_->status(Master::MaxCpuTime);
			terminate = true;
			forceFathom = true;
		}

		// check if maximal elapsed time is exceeded
		if (!terminate &&
			master_->totalCowTime_.exceeds(master_->maxCowTime()))
		{
			Logger::ilout(Logger::Level::Medium) << "Maximal elapsed time "
				<< master_->maxCowTimeAsString() << " exceeded" << std::endl
				<< "Stop subproblem optimization." << std::endl;
			master_->status(Master::MaxCowTime);
			terminate = true;
			forceFathom = true;
		}

		// check if there is a tailing-off effect
		if (tailOff_->tailOff()) {
			Logger::ilout(Logger::Level::Medium) << "Try to tail off subproblem processing" << std::endl;
			terminate = tailingOff();
			if (!terminate) {
				Logger::ilout(Logger::Level::Medium) << "problem specific resolution: no branching enforced" << std::endl;
				tailOff_->reset();
			}
		}

		// should we pause the subproblem
		if (!terminate && pausing()) {
			Logger::ilout(Logger::Level::Medium) << "Try to pause subproblem" << std::endl;
			terminate = true;
		}

		// check if the iteration limit is reached
		if (!terminate && (maxIterations_ > 0) && (nIter_ >= maxIterations_)) {
			Logger::ilout(Logger::Level::Medium) << "Iteration limit reached in subproblem: enforce branching" << std::endl;
			terminate = true;
		}

		// price out inactive variables if a termination criterion is fulfilled
		/* The guarantee and the time limit criteria cause a fathoming of the
		*   subproblem, whereas the other criteria cause a branching.
		*   In the function \a prepareBranching() the active constraints and
		*   variables can  still be modified. In this case the modifications
		*   takes place at the beginning of the cutting plane algorithm. But nevertheless,
		*   after the modifications the linear program is not solved.
		*/
		if (terminate) {
			// if there has been tailing off while solving approximate
			// switch to the exact solver and iterate
			if (lastLP_ == LP::Approximate) {
				forceExactSolver_ = true;
				continue;
			}
			status = _pricing(newValues);
			if (status)       continue;
			if (guaranteed() || forceFathom) return Fathoming;
			if (newValues)    continue;
			status = prepareBranching(lastIteration);
			if (status) continue;
			else        return Branching;
		}


		// perform primal and dual separation
		// should we skip the separation in this subproblem
		if (master_->skippingMode() == Master::SkipByNode) {
			if ((master_->nSubSelected() - 1) % master_->skipFactor() != 0)
				return Branching;
		}
		else {
			if ((level_ - 1) % master_->skipFactor() != 0)
				return Branching;
		}

		if (primalSeparation()) {
			// perform primal separation
			/* We do not check the return status for a successful separation, but check
			*   if new constraints have been stored in the buffer \a cutBuffer_ because
			*   violated inequalities might have been generated already earlier.

			*   We eliminate constraints only if also constraints are generated, because
			*   we prefer to have the same constraint set if variables are generated
			*   by \a _pricing(). If no variables are added in \a _pricing(), then
			*   the function
			*   \a prepareBranching() performs the elimination of the constraints.
			*/
			_separate();

			if (addConBuffer_->number()) _conEliminate();
			else {
				status = _pricing(newValues);
				if (status) continue;
				else {
					if (newValues)    continue;
					if (guaranteed()) return Fathoming;
					status = prepareBranching(lastIteration);
					if (status) continue;
					else        return Branching;
				}
			}

		}
		else {  //!< dual separation
			// perform dual separation
			/* Like in the previous section for the separation we check also the buffer
			*   for new generated variables.
			*/
			_pricing(newValues);

			if (addVarBuffer_->number()) _varEliminate();
			else if (guaranteed()) return Fathoming;
			else if (newValues)    continue;
			else {
				if (_separate()) continue;
				else {
					status = prepareBranching(lastIteration);
					if (status) continue;
					else        return Branching;
				}
			}

		}

	}
}


int Sub::prepareBranching(bool &lastIteration)
{
	lastIteration = true;
	int nElim = _conEliminate();

	if (nElim) {
		lpMethod_ = LP::Primal;
		return 1;
	}
	else return 0;
}


int Sub::solveLp ()
{
	// output some infos on the linear program
	/* The "true" number of nonzeros is the number of nonzeros not including
	*   the coefficients of the eliminated variables.
	*/
	Logger::ilout(Logger::Level::Minor) << std::endl << "Solving LP " << nIter_ << std::endl
	 << "\tNumber of Constraints:  " <<  nCon() << std::endl
	 << "\tNumber of Variables  :  " <<  nVar() << "   (not eliminated "
	 << lp_->trueNCol() << ")" << std::endl
	 << "\tTrue nonzeros        :  " << lp_->trueNnz() << std::endl;

	// optimize the linear program
	LP::OPTSTAT status;

	master_->countLp();

	localTimer_.start(true);

	status = lp_->optimize(lpMethod_);
	lastLP_ = lpMethod_;

	master_->lpSolverTime_.addCentiSeconds( lp_->lpSolverTime_.centiSeconds() );
	lp_->lpSolverTime_.reset();

	master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

	if (master_->printLP())
		Logger::ilout(Logger::Level::Minor) << *lp_;

#ifdef TTT1
	string fileName = master_->problemName() + "." + to_string(master_->nLp()) + ".bas";
	if(lp_->writeBasisMatrix(fileName.c_str())) {
		Logger::ifout() << "Writing basis to file " << fileName << " failed." << std::endl;
	}
#endif

	// buffer the solution of the linear program
	if (lp_->xValStatus() != LP::Available) {
		if (!lp_->infeasible()) {
			Logger::ifout() << "Sub::solveLp(): no LP-solution available.\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::NoSolution);
		}
	}
	else {
		const int nVariables = nVar();
		for (int i = 0; i < nVariables; i++)
			xVal_[i] = lp_->xVal(i);
	}

	// buffer the dual variables of the linear program
	/* If there are constraints but the dual variables are missing we
	*   stop for safety.
	*/
	if ((lp_->yValStatus() != LP::Available) && nCon()) {
		Logger::ifout() << "Sub::solveLp(): no dual variables available.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::NoSolution);
	}
	else {
		const int nConstraints = nCon();
		for (int i = 0; i < nConstraints; i++)
			yVal_[i] = lp_->yVal(i);
	}

	// analyze the result of the linear program
	if (status == LP::Optimal) {

		// output the solution and get the basis
		/* The flag \a ignoreInTailingOff_ might have been set by the function
		*   \a ingnoreInTailingOff() such that the current LP solution is not
		*   considered in the tailing off analysis.
		*/
		Logger::ilout(Logger::Level::Medium) << std::endl << "\tLP-solution            : " << lp_->value() << std::endl
		 << "\tBest feasible solution : " << master_->primalBound()  << std::endl;

		if (ignoreInTailingOff_)
			ignoreInTailingOff_ = false;
		else
			tailOff_->update(lp_->value());

		getBase();

		return 0;

	}
	else  if (status == LP::Infeasible) {
		// try to add variables to make the linear program feasible
		/* The function \a infeasibleSub() sets the dual bound correctly (plus
		*   or minus infinity) for an infeasible subproblem.
		*/
		if (!master_->pricing()) {
			infeasibleSub();
			return 1;
		}
		if (!removeNonLiftableCons()) return 2;
		getBase();
		if (_makeFeasible ()) {
			infeasibleSub();
			return 1;
		}
		else return 2;

	}
	else {
		// stop, a severe error occurred during the solution of the LP
		Logger::ifout() << "Sub::solveLp() return status of LP::optimize() is\n" << status << " (do not know how to proceed)\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::NoSolution);
	}
}


int Sub::_makeFeasible()
{
	if (!master_->pricing()) return 1;

	Logger::ilout(Logger::Level::Minor) << "Sub::_makeFeasible()" << std::endl;

	// make the current basis global dual feasible
	/* If the variables are added we return and solve the linear program again
	*   since these variables might restore already the feasibility and we
	*   can continue this function only if the basis is dual feasible.

	*   The second argument of \a _pricing() is \a false, because no variables
	*   should be fixed and set (it is useless for an infeasible subproblem).
	*/
	bool newValues;  //!< only a dummy here

	int status = _pricing(newValues, false);
	if (status == 1)
		return 0;
	else if (status == 2) {
		Logger::ifout() << "Sub::_makeFeasible(): pricing failed due to\nnon-liftable constraints\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::MakeFeasible);
	}

	// can we fathom the subproblem?
	/* If the basis is dual feasible, then the associated objective function
	*   value is a dual bound for the optimization of the subproblem. Hence
	*   we can fathom the node if the primal bound is violated.
	*/
	if (master_->primalViolated(dualRound(lp_->value())))
		return 1;

	// emulate an iteration of the dual simplex method
	/* An inactive variable is only a candidate if it is not implicitly set 0, i.e.,
	*   let \f$l\f$ be the objective function value of the linear program, \f$p\f$ be
	*   the value of the best known primal feasible solution, and \f$r_e\f$ the
	*   reduced cost of the variable \f$e\f$. If our problem is a maximization problem
	*   \f$e\f$ is only added if \f$l + r_e >= p\f$ holds, or if it is minimization problem
	*   \f$l + r_e <= p\f$ holds.
	*
	*   Let \f$B\f$ be the basis matrix corresponding to the dual feasible LP solution,
	*   at which the primal infeasibility was detected. For each candidate variable
	*   \f$e\f$ let \f$a_e\f$ be the column of the constraint matrix corresponding to \f$e\f$
	*   and solve the system \f$B z = a_e\f$. Let \f$z_b\f$ be the component of \f$z\f$
	*   corresponding to basis variable \f$b\f$. Activating \f$e\f$ "reduces some
	*   infeasibility" if one of the following holds, where \f$l_b\f$ and \f$u_b\f$
	*   are the local lower and upper bounds of variable \f$b\f$.
	*
	*   \shortitem{--} \f$b\f$ is a structural variable (i.e., not a slack variable) and
	*   \f[ x_b < l_b \hbox{\rm\ and } z_b < 0\f]
	*   or
	*   \f[ x_b > u_b \hbox{\rm\ and } z_b > 0\f]
	*
	*   \shortitem{--} \f$b\f$ is a slack variable and
	*   \f[ x_b < 0 \hbox{\rm\ and } z_b < 0.\f]
	*
	*   We refer the reader to \ref{Pad95} for an excellent description of the
	*   dual simplex method.
	*/
	bInvRow_ = new double[nCon()];

	status = lp_->getInfeas(infeasCon_, infeasVar_, bInvRow_);

	if (status) {
		Logger::ifout() << "Sub::_makeFeasible(): lp_->getInfeas() failed\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::MakeFeasible);
	}

	status = makeFeasible();

	delete bInvRow_;
	bInvRow_ = nullptr;

	if (status) return 1;
	else        return 0;
}


bool Sub::goodCol(
	Column &col,
	Array<double> &row,
	double x,
	double lb,
	double ub)
{
	double p = 0.0;

	const int nnz = col.nnz();

	for (int i = 0; i < nnz; i++)
		p += col.coeff(i) * row[col.support(i)];

	if (x < lb) {
		if (p < -master_->eps()) return true;
		else                  return false;
	}
	else if (x > ub) {
		if (p > master_->eps()) return true;
		else                 return false;
	}
	else {
		Logger::ifout() << "Sub::goodCol(): variable is feasible!?\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Unknown);
	}
}


int Sub::_pricing(bool &newValues, bool doFixSet)
{
	int nNew = 0;

	newValues = false;

	if (master_->pricing()) {

		if (!removeNonLiftableCons()) return 2;

		Logger::ilout(Logger::Level::Minor) << std::endl << "Price out Inactive Variables" << std::endl;

		localTimer_.start(true);
		nNew = pricing();
		master_->pricingTime_.addCentiSeconds(localTimer_.centiSeconds());
	}

	if (nNew) {
		if(doFixSet && fixAndSetTime())
			fixing(newValues);  //!< only with old candidates
	} else {
		if (betterDual(lp_->value())) dualBound(dualRound(lp_->value()));
		if (doFixSet && fixAndSetTime()) {
			if (master_->primalViolated(dualBound()))
				fixing(newValues);
			else
				fixAndSet(newValues);
		}
	}

	if (nNew) return 1;
	else      return 0;
}


bool Sub::primalSeparation()
{
	if (master_->cutting()) {
		if (master_->pricing()) {
			if (addConBuffer_->number()) return true;
			int pricingFrequency = master_->pricingFreq();

			if (pricingFrequency && nIter_ % pricingFrequency == 0)
				return false;
			else
				return true;
		}
		else return true;
	}
	else return false;
}


double Sub::dualRound(double x)
{
	if (master_->objInteger()) {
		if (master_->optSense()->max())
			return floor(x + master_->eps());
		else
			return ceil(x  - master_->eps());
	}
	else return x;
}


bool Sub::guaranteed() const
{
	double lb = lowerBound();

	if (fabs(lb) < master_->machineEps()){
		if (fabs(upperBound()) < master_->machineEps()) return true;
		else                                            return false;
	}

	if (guarantee() + master_->machineEps() < master_->requiredGuarantee()) {
		Logger::ilout(Logger::Level::Medium) << "Subproblem guarantee reached" << std::endl;
		master_->status(Master::Guaranteed);
		return true;
	}
	else
		return false;
}


double Sub::guarantee() const
{
	double lb = lowerBound();

	if (fabs(lb) < master_->machineEps()) {
		if (fabs(upperBound()) < master_->machineEps()) return 0.0;
		else {
			Logger::ifout() << "Sub::guarantee(): cannot compute guarantee\nwithh lower bound 0\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Guarantee);
		}
	}

	return fabs((upperBound() - lb)/lb * 100.0);
}


bool Sub::ancestor(const Sub *sub) const
{
	const Sub *current = sub;

	for(;;) {
		if (this == current) return true;
		if (current == master_->root()) break;
		current = current->father();
	}

	return false;
}


bool Sub::removeNonLiftableCons()
{
	if (!genNonLiftCons_) return true;

	int nNonLiftable = 0;

	const int nConstraints = nCon();

	for (int i = 0; i < nConstraints; i++) {
		if (!constraint(i)->liftable()) {
			removeCon(i);
			nNonLiftable++;
		}
	}

	genNonLiftCons_ = false;

	if (nNonLiftable) {
		Logger::ilout(Logger::Level::Medium) << "Removing " << nNonLiftable << " non-liftable constraints" << std::endl;
		lpMethod_ = LP::Primal;
		return false;
	}
	return true;
}


LP::METHOD Sub::chooseLpMethod(
	int nVarRemoved,
	int nConRemoved,
	int nVarAdded,
	int nConAdded)
{
	LP::METHOD lpMethod = LP::Primal;

	if (nIter_ == 0) {
		if (this == master_->root())
			//lpMethod = LP::BarrierAndCrossover;
				lpMethod = LP::Primal;
		else
			lpMethod = LP::Dual;
	}

	if (nConAdded)
		lpMethod = LP::Dual;
	else if (nConRemoved)
		lpMethod = LP::Primal;

	if (nVarAdded)
		lpMethod = LP::Primal;
	else if (nVarRemoved)
		lpMethod = LP::Dual;

	if (nConAdded && nVarAdded)
		//lpMethod = LP::BarrierAndCrossover;
		lpMethod = LP::Primal;

	return lpMethod;
}


void Sub::removeVars(ArrayBuffer<int> &remove)
{
	const int nRemove = remove.size();

	for (int i = 0; i < nRemove; i++)
		removeVarBuffer_->push(remove[i]);
}


void Sub::_selectVars(ArrayBuffer<PoolSlot<Variable, Constraint>*> &newVars)
{
	selectVars();
	addVarBuffer_->sort(master_->maxVarAdd());
	addVarBuffer_->extract(master_->maxVarAdd(), newVars);
}


void Sub::_selectCons(ArrayBuffer<PoolSlot<Constraint, Variable> *> &newCons)
{
	selectCons();
	addConBuffer_->sort(master_->maxConAdd());
	addConBuffer_->extract(master_->maxConAdd(), newCons);
}


int Sub::addCons(
	ArrayBuffer<Constraint*> &constraints,
	Pool<Constraint, Variable> *pool,
	ArrayBuffer<bool> *keepInPool,
	ArrayBuffer<double> *rank)
{
	int       status;
	int       nAdded = 0;
	bool      keepIt;
	const int nConstraints = constraints.size();

	int       lastInserted = nConstraints;

	if (pool == nullptr) pool = master_->cutPool();

	for (int i = 0; i < nConstraints; i++) {
		PoolSlot<Constraint, Variable> *slot = pool->insert(constraints[i]);
		if (slot == nullptr) {
			lastInserted = i - 1;
			break;
		}
		else {
			if (keepInPool)
				keepIt = (*keepInPool)[i];
			else
				keepIt = false;
			if (rank)
				status = addConBuffer_->insert(slot, keepIt, (*rank)[i]);
			else
				status = addConBuffer_->insert(slot, keepIt);
			if (status) {
				if (!keepIt && slot->conVar()->deletable())
					slot->removeConVarFromPool();
			}
			else
				nAdded++;
		}
	}

	// delete the constraints that could be not inserted into the pool
	if (lastInserted < nConstraints) {
		Logger::ilout(Logger::Level::Medium) << "Sub::addCons(): pool too small, deleting " << nConstraints - lastInserted << " constraints." << std::endl;

		for (int i = lastInserted + 1; i < nConstraints; i++)
			delete constraints[i];
	}

	return nAdded;
}


int Sub::addCons(
	ArrayBuffer<PoolSlot<Constraint, Variable>*> &newCons)
{
	// Sub::addCons(): local variables
	const int nNewCons = newCons.size();

	ArrayBuffer<Constraint*> cons(nNewCons,false);
	int i;

	// require the new constraints a reallocation?
	if (nCon() + nNewCons >= maxCon()) {
		int newMax = ((maxCon() + nNewCons)*11)/10 + 1;
		conRealloc(newMax);
	}

	// get the constraints from the pool slots
	for (i = 0; i < nNewCons; i++) {
		newCons[i]->conVar()->activate();
		cons.push(static_cast<Constraint*>(newCons[i]->conVar()));
	}

	// compute the average distance of the added cuts
	if (master_->showAverageCutDistance()) {
		double averageDistance;

		averageDistance = 0.0;

		for (i = 0; i < nNewCons; i++)
			averageDistance += cons[i]->distance(xVal_, actVar_);

		averageDistance /= nNewCons;

		Logger::ilout(Logger::Level::Minor) << "\taverage distance of cuts: " << averageDistance << std::endl;
	}

	// add the constraints to the active constraints and the LP
	for (i = 0; i < nNewCons; i++)
		(*slackStat_)[nCon() + i] = new SlackStat(SlackStat::Unknown);
	actCon_->insert(newCons);

	localTimer_.start(true);
	lp_->addCons(cons);
	master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

	master_->addCons(nNewCons);

	return nNewCons;
}


int Sub::addVars(
	ArrayBuffer<Variable*> &variables,
	Pool<Variable, Constraint> *pool,
	ArrayBuffer<bool> *keepInPool,
	ArrayBuffer<double> *rank)
{

	int       status;
	int       nAdded = 0;
	bool      keepIt;
	const int nVariables = variables.size();
	int       lastInserted = nVariables;


	if (pool == nullptr) pool = master_->varPool();

	for (int i = 0; i < nVariables; i++) {
		PoolSlot<Variable, Constraint> *slot = pool->insert(variables[i]);
		if (slot == nullptr) {
			lastInserted = i - 1;
			break;
		}
		else {
			if (keepInPool)
				keepIt = (*keepInPool)[i];
			else
				keepIt = false;
			if (rank)
				status = addVarBuffer_->insert(slot, keepIt, (*rank)[i]);
			else
				status = addVarBuffer_->insert(slot, keepIt);
			if (status) {
				if (!keepIt && slot->conVar()->deletable())
					slot->removeConVarFromPool();
			}
			else
				nAdded++;
		}
	}

	// delete the variables that could be not inserted into the pool
	if (lastInserted < nVariables) {
		Logger::ilout(Logger::Level::Medium) << "Sub::addVars(): pool too small, deleting " << nVariables - lastInserted << " variables." << std::endl;

		for (int i = lastInserted + 1; i < nVariables; i++)
			delete variables[i];
	}

	return nAdded;
}


int Sub::addVars(
	ArrayBuffer<PoolSlot<Variable, Constraint> *> &newVars)
{
	activateVars(newVars);
	addVarsToLp(newVars);

	tailOff_->reset();

	return newVars.size();
}


int Sub::variablePoolSeparation(
	int ranking,
	Pool<Variable, Constraint> *pool,
	double minAbsViolation)
{
	if (pool)
		return pool->separate(yVal_, actCon_, this, addVarBuffer_, minAbsViolation, ranking);
	else
		return master_->varPool()->separate(yVal_, actCon_, this, addVarBuffer_, minAbsViolation, ranking);
}


int Sub::constraintPoolSeparation(
	int ranking,
	Pool<Constraint, Variable> *pool,
	double minViolation)
{
	if (pool)
		return pool->separate(xVal_, actVar_, this, addConBuffer_, minViolation, ranking);
	else
		return master_->cutPool()->separate(xVal_, actVar_, this, addConBuffer_, minViolation, ranking);
}


bool Sub::objAllInteger() const
{
	const Variable *v;
	double x;

	const int nVariables = nVar();

	for (int i = 0; i < nVariables; i++) {
		v = variable(i);
		if (v->discrete()) {
			x = v->obj();
			if (x - floor(x) > master_->machineEps()) {
				return false;
			}
		}
		else
			return false;
	}

	Logger::ilout(Logger::Level::Medium) << "objective function values of feasible solutions are integer" << std::endl;

	return true;
}


bool Sub::integerFeasible()
{
	double frac;

	const int nVariables = nVar();

	for (int i = 0; i < nVariables; i++) {
		if (variable(i)->discrete()) {
			frac = fracPart(xVal_[i]);
			if ((frac > master_->machineEps()) && (frac < 1.0 - master_->machineEps()))
				return false;
		}
	}

	return true;
}


void Sub::ignoreInTailingOff()
{
	Logger::ilout(Logger::Level::Minor) << "\tnext LP solution ignored in tailing off" << std::endl;
	ignoreInTailingOff_ = true;
}


Sub::PHASE Sub::branching()
{
	Logger::ilout(Logger::Level::Medium) << std::endl << "Branching Phase" << std::endl << std::endl;

	// check if the maximum enumeration level is reached
	if (level_ == master_->maxLevel()) {
		Logger::ilout(Logger::Level::Medium) << "Maximum enumeration level " << master_->maxLevel() << " reached, no branching" << std::endl;
		master_->status(Master::MaxLevel);
		return Fathoming;
	}

	// check if the subproblem becomes dormant without branching
	/* Sometimes it turns out to be appropriate to stop the optimization
	*   of a specific subproblem without creating any sons but putting
	*   the node back into the list of open subproblems. Per default no
	*   pausing is performed but the virtual function \a pausing() can
	*   be redefined in derived classes.

	*   Then we check the parameter if only after processing a node
	*   several times its sons should be generated \a (delayedBranching(nOpt_).
	*   This idea
	*   is motivated by the pool separation. When such a dormant
	*   node is awaked in the meantime pool constraints might have
	*   become available which are violated by the last \a LP-solution.

	*   A subproblem can be only inserted in the set of open subproblems without
	*   branching if there are other subproblems for further processing.

	*   The statuses of the variables (\a fsVarStat, \a lpVarStat) are
	*   not deleted, when a subproblem becomes \a Dormant.
	*/
	if (pausing() || master_->delayedBranching(nOpt_))
		if (!master_->openSub()->empty()) {
			Logger::ilout(Logger::Level::Medium) << "making node dormant" << std::endl;
			master_->openSub()->insert(this);
			status_ = Dormant;
			nDormantRounds_ = 0;
			return Done;
		}

	// generate the branching rules
	/* If no branching rule is found we can fathom the subproblem.
	*   A branch rule defines the modifications of the current subproblem for
	*   a new subproblem.
	*/
	ArrayBuffer<BranchRule*> rules(nVar(),false);

	localTimer_.start(true);
	int status = generateBranchRules(rules);
	master_->branchingTime_.addCentiSeconds( localTimer_.centiSeconds() );

	if (status)
		return Fathoming;

	// generate the sons
	/* For each branch rule a new subproblem is generated.
	*/

	const int nRules = rules.size();

	Logger::ilout(Logger::Level::Medium) << "Number of new problems : " << nRules << std::endl;

	sons_ = new ArrayBuffer<Sub*>(nRules,false);

	for (int i = 0; i < nRules; i++) {
		Sub *newSub = generateSon(rules[i]);
		master_->openSub()->insert(newSub);
		sons_->push(newSub);
		master_->treeInterfaceNewNode(newSub);
	}

	status_ = Processed;

	return Done;
}


int Sub::branchingOnVariable(ArrayBuffer<BranchRule*> &rules)
{
	// select the branching variable
	int branchVar;

	int status = selectBranchingVariable(branchVar);

	if (status) {
		Logger::ilout(Logger::Level::Medium) << "no branching variable found" << std::endl;
		return 1;
	}

	if (variable(branchVar)->binary()) Logger::ilout(Logger::Level::Minor) << std::endl << "Binary ";
	else                               Logger::ilout(Logger::Level::Minor) << std::endl << "Integer ";

	Logger::ilout(Logger::Level::Minor) << "Branching Variable     : "
	 << branchVar << " (value: " << xVal_[branchVar]
	 << ", cost: " << variable(branchVar)->obj() << ") " << std::endl;

	// generate the two rules for the branching variable
	/* A binary branching variable is set to 0 in one of the
	*   two subproblems, and set to 1 in the other subproblem. For an
	*   integer branching variable we have to modify its lower and upper bound.
	*/
	if (variable(branchVar)->binary()) {
		rules.push(new SetBranchRule(master_, branchVar, FSVarStat::SetToUpperBound));
		rules.push(new SetBranchRule(master_, branchVar, FSVarStat::SetToLowerBound));
	}
	else {
		double splitVal=floor(xVal_[branchVar] + master_->eps());
		if(splitVal >= uBound(branchVar)){
			splitVal = splitVal - 1;
		}

		// [splitVal+1,ubound]
		rules.push(new BoundBranchRule(
			master_,
			branchVar,
			splitVal+1.0,
			uBound(branchVar)
			));

		// [lbound, splitVal]
		rules.push(new BoundBranchRule(
			master_,
			branchVar,
			lBound(branchVar),
			splitVal
			));
	}
	return 0;
}


int Sub::selectBranchingVariable(int &variable)
{
	// select the candidates for branching variables
	ArrayBuffer<int> candidates(master_->nBranchingVariableCandidates(),false);

	int status = selectBranchingVariableCandidates(candidates);

	if (status) return 1;

	if (candidates.size() == 1) {
		variable = candidates[0];
		return 0;
	}

#ifdef TTT

	// perform strong branching with help of Cplex internals
	/* The call of \a strongbranching() is only correct if no variables are
	*   eliminated.
	*/
	int *goodlist = new int[candidates.number()];
	double *down  = new double[candidates.number()];
	double *up    = new double[candidates.number()];

	Logger::ilout(Logger::Level::Minor) << "\t" << candidates.number() << " candidates" << std::endl;

	for (int i = 0; i < candidates.number(); i++)
		goodlist[i] = candidates[i];

	status = strongbranch(((CplexIF*) lp_)->cplexLp(), goodlist,
		candidates.number(), down, up, 100000);

	if (status) {
		Logger::ifout() << "Sub::selectBranchingVariable(): strong branching failed\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::afcUnknown);
	}

	for (int i = 0; i < candidates.number(); i++)
		Logger::ilout(Logger::Level::Minor) << "\t" << down[i] << ' ' << up[i] << std::endl;

	int bestCand = 0;
	double bestVal;
	double val;

	if (master_->optSense()->max()) {
		if (down[0] < up[0]) bestVal = up[0];
		else                 bestVal = down[0];
	}
	else {
		if (down[0] > up[0]) bestVal = up[0];
		else                 bestVal = down[0];
	}

	for (int i = 1; i < candidates.number(); i++)
		if (master_->optSense()->max()) {
			if (down[i] < up[i]) val = up[i];
			else                 val = down[i];
			if (val < bestVal) {
				bestCand = i;
				bestVal  = val;
			}
		}
		else {
			if (down[i] > up[i]) val = up[i];
			else                 val = down[i];
			if (val > bestVal) {
				bestCand = i;
				bestVal  = val;
			}
		}

		delete [] goodlist;
		delete [] up;
		delete [] down;

		Logger::ilout(Logger::Level::Minor) << "\t" << "selecting sample " << bestCand << std::endl;

		variable = candidates[bestCand];
		return 0;

#endif

		// generate the two branching rules for each candidate
		const int nCandidates = candidates.size();

		ArrayBuffer<BranchRule*> **samples = new ArrayBuffer<BranchRule*>*[nCandidates];

		for (int i = 0; i < nCandidates; i++) {
			samples[i] = new ArrayBuffer<BranchRule*>(2,false);
			samples[i]->push(new SetBranchRule(master_, candidates[i],
				FSVarStat::SetToUpperBound));
			samples[i]->push(new SetBranchRule(master_, candidates[i],
				FSVarStat::SetToLowerBound));
		}

		// evaluate the candidates and select the best ones
		int best = selectBestBranchingSample(nCandidates, samples);

		if (best == -1) {
			Logger::ifout() << "Sub::selectBranchingVariable(): internal error,\nselectBestBranchingSample returned -1\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::BranchingVariable);
		}

		variable = candidates[best];

		for (int i = 0; i < nCandidates; i++) {
			delete (*samples[i])[0];
			delete (*samples[i])[1];
			delete samples[i];
		}
		delete [] samples;

		return 0;
}


int Sub::selectBranchingVariableCandidates(ArrayBuffer<int> &candidates)
{
	int status = 0;

	if (master_->branchingStrategy() == Master::CloseHalf) {
		status = closeHalf(candidates, VarType::Binary);
		if (status)
			status = closeHalf(candidates, VarType::Integer);
		if (status)
			status = findNonFixedSet(candidates, VarType::Binary);
		if (status)
			status = findNonFixedSet(candidates, VarType::Integer);
	}
	else if (master_->branchingStrategy() == Master::CloseHalfExpensive) {
		status = closeHalfExpensive(candidates, VarType::Binary);
		if (status)
			status = closeHalfExpensive(candidates, VarType::Integer);
		if (status)
			status = findNonFixedSet(candidates, VarType::Binary);
		if (status)
			status = findNonFixedSet(candidates, VarType::Integer);
	}
	else {
		Logger::ifout() << "Sub::selectBranchingVariable(): unknown strategy\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Strategy);
	}

	return status;
}


int Sub::closeHalf(int &branchVar, VarType::TYPE branchVarType)
{
	ArrayBuffer<int> variables(1,false);

	int status = closeHalf(variables, branchVarType);

	if (status)
		return 1;
	else {
		branchVar = variables[0];
		return 0;
	}
}


int Sub::closeHalf(ArrayBuffer<int> &variables, VarType::TYPE branchVarType)
{
	// Sub::closeHalf(): check the branching variable type
	if (branchVarType == VarType::Continuous) {
		Logger::ifout() << "Sub::closeHalf(): we cannot branch on a continuous variable.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::CloseHalf);
	}

	// search fractional variables closest to \f$0.5\f$
	AbaPrioQueue<int, double> closest(variables.capacity());
	//changed uninit.value minkey to 0.
	double diff;
	double minKey = 0.;
	int    min;

	const int nVariables = nVar();
	for (int i = 0; i < nVariables; i++) {
		if ((variable(i)->varType() == branchVarType)
			&& !(*fsVarStat_)[i]->fixedOrSet()
			&& !( lBound(i) == uBound(i) ))
		{
			diff = fabs(fracPart(xVal_[i]) - 0.5);
			if (diff < 0.5 - master_->machineEps()) {
				if (closest.number() < closest.size())
					closest.insert(i, -diff);
				else {
					(void) closest.getMinKey(minKey);
					if (diff < -minKey) {
						(void) closest.extractMin(min);
						closest.insert(i, -diff);
					}
				}
			}
		}
	}

	// copy the best variables in the buffer \a variables
	while(!closest.extractMin(min))
		variables.push(min);

	if (variables.size()) return 0;
	else                  return 1;
}


int Sub::closeHalfExpensive(int &branchVar, VarType::TYPE branchVarType)
{
	ArrayBuffer<int> branchVarBuffer(1,false);

	int status = closeHalfExpensive(branchVarBuffer, branchVarType);

	if (!status) branchVar = branchVarBuffer[0];

	return status;
}


int Sub::closeHalfExpensive(
	ArrayBuffer<int> &branchVar,
	VarType::TYPE branchVarType)
{
	// local variables (Sub::closeHalfExpensive())
	AbaPrioQueue<int, double> candidates(branchVar.capacity());
	int    i;              /* loop index */
	double fraction;       /* fraction of x-value of a variable */
	double eps         = master_->machineEps();
	double oneMinusEps = 1.0 - eps;

	// check the selected branching variable type
	if (branchVarType == VarType::Continuous) {
		Logger::ifout() << "Sub::closeHalfExpensive(): we cannot branch on a continuous variable.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::CloseHalf);
	}

	// determine interval for fraction of candidates
	/* First we determine \a lower the maximal LP-value of a variable less than
	*   \f$0.5\f$ and \a upper the minimal LP-value of a variable greater than \f$0.5\f$.
	*   Finally, \a lower and \a upper are scaled.
	*/
	double lower = eps;
	double upper = oneMinusEps;

	const int nVariables = nVar();

	for (i = 0; i < nVariables; i++) {
		if ((variable(i)->varType() == branchVarType)
			&& !(*fsVarStat_)[i]->fixedOrSet()
			&& !( lBound(i) == uBound(i) ))
		{
			fraction = fracPart(xVal_[i]);

			if (fraction <= 0.5 && fraction > lower) lower = fraction;
			if (fraction >= 0.5 && fraction < upper) upper = fraction;
		}
	}

	if (lower == eps && upper == oneMinusEps) return 1;

	double scale = 0.25;

	lower   = (1.0 - scale) * lower;
	upper = upper + scale * (1.0-upper);

	// select the most expensive variables from interval
	/* Under \a cost in this context we understand the absolute value of the
	*   objective function coefficient.
	*/
	// changed uninit. Value of minCostCandidate to 0.!
	double cost;               //!< cost of current variable
	double minCostCandidate=0.;   //!< cost of worst variable in priority queue
	int    dummy;              //!< for extracting item of priority queue

	for (i = 0; i < nVariables; i++) {
		if ((variable(i)->varType() == branchVarType) && !(*fsVarStat_)[i]->fixedOrSet())
		{
			// check if this variable might a candidate
			/* We select the variable either if there are not enough candidates,
			*   otherwise, we check if its cost are higher than those of the worst
			*   element of \a candidate. In this case we replace this element with the
			*   variable \a i.
			*/
			fraction = fracPart(xVal_[i]);

			if (lower <= fraction && fraction <= upper) {
				cost     = fabs(variable(i)->obj());
				if (candidates.number() < candidates.size())
					candidates.insert(i, cost);
				else {
					if (candidates.getMinKey(minCostCandidate)) {
						Logger::ifout() << "Sub::CloseHalfExpensive(): internal error: candidate priorirty queue is empty.\n";
						OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::CloseHalf);
					}
					if (cost > minCostCandidate) {
						(void) candidates.extractMin(dummy);
						candidates.insert(i, cost);
					}
				}
			}

		}
	}

	// copy the "best" variables to \a branchVar
	if (candidates.number() == 0) {
		Logger::ifout() << "Sub::closeHalfExpensive(): where is the fractional variable?\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::CloseHalf);
	}

	while (!candidates.extractMin(dummy))
		branchVar.push(dummy);

	return 0;
}


int Sub::findNonFixedSet(int &branchVar, VarType::TYPE branchVarType)
{
	ArrayBuffer<int> variables(1,false);

	int status = findNonFixedSet(variables, branchVarType);

	if (status)
		return 1;
	else {
		branchVar = variables[0];
		return 0;
	}
}


int Sub::findNonFixedSet(
	ArrayBuffer<int> &branchVar,
	VarType::TYPE branchVarType)
{
	// Sub::findNonFixedSet: check the selected branching variable type
	if (branchVarType == VarType::Continuous) {
		Logger::ifout() << "Sub::findNonFixedSet(): we cannot branch on a\ncontinuous variable.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Unknown);
	}

	const int nVariables = nVar();

	for (int i = 0; i < nVariables; i++) {
		if ((variable(i)->varType() == branchVarType)
			&& !(*fsVarStat_)[i]->fixedOrSet()
			&& !( lBound(i) == uBound(i) ))
		{
			branchVar.push(i);
			if (branchVar.full()) return 0;
		}
	}

	if (branchVar.size()) return 0;
	else                  return 1;
}


int Sub::selectBestBranchingSample(
	int nSamples,
	ArrayBuffer<BranchRule*> **samples)
{
	// allocate memory (Sub::selectBestBranchingSample())
	Array<double> **rank = new Array<double>*[nSamples];

	for (int i = 0; i < nSamples; i++)
		rank[i] = new Array<double>(samples[i]->size());

	// compute the ranks and select the best sample
	Logger::ilout(Logger::Level::Minor) << "Computing ranks of branching samples: "<< std::endl;
	int best = 0;

	for (int i = 0; i < nSamples; i++) {
		rankBranchingSample(*(samples[i]), *(rank[i]));
		Logger::ilout(Logger::Level::Minor) << "\tSample " << i << ": ";
		for (int j = 0; j < samples[i]->size(); j++)
			Logger::ilout(Logger::Level::Minor) << (*(rank[i]))[j] << ' ';
		Logger::ilout(Logger::Level::Minor) << std::endl;
		if (i > 0 && compareBranchingSampleRanks(*(rank[best]), *(rank[i])) == -1)
			best = i;
	}

	Logger::ilout(Logger::Level::Minor) << std::endl << "Selecting branching sample " << best << "." << std::endl;

	// delete memory (Sub::selectBestBranchingSample())
	for (int i = 0; i < nSamples; i++)
		delete rank[i];
	delete [] rank;

	return best;
}


void Sub::rankBranchingSample(
	ArrayBuffer<BranchRule*> &sample,
	Array<double> &rank)
{
	const int nSample = sample.size();

	for (int i = 0; i < nSample; i++)
		rank[i] = rankBranchingRule(sample[i]);
}


double Sub::lpRankBranchingRule(BranchRule *branchRule, int iterLimit)
{
	// add the branching rule and solve the linear program
	// set the new iteration limit
	int oldIterLimit;

	if (iterLimit >= 0) {
		if (lp_->getSimplexIterationLimit(oldIterLimit)) {
			Logger::ifout() << "WARNING: ";
			Logger::ifout() << "Sub::lpRankBranchingRule(): ";
			Logger::ifout() << "getting the iteration limit of the LP-solver failed.";
			Logger::ifout() << std::endl;
			oldIterLimit = -1;
		}
		else {
			if (lp_->setSimplexIterationLimit(iterLimit)) {
				Logger::ifout() << "WARNING: ";
				Logger::ifout() << "Sub::lpRankBranchingRule(): ";
				Logger::ifout() << "setting the iteration limit of the LP-solver failed.";
				Logger::ifout() << std::endl;
				oldIterLimit = -1;
			}
		}
	}

	// load the final basis of the subproblem optimization
	Array<LPVARSTAT::STATUS> vStat(nVar());
	Array<SlackStat::STATUS> sStat(nCon());

	const int nVariables = nVar();

	for (int i = 0; i < nVariables; i++)
		vStat[i] = lpVarStat(i)->status();

	const int nConstraints = nCon();

	for (int i = 0; i < nConstraints; i++)
		sStat[i] = slackStat(i)->status();

	lp_->loadBasis(vStat, sStat);

	branchRule->extract(lp_);
	localTimer_.start(true);
	lp_->optimize(LP::Dual);
	master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

	// get the \a value of the linear program
	double value;
	if (lp_->infeasible()) {
		if (master_->optSense()->max()) value = -master_->infinity();
		else                            value =  master_->infinity();
	}
	else
		value = lp_->value();

	// remove the branching rule
	// set the iteration limit back to its old value
	if (iterLimit >= 0 && oldIterLimit >=0) {
		if (lp_->setSimplexIterationLimit(oldIterLimit)) {
			Logger::ifout() << "Sub::lpRankBranchingRule(): setting the iteration limit of LP-solver failed\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::BranchingRule);
		}
	}

	branchRule->unExtract(lp_);

	return value;
}


int Sub::compareBranchingSampleRanks(
	Array<double> &rank1,
	Array<double> &rank2)
{
	// build up a priority queue for each rank
	AbaPrioQueue<int, double> prio1(rank1.size());
	AbaPrioQueue<int, double> prio2(rank2.size());

	const int s1 = rank1.size();
	const int s2 = rank2.size();

	if (master_->optSense()->max()) {
		for (int i = 0; i < s1; i++)
			prio1.insert(i, -rank1[i]);

		for (int i = 0; i < s2; i++)
			prio2.insert(i,-rank2[i]);
	}
	else {
		for (int i = 0; i < s1; i++)
			prio1.insert(i, rank1[i]);
		for (int i = 0; i < s2; i++)
			prio2.insert(i, rank2[i]);
	}

	// prefer the rank where the minimal change is maximal
	double minKey1;
	double minKey2;

	int min1;
	int min2;

	while(!prio1.getMinKey(minKey1) && !prio2.getMinKey(minKey2)) {
		if (!master_->equal(minKey1, minKey2)) {
			if (minKey1 > minKey2) return  1;
			else                   return -1;
		}
		else {
			(void) prio1.extractMin(min1);
			(void) prio2.extractMin(min2);
		}
	}

	return 0;
}


Sub::PHASE Sub::fathoming()
{
	Logger::ilout(Logger::Level::Minor) << std::endl << "Fathoming Phase" << std::endl;

	fathom(true);

	return Done;
}


void Sub::fathom(bool reoptimize)
{
	// Sub::fathom(): output some infos
	Logger::ilout(Logger::Level::Minor) << "\tnode " << id_ << " fathomed" << std::endl;

	// reset the flags of the active variables and constraints
	/* If an active subproblem is fathomed, then the active variables
	*   and constraints have to be deactivated. This can only be done
	*   if these sets are allocated already.
	*
	*   Then, we can set the status of the subproblem to \a Fathomed.
	*/
	if (status_ == ActiveSub) {
		if (actVar_) {
			int nActVar = actVar_->number();
			for (int i = 0; i < nActVar; i++)
				(*actVar_)[i]->deactivate();
		}

		if (actCon_) {
			const int nActCon = actCon_->number();
			for (int i = 0; i < nActCon; i++)
				(*actCon_)[i]->deactivate();
		}
	}

	status_ = Fathomed;

	// update the dual bound of the subproblem
	/* If the subproblem is not a leaf of the enumeration tree, we
	*   update its dual bound with the minimal (maximal) dual bound
	*   of its sons if the optimization problem is a minimization
	*   (maximization) problem.

	*   We update the dual bound only if the sons define a better dual
	*   bound. If heuristic separation methods are applied or a
	*   reoptimization of a subproblem has been performed, then it is
	*   possible that the dual bound defined by the sons is worse than the
	*   dual bound of the subproblem.
	*/
	if (sons_) {
		double newDualBound = (*sons_)[0]->dualBound();

		if (master_->optSense()->max()) {
			const int nSons = sons_->size();
			for (int i = 1; i < nSons; i++)
				if (newDualBound < (*sons_)[i]->dualBound())
					newDualBound = (*sons_)[i]->dualBound();
		}
		else {
			const int nSons = sons_->size();
			for (int i = 1; i < nSons; i++)
				if (newDualBound > (*sons_)[i]->dualBound())
					newDualBound = (*sons_)[i]->dualBound();
		}
		if (betterDual(newDualBound))
			dualBound(newDualBound);
	}

	// delete allocated memory of the fathomed subproblem
	/* A fathomed subproblem will neither be required for the initialization of
	*   one of its sons nor in a reoptimization to determine the new candidates
	*   for fixing variables. Hence we can delete all local memory.
	*/

	if (fsVarStat_) {
		const int nVariables = nVar();
		for (int i = 0; i < nVariables; i++)
			delete (*fsVarStat_)[i];
	}

	if (lpVarStat_) {
		const int nVariables = nVar();
		for (int i = 0; i < nVariables; i++)
			delete (*lpVarStat_)[i];
	}

	delete fsVarStat_;
	fsVarStat_ = nullptr;
	delete lpVarStat_;
	lpVarStat_ = nullptr;

	delete lBound_;
	lBound_ = nullptr;
	delete uBound_;
	uBound_ = nullptr;

	if (slackStat_) {
		const int nConstraints = nCon();
		for (int i = 0; i < nConstraints; i++)
			delete (*slackStat_)[i];
		delete slackStat_;
		slackStat_ = nullptr;
	}

	delete actCon_;
	actCon_ = nullptr;
	delete actVar_;
	actVar_ = nullptr;

	delete branchRule_;
	branchRule_ = nullptr;

	// check if the root node is fathomed
	if (this == master_->root()) {
		Logger::ilout(Logger::Level::Medium) << "\t\troot node fathomed" << std::endl;
		return;
	}

	// count the number of unfathomed sons of the father
	int   nuf = 0;            //!< number of unfathomed sons of \a father_

	for (auto &brother : *father_->sons_) {
		if (brother->status_ != Fathomed) {
			++nuf;
		}
	}

	// process the father
	/* If all sons of the father are fathomed we can fathom the father
	*   recursively. If only one son of the father is unfathomed and the father
	*   is the root of the remaining \bac\ tree, then this unfathomed son
	*   becomes the new root of the remaining \bac\ tree.
	*   As in this case, there is exactly one unfathomed son, this
	*   is son \a i when the \a for-loop is left by the \a break instruction.
	*/
	if (nuf==0)
		father_->fathom(reoptimize);
	else if (nuf == 1 && father_ == master_->rRoot()) {
		int i;
		for (i = 0; i < father_->sons_->size(); i++)
			if ((*(father_->sons_))[i]->status_ != Fathomed) break;

		master_->rRoot((*(father_->sons_))[i], reoptimize);
	}
}


int Sub::fixAndSet(bool &newValues)
{
	int  status;
	bool lNewValues;

	newValues = false;

	status = fixing(lNewValues, true);
	if (lNewValues) newValues = true;
	if (status)     return 1;

	status = setting(lNewValues);
	if (lNewValues) newValues = true;
	if (status)     return 1;

	return 0;
}


int Sub::fixing(bool &newValues, bool saveCand)
{
	int  status;
	bool lNewValues = false;

	newValues = false;

	status = fixByRedCost(lNewValues, saveCand);
	if (lNewValues) newValues = true;
	if (status)     return 1;

	status = _fixByLogImp(lNewValues);
	if (lNewValues) newValues = true;
	if (status)     return 1;

	return 0;
}


int Sub::setting(bool &newValues)
{
	int  status;
	bool lNewValues;

	newValues = false;

	status = setByRedCost();
	if (status) return 1;

	status = _setByLogImp(lNewValues);
	if (lNewValues) newValues = true;
	if (status)     return 1;

	return 0;
}


int Sub::fixByRedCost(bool &newValues, bool saveCand)
{
	if (!master_->fixSetByRedCost()) return 0;

	newValues = false;

	if (this == master_->rRoot() && saveCand)
		master_->fixCand()->saveCandidates(this);

	master_->fixCand()->fixByRedCost(addVarBuffer_);

	// update the global fixings also in the subproblem
	/* If a contradiction between a global fixing a the local status of the
	*   variable is detected we immediately stop such that the subproblem can
	*   be fathomed.
	*/
	bool       lNewValues;

	const int nVariables = nVar();

	for (int i = 0; i < nVariables; i++) {
		FSVarStat *global = variable(i)->fsVarStat();
		if (global->fixed() && global->status() != (*fsVarStat_)[i]->status()) {
			if (fix(i, global, lNewValues))
				return 1;
			if (lNewValues)
				newValues = true;
		}
	}

	return 0;
}


int Sub::_fixByLogImp(bool &newValues)
{
	Logger::ilout(Logger::Level::Minor) << "Fixing Variables by Logical Implications:  ";
	// call the virtual function to fix variables by logical implications
	ArrayBuffer<int>            variables(nVar(),false);
	ArrayBuffer<FSVarStat*> status(nVar(),false);

	fixByLogImp(variables, status);

	// check if \a fixByLogImp() caused contradictions or fixed variables to new values
	int contra = 0;
	bool lNewValues;

	newValues = false;

	const int nVariables = variables.size();

	for (int i = 0; i < nVariables; i++) {
		int stat = fix(variables[i], status[i], lNewValues);
		if (stat)       contra    = 1;
		if (lNewValues) newValues = true;
	}

	// Sub::_fixByLogImp(): clean up and return
	for (int i = 0; i < nVariables; i++)
		delete status[i];

	if (contra)
		Logger::ilout(Logger::Level::Minor) << "contradiction" << std::endl;
	else
		Logger::ilout(Logger::Level::Minor) << nVariables << " variables fixed" << std::endl;

	return contra;
}


int Sub::setByRedCost()
{
	if (!master_->fixSetByRedCost()) return 0;

	// Sub::setByRedCost(): local variables
	int  nSet = 0;  //!< number of variables set
	bool dummy =false;     //!< required to call function \a set(), no new values possible here
	int i;          //!< loop index

	Logger::ilout(Logger::Level::Minor) << "Setting Variables by Reduced Costs:        ";

	if (master_->optSense()->max()) {
		// set by reduced costs for maximization problems
		/* In maximization problems the dual bound should not fall below the
		*   primal bound. Remember, the reduced cost of a (nonbasic) variable is the
		*   change of the object function if the variable becomes basic and
		*   changes one unit from its current value. As discrete variables can
		*   take only integer values, we analyzes what would happen if the
		*   value of the variable would decrease by one unit, if it is currently
		*   at its upper bound, or increase by one unit if it is currently at its
		*   lower bound.
		*
		*   Even for integer objective function values we require a violation of
		*   at least \a master_->eps(), otherwise a variable might be set to the wrong
		*   value.
		*/
		const int nVariables = nVar();

		for (i = 0; i < nVariables; i++)
			if (variable(i)->discrete() && !variable(i)->fsVarStat()->fixed()) {
				if (lpVarStat(i)->status() == LPVARSTAT::AtUpperBound) {
					if (lp_->value() - lp_->reco(i) + master_->eps() < master_->primalBound())
					{
						if (set(i, FSVarStat::SetToUpperBound, dummy))
							return 1;
						else
							++nSet;
					}
				}
				else if (lpVarStat(i)->status() == LPVARSTAT::AtLowerBound) {
					if (lp_->value() + lp_->reco(i) + master_->eps() < master_->primalBound())
					{
						if (set(i, FSVarStat::SetToLowerBound, dummy))
							return 1;
						else
							++nSet;
					}
				}
			}
	}
	else {
		// set by reduced costs for minimization problems
		/* In minimization problems the dual bound should not exceed the
		*   primal bound.
		*/
		const int nVariables = nVar();

		for (i = 0; i < nVariables; i++)
			if (variable(i)->discrete() && !variable(i)->fsVarStat()->fixed()) {
				if (lpVarStat(i)->status() == LPVARSTAT::AtUpperBound) {
					if (lp_->value() - lp_->reco(i) - master_->eps() > master_->primalBound())
					{
						if (set(i, FSVarStat::SetToUpperBound, dummy))
							return 1;
						else
							++nSet;
					}
				}
				else if (lpVarStat(i)->status() == LPVARSTAT::AtLowerBound) {
					if (lp_->value() + lp_->reco(i) - master_->eps() > master_->primalBound())
					{
						if (set(i, FSVarStat::SetToLowerBound, dummy))
							return 1;
						else
							++nSet;
					}
				}
			}
	}

	Logger::ilout(Logger::Level::Minor) << nSet << " variables set" << std::endl;
	return 0;
}


void Sub::reoptimize()
{
	PHASE phase;  //!< current phase of the subproblem optimization

	// output a banner for the subproblem
	if(Logger::is_ilout(Logger::Level::Medium)) {
		Logger::ifout() << std::endl << "************************************************" << std::endl
		 << "Subproblem " << id_ << " on Level " << level_ << ":" << std::endl << std::endl;

		if (master_->optSense()->max()) {
			Logger::ifout() << "\tGlobal Lower Bound: " << lowerBound()       << std::endl
			 << "\tLocal  Upper Bound: " << upperBound()       << std::endl
			 << "\tGlobal Upper Bound: " << master_->upperBound() << std::endl;
		}
		else {
			Logger::ifout() << "\tLocal  Lower Bound: " << lowerBound()       << std::endl
			 << "\tGlobal Lower Bound: " << master_->lowerBound() << std::endl
			 << "\tGlobal Upper Bound: " << upperBound()       << std::endl;
		}

		Logger::ifout() << "\tCurrent Guarantee : ";
		master_->printGuarantee();
		Logger::ifout() << std::endl << std::endl << "reoptimization starts" << std::endl;
	}

	phase = _activate ();

	if (phase == Fathoming) fathomTheSubTree ();
	else {
		phase = cutting ();
		if (phase == Fathoming) fathomTheSubTree ();
	}

	_deactivate ();

	status_ = Processed;
}


/**
 * Like in the class Master we work again with primal and dual
 * bounds such that the code works both for minimization and maximization
 * problems.
 */

void Sub::dualBound(double x)
{
	if (master_->optSense()->max()) {
		if (x > dualBound_) {
			Logger::ifout() << "Warning: Sub::dualBound(): worse dual ";
			Logger::ifout() << "bound " << x << "ignored." << std::endl;
			Logger::ifout() << "Keeping old dual bound " << dualBound_ << "." << std::endl;
			return;
		}
	}
	else {
		if (x < dualBound_) {
			Logger::ifout() << "Warning: Sub::dualBound(): worse dual ";
			Logger::ifout() << "bound " << x << "ignored." << std::endl;
			Logger::ifout() << "Keeping old dual bound " << dualBound_ << "." << std::endl;
			return;
		}
	}

	dualBound_ = x;

	if (this == master_->root() && master_->betterDual(dualBound_))
		master_->dualBound(dualBound_);

	if (status_ == ActiveSub) {
		if (master_->optSense()->max())
			master_->treeInterfaceNodeBounds(id_, master_->primalBound(),
			dualBound_);
		else
			master_->treeInterfaceNodeBounds(id_, dualBound_,
			master_->primalBound());
	}

}


void Sub::maxIterations(int max)
{
	Logger::ilout(Logger::Level::Minor) << "Setting maximal number of iterations in the cutting plane phase to " << max << std::endl;
	maxIterations_ = max;
}


void Sub::getBase()
{
	if (lp_->basisStatus() == LP::Available) {
		// get the LP status of the variables
		/* There may be variables which are fixed or set but which could
		*   not be eliminated since their previous LPVARSTAT has been
		*   \a Basic. The LPVARSTAT of these variables after the solution
		*   of the linear program could cause a wrong fixing or setting.
		*   Hence we assign to them the status \a Unknown if the status
		*   is not \a Basic.
		*/

		const int nVariables = nVar();

		for (int i = 0; i < nVariables; i++) {
			LPVARSTAT::STATUS newStat = lp_->lpVarStat(i);
			if (newStat != LPVARSTAT::Eliminated) {
				if ((*fsVarStat_)[i]->fixedOrSet() && newStat != LPVARSTAT::Basic)
					(*lpVarStat_)[i]->status(LPVARSTAT::Unknown);
				else
					(*lpVarStat_)[i]->status(newStat);
			}
			else
				(*lpVarStat_)[i]->status(LPVARSTAT::Eliminated);
		}

		// get the LP status of the slack variables
		const int nActCon = actCon_->number();
		for (int i = 0; i < nActCon; i++)
			(*slackStat_)[i]->status(lp_->slackStat(i));
	}
}


int Sub::fix(int i, FSVarStat *newStat, bool &newValue)
{
	Variable *v = variable(i);

	int contra = 0;

	if (fsVarStat(i)->contradiction(newStat)) contra = 1;
	else fsVarStat(i)->status(newStat);

	if (!v->fsVarStat()->fixed())
		master_->newFixed(1);

	v->fsVarStat()->status(newStat);

	// is variable fixed to a new value
	double x = xVal_[i];
	if ((newStat->status() == FSVarStat::FixedToLowerBound
		&& x > lBound(i) + master_->eps()) ||
		(newStat->status() == FSVarStat::FixedToUpperBound
		&& x < uBound(i) - master_->eps()) ||
		(newStat->status() == FSVarStat::Fixed
		&& !(master_->equal(x, newStat->value())))
		)
		newValue = true;
	else
		newValue = false;

	// update the bounds of the fixed variable
	double newBound = fixSetNewBound(i);

	(*lBound_)[i] = newBound;
	(*uBound_)[i] = newBound;

	variable(i)->lBound(newBound);
	variable(i)->uBound(newBound);

	updateBoundInLp(i);

	return contra;
}


int Sub::set(int i, FSVarStat *newStat, bool &newValue)
{
	return set(i, newStat->status(), newStat->value(), newValue);
}


int Sub::set(int i, FSVarStat::STATUS newStat, bool &newValue)
{
	if (newStat == FSVarStat::Set) {
		Logger::ifout() << "Sub::set() no value specified for status Set \n setting to value not implemented\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Unknown);
	}

	return set (i, newStat, 0.0, newValue);
}


int Sub::set(int i, FSVarStat::STATUS newStat, double value, bool &newValue)
{
	Variable *v = variable(i);

	if (v->fsVarStat()->contradiction(newStat, value)) return 1;
	else {
		(*fsVarStat_)[i]->status(newStat, value);

		// is variable set to a new value
		// If a variable is fixed according to logical implications before
		// the subproblem is processed, then no \a lp_ is available.

		if (lp_ == nullptr) newValue = false;
		else {
			double x = xVal_[i];
			if ((newStat == FSVarStat::SetToLowerBound
				&& x > (*lBound_)[i] + master_->eps())   ||
				(newStat == FSVarStat::SetToUpperBound
				&& x < (*uBound_)[i] - master_->eps())   ||
				(newStat == FSVarStat::Set
				&& !(master_->equal(x, value)))            )
				newValue = true;
			else
				newValue = false;
		}

		// update the bounds of the set variables
		// The function \a updateBoundInLp() checks if a linear program is present
		// in the subproblem. The bounds in the linear program can only be
		// changed if the variable is not eliminated. However if the variable
		// is set to a different bound, then we would have detected a contradiction.

		double newBound = fixSetNewBound(i);

		(*lBound_)[i] = newBound;
		(*uBound_)[i] = newBound;

		updateBoundInLp(i);

		return 0;
	}
}


void Sub::updateBoundInLp(int i)
{
	if (lp_ == nullptr || lp_->eliminated(i)) return;

	double newBound = (*lBound_)[i];

	lp_->changeLBound(i, newBound);
	lp_->changeUBound(i, newBound);
}


double Sub::fixSetNewBound(int i)
{
	switch ((*fsVarStat_)[i]->status()) {
	case FSVarStat::SetToLowerBound:
		return (*lBound_)[i];
	case FSVarStat::FixedToLowerBound:
		return variable(i)->lBound();
	case FSVarStat::SetToUpperBound:
		return (*uBound_)[i];
	case FSVarStat::FixedToUpperBound:
		return variable(i)->uBound();
	case FSVarStat::Set:
	case FSVarStat::Fixed:
		return (*fsVarStat_)[i]->value();
	default:
		Logger::ifout() << "Sub::fixSetNewBound(): variable neither fixed nor set\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::FixSet);
	}
}


int Sub::_conEliminate()
{
	ArrayBuffer<int> eliminate(nCon(),false);

	conEliminate(eliminate);

	removeCons(eliminate);

	Logger::ilout(Logger::Level::Minor) << eliminate.size() << " constraints eliminated" << std::endl;

	return eliminate.size();
}


void Sub::conEliminate(ArrayBuffer<int> &remove)
{
	switch (master_->conElimMode()) {
	case Master::NonBinding:
		nonBindingConEliminate(remove);
		break;
	case Master::Basic:
		basicConEliminate(remove);
		break;
	default:
		break;
	}
}


void Sub::nonBindingConEliminate(ArrayBuffer<int> &remove)
{
	const int conElimAge = master_->conElimAge() - 1;

	const int nConstraints = nCon();

	for (int i = 0; i < nConstraints; i++)
		if ((*actCon_)[i]->dynamic()) {
			if (fabs(lp_->slack(i)) > master_->conElimEps()) {
				if (actCon_->redundantAge(i) >= conElimAge) {
					remove.push(i);
				}
				else
					actCon_->incrementRedundantAge(i);
			}
			else
				actCon_->resetRedundantAge(i);
		}
}


void Sub::basicConEliminate(ArrayBuffer<int> &remove)
{
	const int conElimAge   = master_->conElimAge() - 1;
	const int nConstraints = nCon();

	for (int i = 0; i < nConstraints; i++)
		if ((*actCon_)[i]->dynamic()) {
			if ((*slackStat_)[i]->status() == SlackStat::Basic) {
				if (actCon_->redundantAge(i) >= conElimAge)
					remove.push(i);
				else
					actCon_->incrementRedundantAge(i);
			}
			else
				actCon_->resetRedundantAge(i);
		}
}


int Sub::_varEliminate()
{
	ArrayBuffer<int> eliminate(nVar(),false);

	varEliminate(eliminate);

	removeVars(eliminate);

	Logger::ilout(Logger::Level::Minor) << eliminate.size() << " variables eliminated" << std::endl;

	return eliminate.size();
}


void Sub::varEliminate(ArrayBuffer<int> &remove)
{
	if (master_->varElimMode() == Master::ReducedCost)
		redCostVarEliminate(remove);
}


void Sub::redCostVarEliminate(ArrayBuffer<int> &remove)
{
	bool max = master_->optSense()->max();
	const int  varElimAge = master_->varElimAge() - 1;
	const double eps = master_->machineEps();

	const int nVariables = nVar();
	for (int i = 0; i < nVariables; i++)
		if (variable(i)->dynamic() && !(*fsVarStat_)[i]->fixedOrSet() && fabs(xVal_[i]) < eps)
		{
			bool bad = false;

			if (!lpVarStat(i)->basic()) {
				if (max) {
					if (lp_->reco(i) < -master_->varElimEps()) bad = true;
				}
				else {
					if (lp_->reco(i) > master_->varElimEps()) bad = true;
				}
			}

			if (bad) {
				if (actVar_->redundantAge(i) >= varElimAge)
					remove.push(i);
				else
					actVar_->incrementRedundantAge(i);
			}
			else
				actVar_->resetRedundantAge(i);
		}
}


void Sub::fathomTheSubTree()
{
	Logger::ilout(Logger::Level::Medium) << "fathom complete subtree" << std::endl;

	if (status_ != Fathomed) {
		if (status_ == Dormant || status_ == Unprocessed)
			master_->openSub()->remove(this);

		if (sons_) {
			for (auto &elem : *sons_) {
				elem->fathomTheSubTree();
			}
		} else {
			fathom(false); //!< no reoptimization desired
		}
	}
}


int Sub::_separate()
{
	// should we separate cuts at all?
	if (!master_->cutting()) return 0;

	// separate cuts
	int nCuts;

	Logger::ilout(Logger::Level::Minor) << std::endl << "Separation of Cutting Planes" << std::endl;

	localTimer_.start(true);
	nCuts = separate();
	master_->separationTime_.addCentiSeconds( localTimer_.centiSeconds() );

	return nCuts;
}


int Sub::separate()
{
	Logger::ilout(Logger::Level::Minor) << std::endl << "no separation implemented" << std::endl;
	return 0;
}


int Sub::_improve(double &primalValue)
{
	if (master_->pbMode() != Master::NoPrimalBound) return 0;
	Logger::ilout(Logger::Level::Minor) << std::endl << "Apply Primal Heuristic" << std::endl;

	localTimer_.start(true);

	int status = improve(primalValue);

	master_->improveTime_.addCentiSeconds( localTimer_.centiSeconds() );

	return status;
}


int Sub::improve(double & /* primalValue */)
{
	Logger::ilout(Logger::Level::Minor) << std::endl << "no primal heuristic implemented" << std::endl;
	return 0;
}


void Sub::infeasibleSub()
{
	Logger::ilout(Logger::Level::Medium) << "infeasible subproblem" << std::endl;

	if (master_->optSense()->max())
		dualBound_ = -master_->infinity();
	else
		dualBound_ =  master_->infinity();

	master_->treeInterfaceNodeBounds(id_, lowerBound(), upperBound());
}


bool Sub::infeasible()
{
	if (master_->optSense()->max())
		return (dualBound_ == -master_->infinity());
	else
		return (dualBound_ ==  master_->infinity());
}


void Sub::activateVars(ArrayBuffer<PoolSlot<Variable, Constraint> *> &newVars)
{
	// perform a reallocation if required by the new variables
	int n = nVar();

	if (n + newVars.size() >= maxVar()) {
		int newMax = ((maxVar() + newVars.size())*11)/10 + 1;
		varRealloc(newMax);
	}

	// add the new variables to the data structures of the subproblem
	Variable *v;

	const int nNewVars = newVars.size();

	for (int i = 0; i < nNewVars; i++) {
		v = static_cast<Variable *>(newVars[i]->conVar());

		(*fsVarStat_)[n + i] = new FSVarStat(v->fsVarStat());
		(*lpVarStat_)[n + i] = new LPVARSTAT(LPVARSTAT::Unknown);
		(*lBound_)[n + i]    = v->lBound();
		(*uBound_)[n + i]    = v->uBound();

		v->activate();
	}

	actVar_->insert(newVars);

	master_->addVars(nNewVars);
}


void Sub::addVarsToLp(ArrayBuffer<PoolSlot<Variable, Constraint>*> &newVars,
						  ArrayBuffer<FSVarStat*> *localStatus)
{
	// Sub::addVarsToLp(): local variables
	const int nNewVars = newVars.size();
	ArrayBuffer<Variable*>  vars(nNewVars,false);
	ArrayBuffer<FSVarStat*> stat(nNewVars,false);
	ArrayBuffer<double>     lb(nNewVars,false);
	ArrayBuffer<double>     ub(nNewVars,false);

	// get the new variables together with their status and bounds
	for (int i = 0; i < nNewVars; i++) {
		Variable *v = static_cast<Variable *>(newVars[i]->conVar());

		vars.push(v);

		if (localStatus) {
			if (v->fsVarStat()->contradiction((*localStatus)[i])) {
				Logger::ifout() << "Sub::addVarsToLp(): local status contradicts global status\n";
				OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::AddVar);
			}
			stat.push((*localStatus)[i]);
		}
		else stat.push(v->fsVarStat());

		lb.push(v->lBound());
		ub.push(v->uBound());
	}

	// add the new variables to the linear program
	/* If more than one variable is added we generate the expanded format
	*   of the constraints, such that the columns can be determined more
	*   efficiently. However, this threshold is completely experimental
	*   and problem specific!
	*/
	const int nConstraints = nCon();
	if (vars.size() > 1)
		for (int i = 0; i < nConstraints; i++)
			constraint(i)->_expand();

	localTimer_.start(true);

	lp_->addVars(vars, stat, lb, ub);

	master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

	if (vars.size() > 1)
		for (int i = 0; i < nConstraints; i++) constraint(i)->_compress();
}


int Sub::_removeVars(ArrayBuffer<int> &remove)
{
	const int nRemove = remove.size();

	if (nRemove) {
		// sort the variables which are removed
		/* The following functions removing the variables from the data structures
		*   of the subproblem require the variables sorted in increasing order.
		*   This sorting can be performed in linear time.
		*/
		Array<bool> marked(0,nVar()-1, false);

		for (int i = 0; i < nRemove; i++)
			marked[remove[i]] = true;

		ArrayBuffer<int> removeSorted(nRemove,false);

		const int nVariables = nVar();
		for (int i = 0; i < nVariables; i++)
			if (marked[i]) removeSorted.push(i);

		// remove the variables from the subproblem
		const int nRemoveSorted = removeSorted.size();

		localTimer_.start(true);
		lp_->removeVars(removeSorted);
		master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

		for (int i = 0; i < nRemoveSorted; i++) {
			delete (*fsVarStat_)[removeSorted[i]];
			delete (*lpVarStat_)[removeSorted[i]];
		}
		for (int i = 0; i < nRemoveSorted; i++)
			(*actVar_)[removeSorted[i]]->deactivate();

		actVar_->remove(removeSorted);

		fsVarStat_->leftShift(removeSorted);
		lpVarStat_->leftShift(removeSorted);
		uBound_->leftShift(removeSorted);
		lBound_->leftShift(removeSorted);

		master_->removeVars(nRemoveSorted);
	}

	return remove.size();
}


void Sub::removeCons(ArrayBuffer<int> &remove)
{
	const int nRemove = remove.size();

	for (int i = 0; i < nRemove; i++)
		removeConBuffer_->push(remove[i]);
}


int Sub::_removeCons(ArrayBuffer<int> &remove)
{
	const int nRemove = remove.size();
	if (nRemove) {
		// sort the constraints which are removed
		/* The following functions to remove the variables from the data structures
		*   in the subproblem require the constraints sorted in increasing order.
		*   This sorting is performed in linear time.
		*/
		Array<bool> marked(0,nCon()-1, false);

		for (int i = 0; i < nRemove; i++)
			marked[remove[i]] = true;

		ArrayBuffer<int> removeSorted(nRemove,false);

		const int nConstraints = nCon();
		for (int i = 0; i < nConstraints; i++)
			if (marked[i]) removeSorted.push(i);

		// remove the constraints from the subproblem
		const int nRemoveSorted = removeSorted.size();
		for (int i = 0; i < nRemoveSorted; i++) {
			(*actCon_)[removeSorted[i]]->deactivate();
			delete (*slackStat_)[removeSorted[i]];
		}

		actCon_->remove(removeSorted);
		slackStat_->leftShift(removeSorted);
		localTimer_.start(true);
		lp_->removeCons(removeSorted);
		master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

		master_->removeCons(nRemoveSorted);

	}

	return nRemove;
}


void Sub::varRealloc(int newSize)
{
	actVar_->realloc(newSize);
	fsVarStat_->resize(newSize);
	lpVarStat_->resize(newSize);
	lBound_->resize(newSize);
	uBound_->resize(newSize);
	removeVarBuffer_->setCapacity(newSize);

	double *xValNew = new double[newSize];

	const int nVariables = nVar();

	for (int i = 0; i < nVariables; i++)
		xValNew[i] = xVal_[i];

	delete [] xVal_;
	xVal_ = xValNew;

	localTimer_.start(true);
	lp_->varRealloc(newSize);
	master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());
}


void Sub::conRealloc(int newSize)
{
	actCon_->realloc(newSize);
	slackStat_->resize(newSize);
	removeConBuffer_->setCapacity(newSize);
	localTimer_.start(true);
	lp_->conRealloc(newSize);
	master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

	double *yValNew = new double[newSize];

	const int nConstraints = nCon();

	for (int i = 0; i < nConstraints; i++)
		yValNew[i] = yVal_[i];

	delete [] yVal_;
	yVal_ = yValNew;
}


int Sub::initializeLp()
{
	for(;;) {
		localTimer_.start(true);
		lp_ = generateLp();
		master_->lpTime_.addCentiSeconds(localTimer_.centiSeconds());

		if (lp_->infeasible()) {
			if (_initMakeFeas()) return 1;
			else                 delete lp_;
		}
		else return 0;
	}
}


int Sub::_initMakeFeas()
{
	if (!master_->pricing()) return 1;

	// find variables which could make initial LP feasible
	// The determination of useful variables has be implemented
	// problem specifically in the function \a initMakeFeas().
	ArrayBuffer<InfeasCon*>   *infeasCon = lp_->infeasCon();
	ArrayBuffer<Variable*>  newVars(infeasCon->size(),false);
	Pool<Variable, Constraint> *pool = nullptr;

	int status = initMakeFeas(*infeasCon, newVars, &pool);

	if (status) return 1;

	// insert the variables in a pool and determine the pool slots \a newSlots
	// If not differently specified with the help of the function
	// \a initMakeFeas() we use the default variable pool for the new variables.
	const int nNewVars = newVars.size();

	ArrayBuffer<PoolSlot<Variable, Constraint> *> newSlots(nNewVars,false);

	if (pool == nullptr) pool = master_->varPool();

	for (int i = 0; i < nNewVars; i++) {
		PoolSlot<Variable, Constraint> *slot = pool->insert(newVars[i]);

		if (slot == nullptr) {
			Logger::ifout() << "Sub::_initMakeFeas(): pool too small to insert all constraints\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::MakeFeasible);
		}

		newSlots.push(slot);
	}

	activateVars(newSlots);

	return 0;
}

}
