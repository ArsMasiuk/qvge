// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*
  Debug compile symbols for CoinPresolve.

  DEFINE THE SAME SET OF SYMBOLS when building the various CoinPresolve*.cpp
  files in CoinUtils. Without consistent symbol definitions, the results will
  be somewhere between garbage and a core dump.
*/

// #define PRESOLVE_CONSISTENCY	1
// #define PRESOLVE_DEBUG	1
// #define PRESOLVE_SUMMARY 1

#include <stdio.h>

#include <cassert>
#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "CoinFinite.hpp"

#include "CoinPackedMatrix.hpp"
#include "CoinWarmStartBasis.hpp"
#include "OsiSolverInterface.hpp"

#include "OsiPresolve.hpp"
#include "CoinPresolveMatrix.hpp"

#include "CoinPresolveEmpty.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolvePsdebug.hpp"
#include "CoinPresolveSingleton.hpp"
#include "CoinPresolveDoubleton.hpp"
#include "CoinPresolveTripleton.hpp"
#include "CoinPresolveZeros.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinPresolveForcing.hpp"
#include "CoinPresolveDual.hpp"
#include "CoinPresolveTighten.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinPresolveDupcol.hpp"
#include "CoinPresolveImpliedFree.hpp"
#include "CoinPresolveIsolated.hpp"
#include "CoinMessage.hpp"


OsiPresolve::OsiPresolve() :
  originalModel_(NULL),
  presolvedModel_(NULL),
  nonLinearValue_(0.0),
  originalColumn_(NULL),
  originalRow_(NULL),
  paction_(0),
  ncols_(0),
  nrows_(0),
  nelems_(0),
  presolveActions_(0),
  numberPasses_(5)
{
}

OsiPresolve::~OsiPresolve()
{
  gutsOfDestroy();
}
// Gets rid of presolve actions (e.g.when infeasible)
void
OsiPresolve::gutsOfDestroy()
{
 const CoinPresolveAction *paction = paction_;
  while (paction) {
    const CoinPresolveAction *next = paction->next;
    delete paction;
    paction = next;
  }
  delete [] originalColumn_;
  delete [] originalRow_;
  paction_=NULL;
  originalColumn_=NULL;
  originalRow_=NULL;
}

