/* $Id: CoinPresolveForcing.cpp 1448 2011-06-19 15:34:41Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinPresolveForcing.hpp"
#include "CoinMessage.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

/*
  This just doesn't seem efficient, particularly when used to calculate row
  bounds. Lots of extra work.
*/
void implied_bounds (const double *els,
		     const double *clo, const double *cup,
		     const int *hcol,
		     CoinBigIndex krs, CoinBigIndex kre,
		     double *maxupp, double *maxdownp,
		     int jcol,
		     double rlo, double rup,
		     double *iclb, double *icub)
{
  if (rlo<=-PRESOLVE_INF&&rup>=PRESOLVE_INF) {
    *iclb = -PRESOLVE_INF;
    *icub =  PRESOLVE_INF;
    return;
  }
  bool posinf = false;
  bool neginf = false;
  double maxup = 0.0;
  double maxdown = 0.0;

  int jcolk = -1;

  // compute sum of all bounds except for jcol
  CoinBigIndex kk;
  for (kk=krs; kk<kre; kk++) {
    if (hcol[kk] == jcol)
      jcolk = kk;

    // swap jcol with hcol[kre-1];
    // that is, consider jcol last
    // this assumes that jcol occurs in this row
    CoinBigIndex k = (hcol[kk] == jcol
	     ? kre-1
	     : kk == kre-1
	     ? jcolk
	     : kk);

    PRESOLVEASSERT(k != -1);	// i.e. jcol had better be in the row

    int col = hcol[k];
    double coeff = els[k];
    double lb = clo[col];
    double ub = cup[col];

    // quick!  compute the implied col bounds before maxup/maxdown are changed
    if (kk == kre-1) {
      PRESOLVEASSERT(fabs(coeff) > ZTOLDP);

      double ilb = (rlo - maxup) / coeff;
      bool finite_ilb = (-PRESOLVE_INF < rlo && !posinf && PRESOLVEFINITE(maxup));

      double iub = (rup - maxdown) / coeff;
      bool finite_iub = ( rup < PRESOLVE_INF && !neginf && PRESOLVEFINITE(maxdown));

      if (coeff > 0.0) {
	*iclb = (finite_ilb ? ilb : -PRESOLVE_INF);
	*icub = (finite_iub ? iub :  PRESOLVE_INF);
      } else {
	*iclb = (finite_iub ? iub : -PRESOLVE_INF);
	*icub = (finite_ilb ? ilb :  PRESOLVE_INF);
      }
    }

    if (coeff > 0.0) {
      if (PRESOLVE_INF <= ub) {
	posinf = true;
	if (neginf)
	  break;	// pointless
      } else
	maxup += ub * coeff;

      if (lb <= -PRESOLVE_INF) {
	neginf = true;
	if (posinf)
	  break;	// pointless
      } else
	maxdown += lb * coeff;
    } else {
      if (PRESOLVE_INF <= ub) {
	neginf = true;
	if (posinf)
	  break;	// pointless
      } else
	maxdown += ub * coeff;

      if (lb <= -PRESOLVE_INF) {
	posinf = true;
	if (neginf)
	  break;	// pointless
      } else
	maxup += lb * coeff;
    }
  }

  // If we broke from the loop, then the bounds are infinite.
  // However, since we put the column whose implied bounds we want
  // to know at the end, and it doesn't matter if its own bounds
  // are infinite, don't worry about breaking at the last iteration.
  if (kk<kre-1) {
    *iclb = -PRESOLVE_INF;
    *icub =  PRESOLVE_INF;
  } else
    PRESOLVEASSERT(jcolk != -1);

  // store row bounds
  *maxupp   = (posinf) ?  PRESOLVE_INF : maxup;
  *maxdownp = (neginf) ? -PRESOLVE_INF : maxdown;
}