/* This version of presolve returns a pointer to a new presolved
   model.  NULL if infeasible

   doStatus controls activities required to transform an existing
   solution to match the presolved problem. I'd (lh) argue that this should
   default to false, but to maintain previous behaviour it defaults to true.
   Really, this is only useful if you've already optimised before applying
   presolve and also want to work with the solution after presolve.  I think
   that this is the less common case. The more common situation is to apply
   presolve before optimising.
*/
OsiSolverInterface *
OsiPresolve::presolvedModel(OsiSolverInterface & si,
			    double feasibilityTolerance,
			    bool keepIntegers,
			    int numberPasses,
                            const char * prohibited,
			    bool doStatus,
			    const char * rowProhibited)
{
  ncols_ = si.getNumCols();
  nrows_ = si.getNumRows();
  nelems_ = si.getNumElements();
  numberPasses_ = numberPasses;

  double maxmin = si.getObjSense();
  originalModel_ = &si;
  delete [] originalColumn_;
  originalColumn_ = new int[ncols_];
  delete [] originalRow_;
  originalRow_ = new int[nrows_];
  int i;
  for (i=0;i<ncols_;i++)
    originalColumn_[i]=i;
  for (i=0;i<nrows_;i++)
    originalRow_[i]=i;

  // result is 0 - okay, 1 infeasible, -1 go round again
  int result = -1;

  // User may have deleted - its their responsibility
  presolvedModel_=NULL;
  // Messages
  CoinMessages messages = CoinMessage(si.messages().language());
  // Only go round 100 times even if integer preprocessing
  int totalPasses=100;
  while (result==-1) {

    // make new copy
    delete presolvedModel_;
    presolvedModel_ = si.clone();
    totalPasses--;

    // drop integer information if wanted
    if (!keepIntegers) {
      int i;
      for (i=0;i<ncols_;i++)
	presolvedModel_->setContinuous(i);
    }


    CoinPresolveMatrix prob(ncols_,
			    maxmin,
			    presolvedModel_,
			    nrows_, nelems_,doStatus,nonLinearValue_,prohibited,
			    rowProhibited);
    // make sure row solution correct
    if (doStatus) {
      double *colels	= prob.colels_;
      int *hrow		= prob.hrow_;
      CoinBigIndex *mcstrt		= prob.mcstrt_;
      int *hincol		= prob.hincol_;
      int ncols		= prob.ncols_;


      double * csol = prob.sol_;
      double * acts = prob.acts_;
      int nrows = prob.nrows_;

      int colx;

      memset(acts,0,nrows*sizeof(double));

      for (colx = 0; colx < ncols; ++colx) {
	double solutionValue = csol[colx];
	for (int i=mcstrt[colx]; i<mcstrt[colx]+hincol[colx]; ++i) {
	  int row = hrow[i];
	  double coeff = colels[i];
	  acts[row] += solutionValue*coeff;
	}
      }
    }

    // move across feasibility tolerance
    prob.feasibilityTolerance_ = feasibilityTolerance;

/*
  Do presolve. Allow for the possibility that presolve might be ineffective
  (i.e., we're feasible but no postsolve actions are queued.
*/
    paction_ = presolve(&prob) ;
    result = 0 ;
    // Get rid of useful arrays
    prob.deleteStuff();
/*
  This we don't need to do unless presolve actually reduced the system.
*/
    if (prob.status_==0&&paction_) {
      // Looks feasible but double check to see if anything slipped through
      int n		= prob.ncols_;
      double * lo = prob.clo_;
      double * up = prob.cup_;
      int i;

      for (i=0;i<n;i++) {
	if (up[i]<lo[i]) {
	  if (up[i]<lo[i]-1.0e-8) {
	    // infeasible
	    prob.status_=1;
	  } else {
	    up[i]=lo[i];
	  }
	}
      }

      n = prob.nrows_;
      lo = prob.rlo_;
      up = prob.rup_;

      for (i=0;i<n;i++) {
	if (up[i]<lo[i]) {
	  if (up[i]<lo[i]-1.0e-8) {
	    // infeasible
	    prob.status_=1;
	  } else {
	    up[i]=lo[i];
	  }
	}
      }
    }
/*
  If we're feasible, load the presolved system into the solver. Presumably we
  could skip model update and copying of status and solution if presolve took
  no action.
*/
    if (prob.status_ == 0) {

      prob.update_model(presolvedModel_, nrows_, ncols_, nelems_);

# if PRESOLVE_CONSISTENCY
      if (doStatus)
      { int basicCnt = 0 ;
	int basicColumns = 0;
	int i ;
	CoinPresolveMatrix::Status status ;
	for (i = 0 ; i < prob.ncols_ ; i++)
	{ status = prob.getColumnStatus(i);
	  if (status == CoinPrePostsolveMatrix::basic) basicColumns++ ; }
	basicCnt = basicColumns;
	for (i = 0 ; i < prob.nrows_ ; i++)
	{ status = prob.getRowStatus(i);
	  if (status == CoinPrePostsolveMatrix::basic) basicCnt++ ; }

# if PRESOLVE_DEBUG
	presolve_check_nbasic(&prob) ;
# endif
	if (basicCnt>prob.nrows_) {
	  // Take out slacks
	  double * acts = prob.acts_;
	  double * rlo = prob.rlo_;
	  double * rup = prob.rup_;
	  double infinity = si.getInfinity();
	  for (i = 0 ; i < prob.nrows_ ; i++) {
	    status = prob.getRowStatus(i);
	    if (status == CoinPrePostsolveMatrix::basic) {
	      basicCnt-- ;
	      double down = acts[i]-rlo[i];
	      double up = rup[i]-acts[i];
	      if (CoinMin(up,down)<infinity) {
		if (down<=up)
		  prob.setRowStatus(i,CoinPrePostsolveMatrix::atLowerBound);
		else
		  prob.setRowStatus(i,CoinPrePostsolveMatrix::atUpperBound);
	      } else {
		prob.setRowStatus(i,CoinPrePostsolveMatrix::isFree);
	      }
	    }
	    if (basicCnt==prob.nrows_)
	      break;
	  }
	}
      }
#endif

/*
  Install the status and primal solution, if we've been carrying them along.

  The code that copies status is efficient but brittle. The current definitions
  for CoinWarmStartBasis::Status and CoinPrePostsolveMatrix::Status are in
  one-to-one correspondence. This code will fail if that ever changes.
*/
      if (doStatus) {
	presolvedModel_->setColSolution(prob.sol_);
	CoinWarmStartBasis *basis =
	  dynamic_cast<CoinWarmStartBasis *>(presolvedModel_->getEmptyWarmStart());
	basis->resize(prob.nrows_,prob.ncols_);
	int i;
	for (i=0;i<prob.ncols_;i++) {
	  CoinWarmStartBasis::Status status =
	    static_cast<CoinWarmStartBasis::Status> (prob.getColumnStatus(i));
	  basis->setStructStatus(i,status);
	}
	for (i=0;i<prob.nrows_;i++) {
	  CoinWarmStartBasis::Status status =
	    static_cast<CoinWarmStartBasis::Status> (prob.getRowStatus(i));
	  basis->setArtifStatus(i,status);
	}
	presolvedModel_->setWarmStart(basis);
	delete basis ;
	delete [] prob.sol_;
	delete [] prob.acts_;
	delete [] prob.colstat_;
	prob.sol_=NULL;
	prob.acts_=NULL;
	prob.colstat_=NULL;
      }
/*
  Copy original column and row information from the CoinPresolveMatrix object
  so it'll be available for postsolve.
*/
      int ncolsNow = presolvedModel_->getNumCols();
      memcpy(originalColumn_,prob.originalColumn_,ncolsNow*sizeof(int));
      delete [] prob.originalColumn_;
      prob.originalColumn_=NULL;
      int nrowsNow = presolvedModel_->getNumRows();
      memcpy(originalRow_,prob.originalRow_,nrowsNow*sizeof(int));
      delete [] prob.originalRow_;
      prob.originalRow_=NULL;

      // now clean up integer variables.  This can modify original
      {
	int numberChanges=0;
	const double * lower0 = originalModel_->getColLower();
	const double * upper0 = originalModel_->getColUpper();
	const double * lower = presolvedModel_->getColLower();
	const double * upper = presolvedModel_->getColUpper();
	for (i=0;i<ncolsNow;i++) {
	  if (!presolvedModel_->isInteger(i))
	    continue;
	  int iOriginal = originalColumn_[i];
	  double lowerValue0 = lower0[iOriginal];
	  double upperValue0 = upper0[iOriginal];
	  double lowerValue = ceil(lower[i]-1.0e-5);
	  double upperValue = floor(upper[i]+1.0e-5);
	  presolvedModel_->setColBounds(i,lowerValue,upperValue);
	  if (lowerValue>upperValue) {
	    numberChanges++;
	    presolvedModel_->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
						       messages)
							 <<iOriginal
							 <<lowerValue
							 <<upperValue
							 <<CoinMessageEol;
	    result=1;
	  } else {
	    if (lowerValue>lowerValue0+1.0e-8) {
	      originalModel_->setColLower(iOriginal,lowerValue);
	      numberChanges++;
	    }
	    if (upperValue<upperValue0-1.0e-8) {
	      originalModel_->setColUpper(iOriginal,upperValue);
	      numberChanges++;
	    }
	  }
	}
	if (numberChanges) {
	  presolvedModel_->messageHandler()->message(COIN_PRESOLVE_INTEGERMODS,
						     messages)
						       <<numberChanges
						       <<CoinMessageEol;
	  if (!result&&totalPasses>0&&
	      // we can't go round again in integer if dupcols
	      (prob.presolveOptions_ & 0x80000000) == 0) {
	    result = -1; // round again
	    const CoinPresolveAction *paction = paction_;
	    while (paction) {
	      const CoinPresolveAction *next = paction->next;
	      delete paction;
	      paction = next;
	    }
	    paction_=NULL;
	  }
	}
      }
    } else if (prob.status_ != 0) {
      // infeasible or unbounded
      result = 1 ;
    }
  }
  if (!result) {
    int nrowsAfter = presolvedModel_->getNumRows();
    int ncolsAfter = presolvedModel_->getNumCols();
    CoinBigIndex nelsAfter = presolvedModel_->getNumElements();
    presolvedModel_->messageHandler()->message(COIN_PRESOLVE_STATS, messages)
			   <<nrowsAfter<< -(nrows_ - nrowsAfter)
			   << ncolsAfter<< -(ncols_ - ncolsAfter)
			   <<nelsAfter<< -(nelems_ - nelsAfter)
			   <<CoinMessageEol;
  } else {
    gutsOfDestroy();
    delete presolvedModel_;
    presolvedModel_=NULL;
  }
  return presolvedModel_;
}

// Return pointer to presolved model
OsiSolverInterface *
OsiPresolve::model() const
{
  return presolvedModel_;
}
// Return pointer to original model
OsiSolverInterface *
OsiPresolve::originalModel() const
{
  return originalModel_;
}
void
OsiPresolve::postsolve(bool updateStatus)
{
  // Messages
  CoinMessages messages = CoinMessage(presolvedModel_->messages().language());
  if (!presolvedModel_->isProvenOptimal()) {
    presolvedModel_->messageHandler()->message(COIN_PRESOLVE_NONOPTIMAL,
					     messages)
					       <<CoinMessageEol;
  }

  // this is the size of the original problem
  const int ncols0  = ncols_;
  const int nrows0  = nrows_;
  const CoinBigIndex nelems0 = nelems_;

  // reality check
  assert(ncols0==originalModel_->getNumCols());
  assert(nrows0==originalModel_->getNumRows());

  // this is the reduced problem
  int ncols = presolvedModel_->getNumCols();
  int nrows = presolvedModel_->getNumRows();

  double *acts = new double [nrows0];
  double *sol = new double [ncols0];
  CoinZeroN(acts,nrows0);
  CoinZeroN(sol,ncols0);

  unsigned char * rowstat=NULL;
  unsigned char * colstat = NULL;
  CoinWarmStartBasis * presolvedBasis  =
    dynamic_cast<CoinWarmStartBasis*>(presolvedModel_->getWarmStart());
  if (!presolvedBasis)
    updateStatus=false;
  if (updateStatus) {
    colstat = new unsigned char[ncols0+nrows0];
#   ifdef ZEROFAULT
    memset(colstat,0,((ncols0+nrows0)*sizeof(char))) ;
#   endif
    rowstat = colstat + ncols0;
    int i;
    for (i=0;i<ncols;i++) {
      colstat[i] = presolvedBasis->getStructStatus(i);
    }
    for (i=0;i<nrows;i++) {
      rowstat[i] = presolvedBasis->getArtifStatus(i);
    }
  }
  delete presolvedBasis;

# if PRESOLVE_CONSISTENCY > 0
  if (updateStatus)
  { int basicCnt = 0 ;
    int i ;
    for (i = 0 ; i < ncols ; i++)
    { if (colstat[i] == CoinWarmStartBasis::basic) basicCnt++ ; }
    for (i = 0 ; i < nrows ; i++)
    { if (rowstat[i] == CoinWarmStartBasis::basic) basicCnt++ ; }

    assert (basicCnt == nrows) ;
  }
# endif

/*
  Postsolve back to the original problem.  The CoinPostsolveMatrix object
  assumes ownership of sol, acts, colstat, and rowstat.
*/
  CoinPostsolveMatrix prob(presolvedModel_, ncols0, nrows0, nelems0,
			   presolvedModel_->getObjSense(),
			   sol, acts, colstat, rowstat);

  postsolve(prob);


# if PRESOLVE_CONSISTENCY > 0
  if (updateStatus)
  { int basicCnt = 0 ;
    int i ;
    for (i = 0 ; i < ncols0 ; i++)
    { if (prob.getColumnStatus(i) == CoinWarmStartBasis::basic) basicCnt++ ; }
    for (i = 0 ; i < nrows0 ; i++)
    { if (prob.getRowStatus(i) == CoinWarmStartBasis::basic) basicCnt++ ; }

    assert (basicCnt == nrows0) ;
  }
# endif

  originalModel_->setColSolution(sol);
  if (updateStatus) {
    CoinWarmStartBasis *basis =
      dynamic_cast<CoinWarmStartBasis *>(presolvedModel_->getEmptyWarmStart());
    basis->setSize(ncols0,nrows0);
    int i;
    for (i=0;i<ncols0;i++) {
      CoinWarmStartBasis::Status status = static_cast<CoinWarmStartBasis::Status> (prob.getColumnStatus(i));
      /* FIXME: these asserts seem correct, but seem to reveal some bugs in CoinPresolve */
      // assert(status != CoinWarmStartBasis::atLowerBound || originalModel_->getColLower()[i] > -originalModel_->getInfinity());
      // assert(status != CoinWarmStartBasis::atUpperBound || originalModel_->getColUpper()[i] <  originalModel_->getInfinity());
      basis->setStructStatus(i,status);
    }
    for (i=0;i<nrows0;i++) {
      CoinWarmStartBasis::Status status = static_cast<CoinWarmStartBasis::Status> (prob.getRowStatus(i));
      /* FIXME: these asserts seem correct, but seem to reveal some bugs in CoinPresolve */
      // assert(status != CoinWarmStartBasis::atUpperBound || originalModel_->getRowLower()[i] > -originalModel_->getInfinity());
      // assert(status != CoinWarmStartBasis::atLowerBound || originalModel_->getRowUpper()[i] <  originalModel_->getInfinity());
      basis->setArtifStatus(i,status);
    }
    originalModel_->setWarmStart(basis);
    delete basis ;
  }

}

// return pointer to original columns
const int *
OsiPresolve::originalColumns() const
{
  return originalColumn_;
}
// return pointer to original rows
const int *
OsiPresolve::originalRows() const
{
  return originalRow_;
}
// Set pointer to original model
void
OsiPresolve::setOriginalModel(OsiSolverInterface * model)
{
  originalModel_=model;
}

#if 0
// A lazy way to restrict which transformations are applied
// during debugging.
static int ATOI(const char *name)
{
 return true;
#if	PRESOLVE_DEBUG || PRESOLVE_SUMMARY
  if (getenv(name)) {
    int val = atoi(getenv(name));
    printf("%s = %d\n", name, val);
    return (val);
  } else {
    if (strcmp(name,"off"))
      return (true);
    else
      return (false);
  }
#else
  return (true);
#endif
}
#endif

#if PRESOLVE_DEBUG
// Anonymous namespace for debug routines
namespace {

/*
  A control routine for debug checks --- keeps down the clutter in doPresolve.
  Each time it's called, it prints a list of transforms applied since the last
  call, then does checks.
*/

void check_and_tell (const CoinPresolveMatrix *const prob,
		     const CoinPresolveAction *first,
		     const CoinPresolveAction *&mark)

{ const CoinPresolveAction *current ;

  if (first != mark)
  { printf("PRESOLVE: applied") ;
    for (current = first ;
	 current != mark && current != 0 ;
	 current = current->next)
    { printf(" %s",current->name()) ; }
    printf("\n") ;

    presolve_check_sol(prob) ;
    presolve_check_nbasic(prob) ;
    mark = first ; }

  return ; }

} // end anonymous namespace for debug routines
#endif
//#define COIN_PRESOLVE_BUG
#ifdef COIN_PRESOLVE_BUG
double * debugSolution = NULL;
int debugNumberColumns = -1;
static int counter=1000000;
static bool break2(CoinPresolveMatrix *prob)
{
  if (counter>0)
    printf("counter %d\n",counter);
  counter--;
  if (debugSolution&&prob->ncols_==debugNumberColumns) {
    for (int i=0;i<prob->ncols_;i++) {
      double value = debugSolution[i];
      if (value<prob->clo_[i]) {
	printf("%d inf %g %g %g\n",i,prob->clo_[i],value,prob->cup_[i]);
      } else if (value>prob->cup_[i]) {
	printf("%d inf %g %g %g\n",i,prob->clo_[i],value,prob->cup_[i]);
      }
    }
  }
  if (!counter) {
    printf("skipping next and all\n");
  }
  return (counter<=0);
}
#define possibleBreak if (break2(prob)) break
#define possibleSkip  if (!break2(prob))
#else
#define possibleBreak
#define possibleSkip
#endif
// This is the presolve loop.
// It is a separate virtual function so that it can be easily
// customized by subclassing CoinPresolve.