static void implied_row_bounds(const double *els,
			       const double *clo, const double *cup,
			       const int *hcol,
			       CoinBigIndex krs, CoinBigIndex kre,
			       double *maxupp, double *maxdownp)
{
  int jcol = hcol[krs];
  bool posinf = false;
  bool neginf = false;
  double maxup = 0.0;
  double maxdown = 0.0;

  int jcolk = -1;

  // compute sum of all bounds except for jcol
  CoinBigIndex kk;
  for (kk=krs; kk<kre; kk++) {
    if (hcol[kk] == jcol)
      jcolk = kk;

    // swap jcol with hcol[kre-1];
    // that is, consider jcol last
    // this assumes that jcol occurs in this row
    CoinBigIndex k = (hcol[kk] == jcol
	     ? kre-1
	     : kk == kre-1
	     ? jcolk
	     : kk);

    PRESOLVEASSERT(k != -1);	// i.e. jcol had better be in the row

    int col = hcol[k];
    double coeff = els[k];
    double lb = clo[col];
    double ub = cup[col];

    if (coeff > 0.0) {
      if (PRESOLVE_INF <= ub) {
	posinf = true;
	if (neginf)
	  break;	// pointless
      } else
	maxup += ub * coeff;

      if (lb <= -PRESOLVE_INF) {
	neginf = true;
	if (posinf)
	  break;	// pointless
      } else
	maxdown += lb * coeff;
    } else {
      if (PRESOLVE_INF <= ub) {
	neginf = true;
	if (posinf)
	  break;	// pointless
      } else
	maxdown += ub * coeff;

      if (lb <= -PRESOLVE_INF) {
	posinf = true;
	if (neginf)
	  break;	// pointless
      } else
	maxup += lb * coeff;
    }
  }
  // store row bounds
  *maxupp   = (posinf) ?  PRESOLVE_INF : maxup;
  *maxdownp = (neginf) ? -PRESOLVE_INF : maxdown;
}

const char *forcing_constraint_action::name() const
{
  return ("forcing_constraint_action");
}
// defed out to avoid compiler warning
#if 0
static bool some_col_was_fixed(const int *hcol, CoinBigIndex krs, CoinBigIndex kre,
			       const double *clo,
			       const double *cup)
{
  CoinBigIndex k;
  for (k=krs; k<kre; k++) {
    int jcol = hcol[k];
    if (clo[jcol] == cup[jcol])
      break;
  }
  return (k<kre);
}
#endif

//
// It may be the case that the variable bounds are such that no matter what
// feasible value they take, the constraint cannot be violated;
// in this case, we obviously don't need to take it into account, and
// we just drop it as a USELESS constraint.
//
// On the other hand, it may be that the only way to satisfy a constraint
// is to jam all the constraint variables to one of their bounds;
// in this case, these variables are essentially fixed, which
// we do with a FORCING constraint.
// For now, this just tightens the bounds; subsequently the fixed variables
// will be removed, then the row will be dropped.
//
// Since both operations use similar information (the implied row bounds),
// we combine them into one presolve routine.
//
// I claim that these checks could be performed in parallel,
// that is, the tests could be carried out for all rows in parallel,
// and then the rows deleted and columns tightened afterward.
// Obviously, this is true for useless rows.
// The potential problem is forcing constraints, but I think
// that is ok.
// By doing it in parallel rather than sequentially,
// we may miss transformations due to variables that were fixed
// by forcing constraints, though.
//
// Note that both of these operations will cause problems
// if the variables in question really need to exceed their bounds in order
// to make the problem feasible.