const CoinPresolveAction *OsiPresolve::presolve(CoinPresolveMatrix *prob)
{
  paction_ = 0;

  prob->status_=0; // say feasible

# if PRESOLVE_DEBUG
  const CoinPresolveAction *pactiond = 0 ;
  presolve_check_sol(prob) ;
# endif
  if ((presolveActions_&4)!=0)
    transferCosts(prob);

/*
  Fix variables before we get into the main transform loop.
*/
  paction_ = make_fixed(prob, paction_);

# if PRESOLVE_DEBUG
  check_and_tell(prob,paction_,pactiond) ;
# endif

  // if integers then switch off dual stuff
  // later just do individually
  bool doDualStuff = true;
  if ((presolveActions_&1)==0) {
    int i;
    int ncol = presolvedModel_->getNumCols();
    for (i=0;i<ncol;i++)
      if (presolvedModel_->isInteger(i))
	doDualStuff=false;
  }


# if CHECK_CONSISTENCY
  presolve_links_ok(prob->rlink_, prob->mrstrt_, prob->hinrow_, prob->nrows_);
# endif

/*
  If we're feasible, set up for the main presolve transform loop.
*/
  if (!prob->status_) {
# if 0
/*
  This block is used during debugging. See ATOI to see how it works. Some
  editing will be required to turn it all on.
*/
    bool slackd = ATOI("SLACKD")!=0;
    //bool forcing = ATOI("FORCING")!=0;
    bool doubleton = ATOI("DOUBLETON")!=0;
    bool forcing = ATOI("off")!=0;
    bool ifree = ATOI("off")!=0;
    bool zerocost = ATOI("off")!=0;
    bool dupcol = ATOI("off")!=0;
    bool duprow = ATOI("off")!=0;
    bool dual = ATOI("off")!=0;
# else
# if 1
    // normal operation --- all transforms enabled
    bool slackSingleton = true;
    bool slackd = true;
    bool doubleton = true;
    bool tripleton = true;
    //#define NO_FORCING
#ifndef NO_FORCING
    bool forcing = true;
#endif
    bool ifree = true;
    bool zerocost = true;
    bool dupcol = true;
    bool duprow = true;
    bool dual = doDualStuff;
# else
    // compile time selection of transforms.
    bool slackd = false;
    bool doubleton = true;
    bool tripleton = true;
    bool forcing = true;
    bool ifree = false;
    bool zerocost = false;
    bool dupcol = false;
    bool duprow = false;
    bool dual = false;
# endif
# endif
    // Switch off some stuff if would annoy set partitioning etc
    if ((presolveActions_&2)!=0) {
      doubleton = false;
      tripleton = false;
      ifree = false;
    }
    // stop x+y+z==1
    if ((presolveActions_&8)!=0)
      prob->setPresolveOptions(prob->presolveOptions()|4);
    // switch on stuff which can't be unrolled easily
    if ((presolveActions_&16)!=0)
      prob->setPresolveOptions(prob->presolveOptions()|16);
    // switch on gub stuff
    if ((presolveActions_&32)!=0)
      prob->setPresolveOptions(prob->presolveOptions()|32);

    /*
      The main loop (just below) starts with a minor loop that does
      inexpensive presolve transforms until convergence. At each iteration
      of this loop, next[Rows,Cols]ToDo is copied over to [rows,cols]ToDo.

      Then there's a block like the one here, which sets [rows,cols]ToDo for
      all rows & cols, followed by executions of a set of expensive
      transforms. Then we come back around for another iteration of the main
      loop. [rows,cols]ToDo is not reset as we come back around, so we dive
      into the inexpensive loop set up to process all.
    */

    int i;
    // say look at all
    if (!prob->anyProhibited()) {
      for (i=0;i<nrows_;i++)
	prob->rowsToDo_[i]=i;
      prob->numberRowsToDo_=nrows_;
      for (i=0;i<ncols_;i++)
	prob->colsToDo_[i]=i;
      prob->numberColsToDo_=ncols_;
    } else {
      // some stuff must be left alone
      prob->numberRowsToDo_=0;
      for (i=0;i<nrows_;i++)
	if (!prob->rowProhibited(i))
	    prob->rowsToDo_[prob->numberRowsToDo_++]=i;
      prob->numberColsToDo_=0;
      for (i=0;i<ncols_;i++)
	if (!prob->colProhibited(i))
	    prob->colsToDo_[prob->numberColsToDo_++]=i;
    }


    int iLoop;
    if (dupcol) {
      // maybe allow integer columns to be checked
      if ((presolveActions_&1)!=0)
	prob->setPresolveOptions(prob->presolveOptions()|1);
      possibleSkip;
      paction_ = dupcol_action::presolve(prob, paction_);
    }
    if (duprow) {
      possibleSkip;
      paction_ = duprow_action::presolve(prob, paction_);
    }
    // Check number rows dropped
    int lastDropped=0;
    /*
      Note that pass_ is incremented in testRedundant, evoked from
      implied_free_action. The bulk of testRedundant is executed every other
      pass.
    */
    prob->pass_=0;
    for (iLoop=0;iLoop<numberPasses_;iLoop++) {

#     ifdef PRESOLVE_SUMMARY
      printf("Starting major pass %d\n",iLoop+1);
#     endif

      const CoinPresolveAction * const paction0 = paction_;
      // look for substitutions with no fill
      //#define IMPLIED 3
#ifdef IMPLIED
      int fill_level=3;
#define IMPLIED2 1
#if IMPLIED!=3
#if IMPLIED>0&&IMPLIED<11
      fill_level=IMPLIED;
      printf("** fill_level == %d !\n",fill_level);
#endif
#if IMPLIED>11&&IMPLIED<21
      fill_level=-(IMPLIED-10);
      printf("** fill_level == %d !\n",fill_level);
#endif
#endif
#else
      int fill_level=2;
#endif
      int whichPass=0;
/*
  Apply inexpensive transforms until convergence.
*/
      while (1) {
	whichPass++;
	prob->pass_++;
	const CoinPresolveAction * const paction1 = paction_;

	if (slackd) {
	  bool notFinished = true;
	  while (notFinished) {
	    possibleBreak;
	    paction_ = slack_doubleton_action::presolve(prob, paction_,
							notFinished);
	  }
	  if (prob->status_)
	    break;
#	  if PRESOLVE_DEBUG
	  check_and_tell(prob,paction_,pactiond) ;
#	  endif
	}

	if (dual&&whichPass==1) {
	  possibleBreak;
	  // this can also make E rows so do one bit here
	  paction_ = remove_dual_action::presolve(prob, paction_);
	  if (prob->status_)
	    break;
	}

	if (doubleton) {
	  possibleBreak;
	  paction_ = doubleton_action::presolve(prob, paction_);
	  if (prob->status_)
	    break;
#	  if PRESOLVE_DEBUG
	  check_and_tell(prob,paction_,pactiond) ;
#	  endif
	}

	if (tripleton) {
	  possibleBreak;
	  paction_ = tripleton_action::presolve(prob, paction_);
	  if (prob->status_)
	    break;
#	  if PRESOLVE_DEBUG
	  check_and_tell(prob,paction_,pactiond) ;
#	  endif
	}

	if (zerocost) {
	  possibleBreak;
	  paction_ = do_tighten_action::presolve(prob, paction_);
	  if (prob->status_)
	    break;
#	  if PRESOLVE_DEBUG
	  check_and_tell(prob,paction_,pactiond) ;
#	  endif
	}

#ifndef NO_FORCING
	if (forcing) {
	  possibleBreak;
	  paction_ = forcing_constraint_action::presolve(prob, paction_);
	  if (prob->status_)
	    break;
#	  if PRESOLVE_DEBUG
	  check_and_tell(prob,paction_,pactiond) ;
#	  endif
	}
#endif

	if (ifree&&(whichPass%5)==1) {
	  possibleBreak;
	  paction_ = implied_free_action::presolve(prob, paction_,fill_level);
	  if (prob->status_)
	    break;
#	  if PRESOLVE_DEBUG
	  check_and_tell(prob,paction_,pactiond) ;
#	  endif
	}

#	if CHECK_CONSISTENCY
	presolve_links_ok(prob->rlink_,prob->mrstrt_,
			  prob->hinrow_,prob->nrows_) ;
#	endif

#	if 0 && PRESOLVE_DEBUG

    /*
      For reasons that escape me just now, the linker is unable to find
      this function. Copying the code from CoinPresolvePsdebug to the head
      of this routine works just fine. Library loading order looks ok. Other
      routines from CoinPresolvePsdebug are found. I'm stumped. -- lh --
    */

	presolve_no_zeros(prob->mcstrt_, prob->colels_, prob->hincol_,
			  prob->ncols_);
#	endif
#	if CHECK_CONSISTENCY
	prob->consistent();
#	endif


	// set up for next pass
	// later do faster if many changes i.e. memset and memcpy
	prob->numberRowsToDo_ = prob->numberNextRowsToDo_;
	int kcheck;	// debug?
	bool found=false;
	kcheck=-1;
	for (i=0;i<prob->numberNextRowsToDo_;i++) {
	  int index = prob->nextRowsToDo_[i];
	  prob->unsetRowChanged(index);
	  prob->rowsToDo_[i] = index;
	  if (index==kcheck) {
	    printf("row %d on list after pass %d\n",kcheck,
		   whichPass);
	    found=true;
	  }
	}
	if (!found&&kcheck>=0)
	  prob->rowsToDo_[prob->numberRowsToDo_++]=kcheck;
	prob->numberNextRowsToDo_=0;
	prob->numberColsToDo_ = prob->numberNextColsToDo_;
	kcheck=-1;
	found=false;
	for (i=0;i<prob->numberNextColsToDo_;i++) {
	  int index = prob->nextColsToDo_[i];
	  prob->unsetColChanged(index);
	  prob->colsToDo_[i] = index;
	  if (index==kcheck) {
	    printf("col %d on list after pass %d\n",kcheck,
		   whichPass);
	    found=true;
	  }
	}
	if (!found&&kcheck>=0)
	  prob->colsToDo_[prob->numberColsToDo_++]=kcheck;
	prob->numberNextColsToDo_=0;
	if (paction_ == paction1&&fill_level>0)
	  break;
      } // End of inexpensive transform loop

      // say look at all
      int i;
      if (!prob->anyProhibited()) {
	for (i=0;i<nrows_;i++)
	  prob->rowsToDo_[i]=i;
	prob->numberRowsToDo_=nrows_;
	for (i=0;i<ncols_;i++)
	  prob->colsToDo_[i]=i;
	prob->numberColsToDo_=ncols_;
      } else {
	// some stuff must be left alone
	prob->numberRowsToDo_=0;
	for (i=0;i<nrows_;i++)
	  if (!prob->rowProhibited(i))
	    prob->rowsToDo_[prob->numberRowsToDo_++]=i;
	prob->numberColsToDo_=0;
	for (i=0;i<ncols_;i++)
	  if (!prob->colProhibited(i))
	    prob->colsToDo_[prob->numberColsToDo_++]=i;
      }
      // now expensive things
      // this caused world.mps to run into numerical difficulties

#     ifdef PRESOLVE_SUMMARY
      printf("Starting expensive\n");
#     endif

      if (dual) {
	int itry;
	for (itry=0;itry<5;itry++) {
	  const CoinPresolveAction * const paction2 = paction_;
	  possibleBreak;
	  paction_ = remove_dual_action::presolve(prob, paction_);
#	  if PRESOLVE_DEBUG
	  check_and_tell(prob,paction_,pactiond) ;
#	  endif
	  if (prob->status_)
	    break;
	  if (ifree) {
#ifdef IMPLIED
#if IMPLIED2 ==0
	    int fill_level=0; // switches off substitution
#elif IMPLIED2!=99
	    int fill_level=IMPLIED2;
#endif
#endif
	    if ((itry&1)==0) {
	      possibleBreak;
	      paction_ = implied_free_action::presolve(prob, paction_,fill_level);
	    }
#	    if PRESOLVE_DEBUG
	    check_and_tell(prob,paction_,pactiond) ;
#	    endif
	    if (prob->status_)
	      break;
	  }
	  if (paction_ == paction2)
	    break;
	}
      } else if (ifree) {
	// just free
#ifdef IMPLIED
#if IMPLIED2 ==0
	int fill_level=0; // switches off substitution
#elif IMPLIED2!=99
	int fill_level=IMPLIED2;
#endif
#endif
	possibleBreak;
	paction_ = implied_free_action::presolve(prob, paction_,fill_level);
	if (prob->status_)
	  break;
      }

      if (dupcol) {
        // maybe allow integer columns to be checked
        if ((presolveActions_&1)!=0)
          prob->setPresolveOptions(prob->presolveOptions()|1);
	possibleBreak;
	paction_ = dupcol_action::presolve(prob, paction_);
#	if PRESOLVE_DEBUG
	check_and_tell(prob,paction_,pactiond) ;
#	endif
	if (prob->status_)
	  break;
      }

      if (duprow) {
	possibleBreak;
	paction_ = duprow_action::presolve(prob, paction_);
#	if PRESOLVE_DEBUG
	check_and_tell(prob,paction_,pactiond) ;
#	endif
	if (prob->status_)
	  break;
      }
      if ((presolveActions_&32)!=0) {
	possibleBreak;
	paction_ = gubrow_action::presolve(prob, paction_);
      }

      bool stopLoop=false;
      {
	int * hinrow = prob->hinrow_;
	int numberDropped=0;
	for (i=0;i<nrows_;i++)
	  if (!hinrow[i])
	    numberDropped++;
	//printf("%d rows dropped after pass %d\n",numberDropped,
	//     iLoop+1);
	if (numberDropped==lastDropped)
	  stopLoop=true;
	else
	  lastDropped = numberDropped;
      }
      // Do this here as not very loopy
      if (slackSingleton) {
        // On most passes do not touch costed slacks
        if (paction_ != paction0&&!stopLoop) {
	  possibleBreak;
          paction_ = slack_singleton_action::presolve(prob, paction_,NULL);
        } else {
          // do costed if Clp (at end as ruins rest of presolve)
	  possibleBreak;
          paction_ = slack_singleton_action::presolve(prob, paction_,NULL);
          stopLoop=true;
        }
      }
#if	PRESOLVE_DEBUG
      presolve_check_sol(prob,1);
#endif
      if (paction_ == paction0||stopLoop)
	break;

    } // End of major pass loop
  }
/*
  Final cleanup: drop zero coefficients from the matrix, then drop empty rows
  and columns.
*/
  if (!prob->status_) {
    paction_ = drop_zero_coefficients(prob, paction_);
#   if PRESOLVE_DEBUG
    check_and_tell(prob,paction_,pactiond) ;
#   endif

    paction_ = drop_empty_cols_action::presolve(prob, paction_);
#   if PRESOLVE_DEBUG
    check_and_tell(prob,paction_,pactiond) ;
#   endif

    paction_ = drop_empty_rows_action::presolve(prob, paction_);
#   if PRESOLVE_DEBUG
    check_and_tell(prob,paction_,pactiond) ;
#   endif
  }
  // Messages
  CoinMessages messages = CoinMessage(prob->messages().language());
  if (prob->status_) {
    if (prob->status_==1)
	  prob->messageHandler()->message(COIN_PRESOLVE_INFEAS,
					     messages)
					       <<prob->feasibilityTolerance_
					       <<CoinMessageEol;
    else if (prob->status_==2)
	  prob->messageHandler()->message(COIN_PRESOLVE_UNBOUND,
					     messages)
					       <<CoinMessageEol;
    else
	  prob->messageHandler()->message(COIN_PRESOLVE_INFEASUNBOUND,
					     messages)
					       <<CoinMessageEol;
    // get rid of data
    gutsOfDestroy();
  }
  return (paction_);
}