const CoinPresolveAction
    *forcing_constraint_action::presolve(CoinPresolveMatrix *prob,
					 const CoinPresolveAction *next)
{
  double startTime = 0.0;
  int startEmptyRows=0;
  int startEmptyColumns = 0;
  if (prob->tuning_) {
    startTime = CoinCpuTime();
    startEmptyRows = prob->countEmptyRows();
    startEmptyColumns = prob->countEmptyCols();
  }
  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *csol  = prob->sol_ ;

  const double *rowels	= prob->rowels_;
  const int *hcol	= prob->hcol_;
  const CoinBigIndex *mrstrt	= prob->mrstrt_;
  const int *hinrow	= prob->hinrow_;
  const int nrows	= prob->nrows_;

  const double *rlo	= prob->rlo_;
  const double *rup	= prob->rup_;

  //  const char *integerType = prob->integerType_;

  const double tol	= ZTOLDP;
  const double inftol	= prob->feasibilityTolerance_;
  const int ncols	= prob->ncols_;

  int *fixed_cols	= new int[ncols];
  int nfixed_cols	= 0;

  action *actions	= new action [nrows];
  int nactions = 0;

  int *useless_rows	= new int[nrows];
  int nuseless_rows	= 0;

  int numberLook = prob->numberRowsToDo_;
  int iLook;
  int * look = prob->rowsToDo_;
  bool fixInfeasibility = (prob->presolveOptions_&16384)!=0;

# if PRESOLVE_DEBUG
  std::cout << "Entering forcing_constraint_action::presolve." << std::endl ;
  presolve_check_sol(prob) ;
# endif
/*
  Open a loop to scan the constraints of interest. There must be variables
  left in the row.
*/
  for (iLook=0;iLook<numberLook;iLook++) {
    int irow = look[iLook];
    if (hinrow[irow] > 0) {
      CoinBigIndex krs = mrstrt[irow];
      CoinBigIndex kre = krs + hinrow[irow];
/*
  Calculate upper and lower bounds on the row activity based on upper and lower
  bounds on the variables. If these are finite and incompatible with the given
  row bounds, we have infeasibility.
*/
      double maxup, maxdown;
      implied_row_bounds(rowels, clo, cup, hcol, krs, kre,
			 &maxup, &maxdown);

      if (maxup < PRESOLVE_INF && maxup + inftol < rlo[irow]&&!fixInfeasibility) {
	/* max row activity below the row lower bound */
	prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					     prob->messages())
				 	       <<irow
					       <<rlo[irow]
					       <<rup[irow]
					       <<CoinMessageEol;
	break;
      } else if (-PRESOLVE_INF < maxdown && rup[irow] < maxdown - inftol&&!fixInfeasibility) {
	/* min row activity above the row upper bound */
	prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					     prob->messages())
					       <<irow
					       <<rlo[irow]
					       <<rup[irow]
					       <<CoinMessageEol;
	break;
      }
      // ADD TOLERANCE TO THESE TESTS
      else if ((rlo[irow] <= -PRESOLVE_INF ||
		(-PRESOLVE_INF < maxdown && rlo[irow] <= maxdown)) &&
	       (rup[irow] >= PRESOLVE_INF ||
		(maxup < PRESOLVE_INF && rup[irow] >= maxup))) {

/*
  Original comment: I'm not sure that these transforms don't intefere with
		    each other. We can get it next time.

  Well, I'll argue that bounds are never really loosened (at worst, they're
  transferred onto some other variable, or inferred to be unnecessary.
  Once useless, always useless. Leaving this hook in place allows for a sort
  of infinite loop where this routine keeps queuing the same constraints over
  and over.  -- lh, 040901 --

	 if (some_col_was_fixed(hcol, krs, kre, clo, cup)) {
	   prob->addRow(irow);
	   continue;
	 }
*/

	// this constraint must always be satisfied - drop it
	useless_rows[nuseless_rows++] = irow;

      } else if ((maxup < PRESOLVE_INF && fabs(rlo[irow] - maxup) < tol) ||
		 (-PRESOLVE_INF < maxdown && fabs(rup[irow] - maxdown) < tol)) {

	// the lower bound can just be reached, or
	// the upper bound can just be reached;
	// called a "forcing constraint" in the paper (p. 226)
	const int lbound_tight = (maxup < PRESOLVE_INF &&
				  fabs(rlo[irow] - maxup) < tol);

/*
  Original comment and rebuttal as above.
	if (some_col_was_fixed(hcol, krs, kre, clo, cup)) {
	  // make sure on next time
	  prob->addRow(irow);
	  continue;
	}
*/
	// out of space - this probably never happens (but this routine will
	// often put duplicates in the fixed column list)
	if (nfixed_cols + (kre-krs) >= ncols)
	  break;

	double *bounds = new double[hinrow[irow]];
	int *rowcols = new int[hinrow[irow]];
	int lk = krs;	// load fix-to-down in front
	int uk = kre;	// load fix-to-up in back
        CoinBigIndex k;
	for ( k=krs; k<kre; k++) {
	  int jcol = hcol[k];
	  prob->addCol(jcol);
	  double coeff = rowels[k];

	  PRESOLVEASSERT(fabs(coeff) > ZTOLDP);

	  // one of the two contributed to maxup - set the other to that
	  if (lbound_tight == (coeff > 0.0)) {
	    --uk;
	    bounds[uk-krs] = clo[jcol];
	    rowcols[uk-krs] = jcol;
	    if (csol != 0) {
	      csol[jcol] = cup[jcol] ;
	    }
	    clo[jcol] = cup[jcol];
	  } else {
	    bounds[lk-krs] = cup[jcol];
	    rowcols[lk-krs] = jcol;
	    ++lk;
	    if (csol != 0) {
	      csol[jcol] = clo[jcol] ;
	    }
	    cup[jcol] = clo[jcol];
	  }

	  fixed_cols[nfixed_cols++] = jcol;
	}
	PRESOLVEASSERT(uk == lk);

	action *f = &actions[nactions];
	nactions++;
	PRESOLVE_DETAIL_PRINT(printf("pre_forcing %dR E\n",irow));

	f->row = irow;
	f->nlo = lk-krs;
	f->nup = kre-uk;
	f->rowcols = rowcols;
	f->bounds = bounds;
      }
    }
  }


  if (nactions) {
#if	PRESOLVE_SUMMARY
    printf("NFORCED:  %d\n", nactions);
#endif
    next = new forcing_constraint_action(nactions,
					 CoinCopyOfArray(actions,nactions), next);
  }
  deleteAction(actions,action*);
  if (nuseless_rows) {
    next = useless_constraint_action::presolve(prob,
					       useless_rows, nuseless_rows,
					       next);
  }
  delete[]useless_rows;