// We could have implemented this by having each postsolve routine
// directly call the next one, but this may make it easier to add debugging checks.
void OsiPresolve::postsolve(CoinPostsolveMatrix &prob)
{
  const CoinPresolveAction *paction = paction_;

#if	PRESOLVE_DEBUG
  printf("Begin POSTSOLVING\n") ;
  if (prob.colstat_)
  { presolve_check_nbasic(&prob);
    presolve_check_sol(&prob); }
  presolve_check_duals(&prob);
#endif


  while (paction) {
#   if PRESOLVE_DEBUG
    printf("POSTSOLVING %s\n", paction->name());
#   endif

    paction->postsolve(&prob);

#   if PRESOLVE_DEBUG
    if (prob.colstat_)
    { presolve_check_nbasic(&prob);
      presolve_check_sol(&prob); }
#   endif
    paction = paction->next;
#   if PRESOLVE_DEBUG
    presolve_check_duals(&prob);
#   endif
  }
# if PRESOLVE_DEBUG
    printf("End POSTSOLVING\n") ;
# endif

#if	0 && PRESOLVE_DEBUG

  << This block of checks will require some work to get it to compile. >>

  for (i=0; i<ncols0; i++) {
    if (!cdone_[i]) {
      printf("!cdone_[%d]\n", i);
      abort();
    }
  }

  for (int i=0; i<nrows0; i++) {
    if (!rdone[i]) {
      printf("!rdone[%d]\n", i);
      abort();
    }
  }


  for (i=0; i<ncols0; i++) {
    if (sol[i] < -1e10 || sol[i] > 1e10)
      printf("!!!%d %g\n", i, sol[i]);

  }


#endif

#if	0 && PRESOLVE_DEBUG

  << This block of checks will require some work to get it to compile. >>

  // debug check:  make sure we ended up with same original matrix
  {
    int identical = 1;

    for (int i=0; i<ncols0; i++) {
      PRESOLVEASSERT(hincol[i] == &prob->mcstrt0[i+1] - &prob->mcstrt0[i]);
      CoinBigIndex kcs0 = &prob->mcstrt0[i];
      CoinBigIndex kcs = mcstrt[i];
      int n = hincol[i];
      for (int k=0; k<n; k++) {
	CoinBigIndex k1 = presolve_find_row1(&prob->hrow0[kcs0+k], kcs, kcs+n, hrow);

	if (k1 == kcs+n) {
	  printf("ROW %d NOT IN COL %d\n", &prob->hrow0[kcs0+k], i);
	  abort();
	}

	if (colels[k1] != &prob->dels0[kcs0+k])
	  printf("BAD COLEL[%d %d %d]:  %g\n",
		 k1, i, &prob->hrow0[kcs0+k], colels[k1] - &prob->dels0[kcs0+k]);

	if (kcs0+k != k1)
	  identical=0;
      }
    }
    printf("identical? %d\n", identical);
  }
#endif
  // put back duals
  double maxmin = originalModel_->getObjSense();
  if (maxmin<0.0) {
    // swap signs
    int i;
    double * pi = prob.rowduals_;
    for (i=0;i<nrows_;i++)
      pi[i] = -pi[i];
  }
  originalModel_->setRowPrice(prob.rowduals_);
  // Now check solution
  // **** code later - has to be by hand
#if 0
  memcpy(originalModel_->dualColumnSolution(),
	 originalModel_->objective(),ncols_*sizeof(double));
  originalModel_->transposeTimes(-1.0,
				 originalModel_->dualRowSolution(),
				 originalModel_->dualColumnSolution());
  memset(originalModel_->primalRowSolution(),0,nrows_*sizeof(double));
  originalModel_->times(1.0,originalModel_->primalColumnSolution(),
			originalModel_->primalRowSolution());
  originalModel_->checkSolution();
  // Messages
  CoinMessages messages = CoinMessage(presolvedModel_->messages().language());
  presolvedModel_->messageHandler()->message(COIN_PRESOLVE_POSTSOLVE,
					    messages)
					      <<originalModel_->objectiveValue()
					      <<originalModel_->sumDualInfeasibilities()
					      <<originalModel_->numberDualInfeasibilities()
					      <<originalModel_->sumPrimalInfeasibilities()
					      <<originalModel_->numberPrimalInfeasibilities()
					       <<CoinMessageEol;
  //originalModel_->objectiveValue_=objectiveValue_;
  originalModel_->setNumberIterations(presolvedModel_->numberIterations());
  if (!presolvedModel_->status()) {
    if (!originalModel_->numberDualInfeasibilities()&&
	!originalModel_->numberPrimalInfeasibilities()) {
      originalModel_->setProblemStatus( 0);
    } else {
      originalModel_->setProblemStatus( -1);
      presolvedModel_->messageHandler()->message(COIN_PRESOLVE_NEEDS_CLEANING,
					    messages)
					      <<CoinMessageEol;
    }
  } else {
    originalModel_->setProblemStatus( presolvedModel_->status());
  }
#endif
}