/*
  We need to remove duplicates here, or we get into trouble in
  remove_fixed_action::postsolve when we try to reinstate a column multiple
  times.
*/
  if (nfixed_cols)
  { if (nfixed_cols > 1)
    { std::sort(fixed_cols,fixed_cols+nfixed_cols) ;
      int *end = std::unique(fixed_cols,fixed_cols+nfixed_cols) ;
      nfixed_cols = static_cast<int>(end-fixed_cols) ; }
    next = remove_fixed_action::presolve(prob,fixed_cols,nfixed_cols,next) ; }
  delete[]fixed_cols ;

  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    int droppedRows = prob->countEmptyRows() - startEmptyRows ;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
    printf("CoinPresolveForcing(32) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_);
  }

# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
  std::cout << "Leaving forcing_constraint_action::presolve." << std::endl ;
# endif

  return (next);
}

void forcing_constraint_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

  const double *colels	= prob->colels_;
  const int *hrow		= prob->hrow_;
  const CoinBigIndex *mcstrt		= prob->mcstrt_;
  const int *hincol		= prob->hincol_;
  const int *link		= prob->link_;

  //  CoinBigIndex free_list = prob->free_list_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  const double *sol	= prob->sol_;
  double *rcosts	= prob->rcosts_;

  double *acts	= prob->acts_;
  double *rowduals = prob->rowduals_;

  const double ztoldj	= prob->ztoldj_;
  const double ztolzb	= prob->ztolzb_;

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {

    const int irow	= f->row;
    const int nlo	= f->nlo;
    const int nup	= f->nup;
    const int ninrow	= nlo + nup;
    const int *rowcols	= f->rowcols;
    const double *bounds= f->bounds;
    int k;
/*
  Original comment: When we restore bounds here, we need to allow for the
	possibility that the restored bound is infinite. This implies a check
	for viable status.

  Hmmm ... I'm going to argue that in fact we have no choice: the status
  of the variable must reflect the value it was fixed at, else we lose
  feasibility. We don't care what the other bound does.   -- lh, 040903 --
*/
    for (k=0; k<nlo; k++) {
      int jcol = rowcols[k];
      cup[jcol] = bounds[k];
      prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atLowerBound) ;
/*
      PRESOLVEASSERT(prob->getColumnStatus(jcol)!=CoinPrePostsolveMatrix::basic);
      if (cup[jcol] >= PRESOLVE_INF)
      { CoinPrePostsolveMatrix::Status statj = prob->getColumnStatus(jcol) ;
	if (statj == CoinPrePostsolveMatrix::atUpperBound)
	{ if (clo[jcol] > -PRESOLVE_INF)
	  { statj = CoinPrePostsolveMatrix::atLowerBound ; }
	  else
	  { statj = CoinPrePostsolveMatrix::isFree ; }
	  prob->setColumnStatus(jcol,statj) ; } }
*/
    }

    for (k=nlo; k<ninrow; k++) {
      int jcol = rowcols[k];
      clo[jcol] = bounds[k];
      prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atUpperBound) ;