static inline double getTolerance(const OsiSolverInterface  *si, OsiDblParam key)
{
  double tol;
  if (! si->getDblParam(key, tol)) {
    CoinPresolveAction::throwCoinError("getDblParam failed",
				      "CoinPrePostsolveMatrix::CoinPrePostsolveMatrix");
  }
  return (tol);
}


// Assumptions:
// 1. nrows>=m.getNumRows()
// 2. ncols>=m.getNumCols()
//
// In presolve, these values are equal.
// In postsolve, they may be inequal, since the reduced problem
// may be smaller, but we need room for the large problem.
// ncols may be larger than si.getNumCols() in postsolve,
// this at that point si will be the reduced problem,
// but we need to reserve enough space for the original problem.

CoinPrePostsolveMatrix::CoinPrePostsolveMatrix(const OsiSolverInterface * si,
					     int ncols_in,
					     int nrows_in,
					     CoinBigIndex nelems_in) :
  ncols_(si->getNumCols()),
  nelems_(si->getNumElements()),
  ncols0_(ncols_in),
  nrows0_(nrows_in),
  bulkRatio_(2.0),

  mcstrt_(new CoinBigIndex[ncols_in+1]),
  hincol_(new int[ncols_in+1]),

  cost_(new double[ncols_in]),
  clo_(new double[ncols_in]),
  cup_(new double[ncols_in]),
  rlo_(new double[nrows_in]),
  rup_(new double[nrows_in]),
  originalColumn_(new int[ncols_in]),
  originalRow_(new int[nrows_in]),

  ztolzb_(getTolerance(si, OsiPrimalTolerance)),
  ztoldj_(getTolerance(si, OsiDualTolerance)),

  maxmin_(si->getObjSense()),

  handler_(0),
  defaultHandler_(false),
  messages_()

{
  bulk0_ = static_cast<CoinBigIndex>(bulkRatio_*nelems_in) ;
  hrow_ = new int [bulk0_] ;
  colels_ = new double[bulk0_] ;

  si->getDblParam(OsiObjOffset,originalOffset_);
  int ncols = si->getNumCols();
  int nrows = si->getNumRows();

  setMessageHandler(si->messageHandler()) ;

  CoinDisjointCopyN(si->getColLower(), ncols, clo_);
  CoinDisjointCopyN(si->getColUpper(), ncols, cup_);
  CoinDisjointCopyN(si->getObjCoefficients(), ncols, cost_);
  CoinDisjointCopyN(si->getRowLower(), nrows,  rlo_);
  CoinDisjointCopyN(si->getRowUpper(), nrows,  rup_);
  int i;
  // initialize and clean up bounds
  double infinity = si->getInfinity();
  if (infinity!=COIN_DBL_MAX) {
    for (i=0;i<ncols;i++) {
      if (clo_[i]==-infinity)
	clo_[i]=-COIN_DBL_MAX;
      if (cup_[i]==infinity)
	cup_[i]=COIN_DBL_MAX;
    }
    for (i=0;i<nrows;i++) {
      if (rlo_[i]==-infinity)
	rlo_[i]=-COIN_DBL_MAX;
      if (rup_[i]==infinity)
	rup_[i]=COIN_DBL_MAX;
    }
  }
  for (i=0;i<ncols_in;i++)
    originalColumn_[i]=i;
  for (i=0;i<nrows_in;i++)
    originalRow_[i]=i;
  sol_=NULL;
  rowduals_=NULL;
  acts_=NULL;

  rcosts_=NULL;
  colstat_=NULL;
  rowstat_=NULL;
}