/*
      PRESOLVEASSERT(prob->getColumnStatus(jcol)!=CoinPrePostsolveMatrix::basic);
      if (clo[jcol] <= -PRESOLVE_INF)
      { CoinPrePostsolveMatrix::Status statj = prob->getColumnStatus(jcol) ;
	if (statj == CoinPrePostsolveMatrix::atLowerBound)
	{ if (cup[jcol] < PRESOLVE_INF)
	  { statj = CoinPrePostsolveMatrix::atUpperBound ; }
	  else
	  { statj = CoinPrePostsolveMatrix::isFree ; }
	  prob->setColumnStatus(jcol,statj) ; } }
*/
    }

    PRESOLVEASSERT(prob->getRowStatus(irow)==CoinPrePostsolveMatrix::basic);
    PRESOLVEASSERT(rowduals[irow] == 0.0);

    // this is a lazy implementation.
    // we tightened the col bounds, then let them be eliminated
    // by repeated uses of FIX_VARIABLE and a final DROP_ROW.
    // Therefore, by this point the row has been marked basic,
    // the rowdual of this row is 0.0,
    // and the reduced costs for the cols may or may not be ok
    // for the relaxed column bounds.
    //
    // find the one most out of whack and fix it.
    int whacked = -1;
    double whack = 0.0;
    for (k=0; k<ninrow; k++) {
      int jcol = rowcols[k];
      CoinBigIndex kk = presolve_find_row2(irow, mcstrt[jcol], hincol[jcol], hrow, link);

      // choose rowdual to cancel out reduced cost
      double whack0 = rcosts[jcol] / colels[kk];

      if (((rcosts[jcol] > ztoldj  && !(fabs(sol[jcol] - clo[jcol]) <= ztolzb)) ||
	   (rcosts[jcol] < -ztoldj && !(fabs(sol[jcol] - cup[jcol]) <= ztolzb))) &&
	  fabs(whack0) > fabs(whack)) {
	whacked = jcol;
	whack = whack0;
      }
    }

    if (whacked != -1) {
      prob->setColumnStatus(whacked,CoinPrePostsolveMatrix::basic);
      if (acts[irow]-rlo[irow]<rup[irow]-acts[irow])
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::atLowerBound);
      else
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::atUpperBound);
      rowduals[irow] = whack;

      for (k=0; k<ninrow; k++) {
	int jcol = rowcols[k];
	CoinBigIndex kk = presolve_find_row2(irow, mcstrt[jcol], hincol[jcol], hrow, link);

	rcosts[jcol] -= (rowduals[irow] * colels[kk]);
      }
    }
  }

# if PRESOLVE_CONSISTENCY
  presolve_check_threads(prob) ;
# endif

}



#if 0		// (A)
// Determine the maximum and minimum values the constraint sums
// may take, given the bounds on the variables.
// If there are infinite terms, record where the first one is,
// and whether there is more than one.
// It is possible to compute implied bounds for the (one) variable
// with no bound.
static void implied_bounds1(CoinPresolveMatrix * prob, const double *rowels,
				const int *mrstrt,
				const int *hrow,
				const int *hinrow,
				const double *clo, const double *cup,
				const int *hcol,
				int ncols,
				const double *rlo, const double *rup,
				const char *integerType,
				int nrows,
				double *ilbound, double *iubound)
{
  const double tol = prob->feasibilityTolerance_;

  for (int irow=0; irow<nrows; irow++) {
    CoinBigIndex krs = mrstrt[irow];
    CoinBigIndex kre = krs + hinrow[irow];

    double irlo = rlo[irow];
    double irup = rup[irow];

    // These are used to set column bounds below.
    // If there are no (positive) infinite terms,
    // the loop will range from krs to kre;
    // if there is just one, it will range over that one variable;
    // otherwise, it will be empty.
    int ub_inf_index = -1;
    int lb_inf_index = -1;

    double maxup = 0.0;
    double maxdown = 0.0;
    CoinBigIndex k;
    for (k=krs; k<kre; k++) {
      int jcol = hcol[k];
      double coeff = rowels[k];
      double lb = clo[jcol];
      double ub = cup[jcol];

      // HAVE TO DEAL WITH BOUNDS OF INTEGER VARIABLES
      if (coeff > 0.0) {
	if (PRESOLVE_INF <= ub) {
	  if (ub_inf_index == -1) {
	    ub_inf_index = k;
	  } else {
	    ub_inf_index = -2;
	    if (lb_inf_index == -2)
	      break;	// pointless
	  }
	} else
	  maxup += ub * coeff;

	if (lb <= -PRESOLVE_INF) {
	  if (lb_inf_index == -1) {
	    lb_inf_index = k;
	  } else {
	    lb_inf_index = -2;
	    if (ub_inf_index == -2)
	      break;	// pointless
	  }
	} else
	  maxdown += lb * coeff;
      }
      else {
	if (PRESOLVE_INF <= ub) {
	  if (lb_inf_index == -1) {
	    lb_inf_index = k;
	  } else {
	    lb_inf_index = -2;
	    if (ub_inf_index == -2)
	      break;	// pointless
	  }
	} else
	  maxdown += ub * coeff;

	if (lb <= -PRESOLVE_INF) {
	  if (ub_inf_index == -1) {
	    ub_inf_index = k;
	  } else {
	    ub_inf_index = -2;
	    if (lb_inf_index == -2)
	      break;	// pointless
	  }
	} else
	  maxup += lb * coeff;
      }
    }

    // ub_inf says whether the sum of the "other" ub terms is infinite
    // in the loop below.
    // In the case where we only saw one infinite term, the loop
    // will only cover that case, in which case the other terms
    // are *not* infinite.
    // With two or more terms, it is infinite.
    // If we only saw one infinite term, then
    if (ub_inf_index == -2)
      maxup = PRESOLVE_INF;

    if (lb_inf_index == -2)
      maxdown = -PRESOLVE_INF;

    const bool maxup_finite = PRESOLVEFINITE(maxup);
    const bool maxdown_finite = PRESOLVEFINITE(maxdown);

    if (ub_inf_index == -1 && maxup_finite && maxup + tol < rlo[irow]&&!fixInfeasibility) {
      /* infeasible */
	prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					     prob->messages())
					       <<irow
					       <<rlo[irow]
					       <<rup[irow]
					       <<CoinMessageEol;
	break;
    } else if (lb_inf_index == -1 && maxdown_finite && rup[irow] < maxdown - tol&&!fixInfeasibility) {
      /* infeasible */
	prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					     prob->messages())
					       <<irow
					       <<rlo[irow]
					       <<rup[irow]
					       <<CoinMessageEol;
	break;
    }

    for (k = krs; k<kre; ++k) {
      int jcol = hcol[k];
      double coeff = rowels[k];

      // SHOULD GET RID OF THIS
      if (fabs(coeff) > ZTOLDP2 &&
	  !integerType[jcol]) {
	double maxup1 = (ub_inf_index == -1 || ub_inf_index == k
			 ? maxup
			 : PRESOLVE_INF);
	bool maxup_finite1 = (ub_inf_index == -1 || ub_inf_index == k
			      ? maxup_finite
			      : false);
	double maxdown1 = (lb_inf_index == -1 || lb_inf_index == k
			 ? maxdown
			 : PRESOLVE_INF);
	bool maxdown_finite1 = (ub_inf_index == -1 || ub_inf_index == k
			      ? maxdown_finite
			      : false);

	double ilb = (irlo - maxup1) / coeff;
	bool finite_ilb = (-PRESOLVE_INF < irlo && maxup_finite1);

	double iub = (irup - maxdown1) / coeff;
	bool finite_iub = ( irup < PRESOLVE_INF && maxdown_finite1);

	double ilb1 = (coeff > 0.0
		       ? (finite_ilb ? ilb : -PRESOLVE_INF)
		       : (finite_iub ? iub : -PRESOLVE_INF));

	if (ilbound[jcol] < ilb1) {
	  ilbound[jcol] = ilb1;
	  //if (jcol == 278001)
	  //printf("JCOL LB %g\n", ilb1);
	}
      }
    }

    for (k = krs; k<kre; ++k) {
      int jcol = hcol[k];
      double coeff = rowels[k];

      // SHOULD GET RID OF THIS
      if (fabs(coeff) > ZTOLDP2 &&
	  !integerType[jcol]) {
	double maxup1 = (ub_inf_index == -1 || ub_inf_index == k
			 ? maxup
			 : PRESOLVE_INF);
	bool maxup_finite1 = (ub_inf_index == -1 || ub_inf_index == k
			      ? maxup_finite
			      : false);
	double maxdown1 = (lb_inf_index == -1 || lb_inf_index == k
			 ? maxdown
			 : PRESOLVE_INF);
	bool maxdown_finite1 = (ub_inf_index == -1 || ub_inf_index == k
			      ? maxdown_finite
			      : false);


	double ilb = (irlo - maxup1) / coeff;
	bool finite_ilb = (-PRESOLVE_INF < irlo && maxup_finite1);

	double iub = (irup - maxdown1) / coeff;
	bool finite_iub = ( irup < PRESOLVE_INF && maxdown_finite1);

	double iub1 = (coeff > 0.0
		       ? (finite_iub ? iub :  PRESOLVE_INF)
		       : (finite_ilb ? ilb :  PRESOLVE_INF));

	if (iub1 < iubound[jcol]) {
	  iubound[jcol] = iub1;
	  //if (jcol == 278001)
	  //printf("JCOL UB %g\n", iub1);
	}
      }
    }
  }
}