// I am not familiar enough with CoinPackedMatrix to be confident
// that I will implement a row-ordered version of toColumnOrderedGapFree
// properly.
static bool isGapFree(const CoinPackedMatrix& matrix)
{
  const CoinBigIndex * start = matrix.getVectorStarts();
  const int * length = matrix.getVectorLengths();
  int i;
  for (i = matrix.getSizeVectorLengths() - 1; i >= 0; --i) {
    if (start[i+1] - start[i] != length[i])
      break;
  }
  return (! (i >= 0));
}
CoinPresolveMatrix::CoinPresolveMatrix(int ncols0_in,
				       double /*maxmin_*/,
				       // end prepost members
				       OsiSolverInterface * si,
				       // rowrep
				       int nrows_in,
				       CoinBigIndex nelems_in,
				       bool doStatus,
				       double nonLinearValue,
                                       const char * prohibited,
				       const char * rowProhibited) :

  CoinPrePostsolveMatrix(si,
			ncols0_in, nrows_in, nelems_in),
  clink_(new presolvehlink[ncols0_in+1]),
  rlink_(new presolvehlink[nrows_in+1]),

  dobias_(0.0),

  // temporary init
  mrstrt_(new CoinBigIndex[nrows_in+1]),
  hinrow_(new int[nrows_in+1]),
  integerType_(new unsigned char[ncols0_in]),
  tuning_(false),
  startTime_(0.0),
  feasibilityTolerance_(0.0),
  status_(-1),
  maxSubstLevel_(3),
  colsToDo_(new int [ncols0_in]),
  numberColsToDo_(0),
  nextColsToDo_(new int[ncols0_in]),
  numberNextColsToDo_(0),
  rowsToDo_(new int [nrows_in]),
  numberRowsToDo_(0),
  nextRowsToDo_(new int[nrows_in]),
  numberNextRowsToDo_(0),
  presolveOptions_(0)
{

  rowels_ = new double [bulk0_] ;
  hcol_ = new int [bulk0_] ;

  nrows_ = si->getNumRows() ;
  const CoinBigIndex bufsize = static_cast<CoinBigIndex>(bulkRatio_*nelems_in) ;

  // Set up change bits
  rowChanged_ = new unsigned char[nrows_];
  memset(rowChanged_,0,nrows_);
  colChanged_ = new unsigned char[ncols_];
  memset(colChanged_,0,ncols_);
  const CoinPackedMatrix * m1 = si->getMatrixByCol();

  // The coefficient matrix is a big hunk of stuff.
  // Do the copy here to try to avoid running out of memory.

  const CoinBigIndex * start = m1->getVectorStarts();
  const int * length = m1->getVectorLengths();
  const int * row = m1->getIndices();
  const double * element = m1->getElements();
  int icol,nel=0;
  mcstrt_[0]=0;
  for (icol=0;icol<ncols_;icol++) {
    int j;
    for (j=start[icol];j<start[icol]+length[icol];j++) {
      if (fabs(element[j])>ZTOLDP) {
        hrow_[nel]=row[j];
	colels_[nel++]=element[j];
      }
    }
    hincol_[icol]=nel-mcstrt_[icol];
    mcstrt_[icol+1]=nel;
  }

  // same thing for row rep
  CoinPackedMatrix * m = new CoinPackedMatrix();
  m->reverseOrderedCopyOf(*si->getMatrixByCol());
  // do by hand because of zeros m->removeGaps();
  CoinDisjointCopyN(m->getVectorStarts(),  nrows_,  mrstrt_);
  mrstrt_[nrows_] = nelems_;
  CoinDisjointCopyN(m->getVectorLengths(), nrows_,  hinrow_);
  CoinDisjointCopyN(m->getIndices(),       nelems_, hcol_);
  CoinDisjointCopyN(m->getElements(),      nelems_, rowels_);
  start = m->getVectorStarts();
  length = m->getVectorLengths();
  const int * column = m->getIndices();
  element = m->getElements();
  // out zeros
  int irow;
  nel=0;
  mrstrt_[0]=0;
  for (irow=0;irow<nrows_;irow++) {
    int j;
    for (j=start[irow];j<start[irow]+length[irow];j++) {
      if (fabs(element[j])>ZTOLDP) {
        hcol_[nel]=column[j];
        rowels_[nel++]=element[j];
      }
    }
    hinrow_[irow]=nel-mrstrt_[irow];
    mrstrt_[irow+1]=nel;
  }
  nelems_=nel;

  delete m;
  {
    int i;
    for (i=0;i<ncols_;i++) {
      if (si->isInteger(i))
	integerType_[i] = 1;
      else
	integerType_[i] = 0;
    }
  }

  // Set up prohibited bits if needed
  if (nonLinearValue) {
    anyProhibited_ = true;
    for (icol=0;icol<ncols_;icol++) {
      int j;
      bool nonLinearColumn = false;
      if (cost_[icol]==nonLinearValue)
	nonLinearColumn=true;
      for (j=mcstrt_[icol];j<mcstrt_[icol+1];j++) {
	if (colels_[j]==nonLinearValue) {
	  nonLinearColumn=true;
	  setRowProhibited(hrow_[j]);
	}
      }
      if (nonLinearColumn)
	setColProhibited(icol);
    }
  } else if (prohibited) {
    anyProhibited_ = true;
    for (icol=0;icol<ncols_;icol++) {
      if (prohibited[icol])
	setColProhibited(icol);
    }
  } else {
    anyProhibited_ = false;
  }
  // Any rows special?
  if (rowProhibited) {
    anyProhibited_ = true;
    for (int irow=0;irow<nrows_;irow++) {
      if (rowProhibited[irow])
	setRowProhibited(irow);
    }
  }
  if (doStatus) {
    // allow for status and solution
    sol_ = new double[ncols_];
    const double *presol ;
    presol = si->getColSolution() ;
    memcpy(sol_,presol,ncols_*sizeof(double));
    acts_ = new double [nrows_];
    memcpy(acts_,si->getRowActivity(),nrows_*sizeof(double));
    CoinWarmStartBasis * basis  =
    dynamic_cast<CoinWarmStartBasis*>(si->getWarmStart());
    colstat_ = new unsigned char [nrows_+ncols_];
    rowstat_ = colstat_+ncols_;
    // If basis is NULL then put in all slack basis
    if (basis&&basis->getNumStructural()==ncols_) {
      int i;
      for (i=0;i<ncols_;i++) {
	colstat_[i] = basis->getStructStatus(i);
      }
      for (i=0;i<nrows_;i++) {
	rowstat_[i] = basis->getArtifStatus(i);
      }
    } else {
      int i;
      // no basis
      for (i=0;i<ncols_;i++) {
	colstat_[i] = 3;
      }
      for (i=0;i<nrows_;i++) {
	rowstat_[i] = 1;
      }
    }
    delete basis;
  }

#if 0
  for (i=0; i<nrows; ++i)
    printf("NR: %6d\n", hinrow[i]);
  for (int i=0; i<ncols; ++i)
    printf("NC: %6d\n", hincol[i]);
#endif

#if 0 /* for building against CoinUtils 2.6, this #if 1 need to be changed into an #if 0 */
  presolve_make_memlists(mcstrt_, hincol_, clink_, ncols_);
  presolve_make_memlists(mrstrt_, hinrow_, rlink_, nrows_);
#else
  presolve_make_memlists(/*mcstrt_,*/ hincol_, clink_, ncols_);
  presolve_make_memlists(/*mrstrt_,*/ hinrow_, rlink_, nrows_);
#endif

  // this allows last col/row to expand up to bufsize-1 (22);
  // this must come after the calls to presolve_prefix
  mcstrt_[ncols_] = bufsize-1;
  mrstrt_[nrows_] = bufsize-1;
  // Allocate useful arrays
  initializeStuff();

#if	CHECK_CONSISTENCY
  consistent(false);
#endif
}