#if 0		// (B)
postsolve for implied_bound
	{
	  double lo0	= pa->clo;
	  double up0	= pa->cup;
	  int irow	= pa->irow;
	  int jcol	= pa->icol;
	  int *rowcols	= pa->rowcols;
	  int ninrow	= pa->ninrow;

	  clo[jcol] = lo0;
	  cup[jcol] = up0;

	  if ((colstat[jcol] & PRESOLVE_XBASIC) == 0 &&
	      fabs(lo0 - sol[jcol]) > ztolzb &&
	      fabs(up0 - sol[jcol]) > ztolzb) {

	    // this non-basic variable is now away from its bound
	    // it is ok just to force it to be basic
	    // informally:  if this variable is at its implied bound,
	    // then the other variables must be at their bounds,
	    // which means the bounds will stop them even if the aren't basic.
	    if (rowstat[irow] & PRESOLVE_XBASIC)
	      rowstat[irow] = 0;
	    else {
	      int k;
	      for (k=0; k<ninrow; k++) {
		int col = rowcols[k];
		if (cdone[col] &&
		    (colstat[col] & PRESOLVE_XBASIC) &&
		    ((fabs(clo[col] - sol[col]) <= ztolzb && rcosts[col] >= -ztoldj) ||
		     (fabs(cup[col] - sol[col]) <= ztolzb && rcosts[col] <= ztoldj)))
		  break;
	      }
	      if (k<ninrow) {
		int col = rowcols[k];
		// steal this basic variable
#if	PRESOLVE_DEBUG
		printf("PIVOTING ON COL:  %d %d -> %d\n", irow, col, jcol);
#endif
		colstat[col] = 0;

		// since all vars were at their bounds, the slack must be 0
		PRESOLVEASSERT(fabs(acts[irow]) < ZTOLDP);
		rowstat[irow] = PRESOLVE_XBASIC;
	      }
	      else {
		// should never happen?
		abort();
	      }

	      // get rid of any remaining basic structurals, since their rcosts
	      // are going to become non-zero in a second.
	      abort();
	      ///////////////////zero_pivot();
	    }

	    double rdual_adjust;
	    {
	      CoinBigIndex kk = presolve_find_row(irow, mcstrt[jcol], mcstrt[jcol] + hincol[jcol], hrow);
	      // adjust rowdual to cancel out reduced cost
	      // should probably search for col with largest factor
	      rdual_adjust = (rcosts[jcol] / colels[kk]);
	      rowduals[irow] += rdual_adjust;
	      colstat[jcol] = PRESOLVE_XBASIC;
	    }

	    for (k=0; k<ninrow; k++) {
	      int jcol = rowcols[k];
	      CoinBigIndex kk = presolve_find_row(irow, mcstrt[jcol], mcstrt[jcol] + hincol[jcol], hrow);

	      rcosts[jcol] -= (rdual_adjust * colels[kk]);
	    }

	    {
	      int k;
	      int badbasic = -1;

	      // we may have just screwed up the rcost of another basic variable
	      for (k=0; k<ninrow; k++) {
		int col = rowcols[k];
		if (col != jcol &&
		    cdone[col] &&
		    (colstat[col] & PRESOLVE_XBASIC) &&
		    !(fabs(rcosts[col]) < ztoldj))
		  if (badbasic == -1)
		    badbasic = k;
		  else
		    abort();	// two!!  what to do???
	      }

	      if (badbasic != -1) {
		int col = rowcols[badbasic];

		if (fabs(acts[irow]) < ZTOLDP) {
#if	PRESOLVE_DEBUG
		  printf("PIVOTING COL TO SLACK!:  %d %d\n", irow, col);
#endif
		  colstat[col] = 0;
		  rowstat[irow] = PRESOLVE_XBASIC;
		}
		else
		  abort();
	      }
	    }
	  }
	}
#endif		// #if 0	// (B)
#endif		// #if 0	// (A)

forcing_constraint_action::~forcing_constraint_action()
{
  int i;
  for (i=0;i<nactions_;i++) {
    //delete [] actions_[i].rowcols; MS Visual C++ V6 can not compile
    //delete [] actions_[i].bounds; MS Visual C++ V6 can not compile
    deleteAction(actions_[i].rowcols,int *);
    deleteAction(actions_[i].bounds,double *);
  }
  // delete [] actions_; MS Visual C++ V6 can not compile
  deleteAction(actions_,action *);
}