void CoinPresolveMatrix::update_model(OsiSolverInterface * si,
				      int /*nrows0*/,
				      int /*ncols0*/,
				      CoinBigIndex /*nelems0*/)
{
  int nels=0;
  int i;
  for ( i=0; i<ncols_; i++)
    nels += hincol_[i];
  CoinPackedMatrix m(true,nrows_,ncols_,nels, colels_, hrow_,mcstrt_,hincol_);
  si->loadProblem(m, clo_, cup_, cost_, rlo_, rup_);

  for ( i=0; i<ncols_; i++) {
    if (integerType_[i])
      si->setInteger(i);
    else
      si->setContinuous(i);
  }

#if	PRESOLVE_SUMMARY
  printf("NEW NCOL/NROW/NELS:  %d(-%d) %d(-%d) %d(-%d)\n",
	 ncols_, ncols0-ncols_,
	 nrows_, nrows0-nrows_,
	 si->getNumElements(), nelems0-si->getNumElements());
#endif
  si->setDblParam(OsiObjOffset,originalOffset_-dobias_);

}











////////////////  POSTSOLVE

CoinPostsolveMatrix::CoinPostsolveMatrix(OsiSolverInterface*  si,
				       int ncols0_in,
				       int nrows0_in,
				       CoinBigIndex nelems0,

				       double maxmin,
				       // end prepost members

				       double *sol_in,
				       double *acts_in,

				       unsigned char *colstat_in,
				       unsigned char *rowstat_in) :

  CoinPrePostsolveMatrix(si, ncols0_in, nrows0_in, nelems0),
/*
  Used only to mark processed columns and rows so that debugging routines know
  what to check.
*/
# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
  cdone_(new char[ncols0_in]),
  rdone_(new char[nrows0_in])
# else
  cdone_(0),
  rdone_(0)
# endif

{
/*
  The CoinPrePostsolveMatrix constructor will set bulk0_ to bulkRatio_*nelems0.
  By default, bulkRatio_ is 2. This is certainly larger than absolutely
  necessary, but good for efficiency (minimises the need to compress the bulk
  store). The main storage arrays for the threaded column-major representation
  (hrow_, colels_, link_) should be allocated to this size.
*/
  free_list_ = 0 ;
  maxlink_ = bulk0_ ;
  link_ = new int[maxlink_] ;

  nrows_ = si->getNumRows() ;
  ncols_ = si->getNumCols() ;

  sol_=sol_in;
  rowduals_=NULL;
  acts_=acts_in;

  rcosts_=NULL;
  colstat_=colstat_in;
  rowstat_=rowstat_in;

  // this is the *reduced* model, which is probably smaller
  int ncols1 = ncols_ ;
  int nrows1 = nrows_ ;

  const CoinPackedMatrix * m = si->getMatrixByCol();
#if 0
  if (! isGapFree(*m)) {
    CoinPresolveAction::throwCoinError("Matrix not gap free",
				      "CoinPostsolveMatrix");
  }
#endif
  const CoinBigIndex nelemsr = m->getNumElements();

  if (isGapFree(*m)) {
    CoinDisjointCopyN(m->getVectorStarts(), ncols1, mcstrt_);
    CoinZeroN(mcstrt_+ncols1,ncols0_-ncols1);
    mcstrt_[ncols_] = nelems0;	// points to end of bulk store
    CoinDisjointCopyN(m->getVectorLengths(),ncols1,  hincol_);
    CoinDisjointCopyN(m->getIndices(),      nelemsr, hrow_);
    CoinDisjointCopyN(m->getElements(),     nelemsr, colels_);
  }
  else
  {
    CoinPackedMatrix* mm = new CoinPackedMatrix(*m);
    if( mm->hasGaps())
      mm->removeGaps();
    assert(nelemsr == mm->getNumElements());
    CoinDisjointCopyN(mm->getVectorStarts(), ncols1, mcstrt_);
    CoinZeroN(mcstrt_+ncols1,ncols0_-ncols1);
    mcstrt_[ncols_] = nelems0;  // points to end of bulk store
    CoinDisjointCopyN(mm->getVectorLengths(),ncols1,  hincol_);
    CoinDisjointCopyN(mm->getIndices(),      nelemsr, hrow_);
    CoinDisjointCopyN(mm->getElements(),     nelemsr, colels_);
  }

# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
  memset(cdone_, -1, ncols0_);
  memset(rdone_, -1, nrows0_);
# endif

  rowduals_ = new double[nrows0_];
  CoinDisjointCopyN(si->getRowPrice(), nrows1, rowduals_);
  rcosts_ = new double[ncols0_];
  CoinDisjointCopyN(si->getReducedCost(), ncols1, rcosts_);

#if PRESOLVE_DEBUG
  // check accuracy of reduced costs (rcosts_ is recalculated reduced costs)
  si->getMatrixByCol()->transposeTimes(rowduals_,rcosts_);
  const double * obj =si->getObjCoefficients();
  const double * dj =si->getReducedCost();
  {
    int i;
    for (i=0;i<ncols1;i++) {
      double newDj = obj[i]-rcosts_[i];
      rcosts_[i]=newDj;
      assert (fabs(newDj-dj[i])<1.0e-1);
    }
  }
  // check reduced costs are 0 for basic variables
  {
    int i;
    for (i=0;i<ncols1;i++)
      if (columnIsBasic(i))
	assert (fabs(rcosts_[i])<1.0e-5);
    for (i=0;i<nrows1;i++)
      if (rowIsBasic(i))
	assert (fabs(rowduals_[i])<1.0e-5);
  }
#endif

  if (maxmin<0.0) {
    // change so will look as if minimize
    int i;
    for (i=0;i<nrows1;i++)
      rowduals_[i] = - rowduals_[i];
    for (i=0;i<ncols1;i++) {
      rcosts_[i] = - rcosts_[i];
    }
  }

/*
  CoinPresolve requires both column solution and row activity for correct
  operation.
*/
  CoinDisjointCopyN(si->getColSolution(), ncols1, sol_);
  CoinDisjointCopyN(si->getRowActivity(), nrows1, acts_) ;
  si->setDblParam(OsiObjOffset,originalOffset_);

  for (int j=0; j<ncols1; j++) {
    CoinBigIndex kcs = mcstrt_[j];
    CoinBigIndex kce = kcs + hincol_[j];
    for (CoinBigIndex k=kcs; k<kce; ++k) {
      link_[k] = k+1;
    }
    if (kce>0)
      link_[kce-1] = NO_LINK ;
  }
  if (maxlink_>0) {
    int ml = maxlink_;
    for (CoinBigIndex k=nelemsr; k<ml; ++k)
      link_[k] = k+1;
    link_[ml-1] = NO_LINK;
  }
  free_list_ = nelemsr;

# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
/*
  These are used to track the action of postsolve transforms during debugging.
*/
  CoinFillN(cdone_,ncols1,PRESENT_IN_REDUCED) ;
  CoinZeroN(cdone_+ncols1,ncols0_in-ncols1) ;
  CoinFillN(rdone_,nrows1,PRESENT_IN_REDUCED) ;
  CoinZeroN(rdone_+nrows1,nrows0_in-nrows1) ;
# endif
}
