/* $Id: CoinPresolveDoubleton.cpp 1448 2011-06-19 15:34:41Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinFinite.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"

#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolveZeros.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveDoubleton.hpp"

#include "CoinPresolvePsdebug.hpp"
#include "CoinMessage.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif


namespace {	/* begin unnamed local namespace */

/*
   This routine does the grunt work needed to substitute x for y in all rows i
   where coeff[i,y] != 0. We have

  	 y = (c - a*x)/b = c/b + (-a/b)*x

   Suppose we're fixing row i. We need to adjust the row bounds by
   -coeff[i,y]*(c/b) and coeff[i,x] by coeff[i,y]*(-a/b). The value
   c/b is passed as the bounds_factor, and -a/b as the coeff_factor.

   row0 is the doubleton row.  It is assumed that coeff[row0,y] has been
   removed from the column major representation before this routine is
   called. (Otherwise, we'd have to check for it to avoid a useless row
   update.)

   Both the row and col representations are updated. There are two cases:

   * coeff[i,x] != 0:
	in the column rep, modify coeff[i,x];
	in the row rep, modify coeff[i,x] and drop coeff[i,y].

   * coeff[i,x] == 0 (i.e., non-existent):
        in the column rep, add coeff[i,x]; mcstrt is modified if the column
	must be moved;
	in the row rep, convert coeff[i,y] to coeff[i,x].

   The row and column reps are inconsistent during the routine and at
   completion.  In the row rep, column x and y are updated except for
   the doubleton row, and in the column rep only column x is updated
   except for coeff[row0,x]. On return, column y and row row0 will be deleted
   and consistency will be restored.
*/

  bool elim_doubleton (const char *
#ifdef PRESOLVE_DEBUG
msg
#endif
		       ,
		     CoinBigIndex *mcstrt,
		     double *rlo, double *rup,
		     double *colels,
		     int *hrow, int *hcol,
		     int *hinrow, int *hincol,
		     presolvehlink *clink, int ncols,
		     CoinBigIndex *mrstrt, double *rowels,
		     double coeff_factor,
		     double bounds_factor,
		       int
#ifdef PRESOLVE_DEBUG
		       row0
#endif
		       , int icolx, int icoly)

{
  CoinBigIndex kcsx = mcstrt[icolx];
  CoinBigIndex kcex = kcsx + hincol[icolx];

# if PRESOLVE_DEBUG
  printf("%s %d x=%d y=%d cf=%g bf=%g nx=%d yrows=(", msg,
	 row0, icolx, icoly, coeff_factor, bounds_factor, hincol[icolx]);
# endif
/*
  Open a loop to scan column y. For each nonzero coefficient (row,y),
  update column x and the row bounds for the row.

  The initial assert checks that we're properly updating column x.
*/
  CoinBigIndex base = mcstrt[icoly];
  int numberInY = hincol[icoly];
  for (int kwhere = 0 ; kwhere < numberInY ; kwhere++)
  { PRESOLVEASSERT(kcex == kcsx+hincol[icolx]) ;
  CoinBigIndex kcoly = base+kwhere;
/*
  Look for coeff[row,x], then update accordingly.
*/
    double coeffy = colels[kcoly] ;
    double delta = coeffy*coeff_factor ;
    int row = hrow[kcoly] ;
    CoinBigIndex kcolx = presolve_find_row1(row,kcsx,kcex,hrow) ;
#   if PRESOLVE_DEBUG
    printf("%d%s ",row,(kcolx<kcex)?"+":"") ;
#   endif
/*
  Case 1: coeff[i,x] != 0: update it in column and row reps; drop coeff[i,y]
  from row rep.
*/
    if (kcolx < kcex)
    { colels[kcolx] += delta ;

      CoinBigIndex kmi = presolve_find_col(icolx,mrstrt[row],
					   mrstrt[row]+hinrow[row],hcol) ;
      rowels[kmi] = colels[kcolx] ;
      presolve_delete_from_row(row,icoly,mrstrt,hinrow,hcol,rowels) ; }
/*
  Case 2: coeff[i,x] == 0: add it in the column rep; convert coeff[i,y] in
  the row rep. presolve_expand_col ensures an empty entry exists at the
  end of the column. The location of column x may change with expansion.
*/
    else
    { bool no_mem = presolve_expand_col(mcstrt,colels,hrow,hincol,
					clink,ncols,icolx) ;
	if (no_mem)
	  return (true) ;

      kcsx = mcstrt[icolx] ;
      kcex = mcstrt[icolx]+hincol[icolx] ;
      // recompute y as well
      base = mcstrt[icoly];

      hrow[kcex] = row ;
      colels[kcex] = delta ;
      hincol[icolx]++ ;
      kcex++ ;

      CoinBigIndex k2 = presolve_find_col(icoly,mrstrt[row],
					  mrstrt[row]+hinrow[row],hcol) ;
      hcol[k2] = icolx ;
      rowels[k2] = delta ; }
/*
  Update the row bounds, if necessary. Avoid updating finite infinity.
*/
    if (bounds_factor != 0.0)
    { delta = coeffy*bounds_factor ;
      if (-PRESOLVE_INF < rlo[row])
	rlo[row] -= delta ;
      if (rup[row] < PRESOLVE_INF)
	rup[row] -= delta ; } }

# if PRESOLVE_DEBUG
  printf(")\n") ;
# endif

  return (false) ; }

} /* end unnamed local namespace */


/*
 * It is always the case that one of the variables of a doubleton
 * will be (implied) free, but neither will necessarily be a singleton.
 * Since in the case of a doubleton the number of non-zero entries
 * will never increase, though, it makes sense to always eliminate them.
 *
 * The col rep and row rep must be consistent.
 */
const CoinPresolveAction
  *doubleton_action::presolve (CoinPresolveMatrix *prob,
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
  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int ncols		= prob->ncols_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
  int nrows		= prob->nrows_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  presolvehlink *clink = prob->clink_;
  presolvehlink *rlink = prob->rlink_;

  const unsigned char *integerType = prob->integerType_;

  double *cost	= prob->cost_;

  int numberLook = prob->numberRowsToDo_;
  int iLook;
  int * look = prob->rowsToDo_;
  const double ztolzb	= prob->ztolzb_;

  action * actions = new action [nrows];
  int nactions = 0;

  int *zeros	= prob->usefulColumnInt_; //new int[ncols];
  int nzeros	= 0;

  int *fixed	= zeros+ncols; //new int[ncols];
  int nfixed	= 0;

  unsigned char *rowstat = prob->rowstat_;
  double *acts	= prob->acts_;
  double * sol = prob->sol_;

  bool fixInfeasibility = (prob->presolveOptions_&16384)!=0;
# if PRESOLVE_CONSISTENCY
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
# endif

  // wasfor (int irow=0; irow<nrows; irow++)
  for (iLook=0;iLook<numberLook;iLook++) {
    int irow = look[iLook];
    if (hinrow[irow] == 2 &&
	fabs(rup[irow] - rlo[irow]) <= ZTOLDP) {
      double rhs = rlo[irow];
      CoinBigIndex krs = mrstrt[irow];
      int icolx, icoly;
      CoinBigIndex k;

      icolx = hcol[krs];
      icoly = hcol[krs+1];
      if (hincol[icolx]<=0||hincol[icoly]<=0) {
        // should never happen ?
        //printf("JJF - doubleton column %d has %d entries and %d has %d\n",
        //     icolx,hincol[icolx],icoly,hincol[icoly]);
        continue;
      }
      // check size
      if (fabs(rowels[krs]) < ZTOLDP2 || fabs(rowels[krs+1]) < ZTOLDP2)
	continue;
      // See if prohibited for any reason
      if (prob->colProhibited(icolx) || prob->colProhibited(icolx))
	continue;

      // don't bother with fixed variables
      if (!(fabs(cup[icolx] - clo[icolx]) < ZTOLDP) &&
	  !(fabs(cup[icoly] - clo[icoly]) < ZTOLDP)) {
	double coeffx, coeffy;
	/* find this row in each of the columns */
	CoinBigIndex krowx = presolve_find_row(irow, mcstrt[icolx], mcstrt[icolx] + hincol[icolx], hrow);
	CoinBigIndex krowy = presolve_find_row(irow, mcstrt[icoly], mcstrt[icoly] + hincol[icoly], hrow);

/*
  Check for integrality: If one variable is integer, keep it and substitute
  for the continuous variable. If both are integer, substitute only for the
  forms x = k * y (k integral and non-empty intersection on bounds on x)
  or x = 1-y, where both x and y are binary.

  flag bits for integerStatus: 1>>0	x integer
			       1>>1	y integer
*/
	int integerStatus=0;
	if (integerType[icolx]) {
	  if (integerType[icoly]) {
	    // both integer
	    int good = 0;
	    double rhs2 = rhs;
	    double value;
	    value=colels[krowx];
	    if (value<0.0) {
	      value = - value;
	      rhs2 += 1;
	    }
	    if (cup[icolx]==1.0&&clo[icolx]==0.0&&fabs(value-1.0)<1.0e-7)
	      good =1;
	    value=colels[krowy];
	    if (value<0.0) {
	      value = - value;
	      rhs2 += 1;
	    }
	    if (cup[icoly]==1.0&&clo[icoly]==0.0&&fabs(value-1.0)<1.0e-7)
	      good  |= 2;
	    if (good==3&&fabs(rhs2-1.0)<1.0e-7)
	      integerStatus = 3;
	    else
	      integerStatus=-1;
	    if (integerStatus==-1&&!rhs) {
	      // maybe x = k * y;
	      double value1 = colels[krowx];
	      double value2 = colels[krowy];
	      double ratio;
	      bool swap=false;
	      if (fabs(value1)>fabs(value2)) {
		ratio = value1/value2;
	      } else {
		ratio = value2/value1;
		swap=true;
	      }
	      ratio=fabs(ratio);
	      if (fabs(ratio-floor(ratio+0.5))<1.0e-12) {
		// possible
		integerStatus = swap ? 2 : 1;
		//printf("poss type %d\n",integerStatus);
	      }
	    }
	  } else {
	    integerStatus = 1;
	  }
	} else if (integerType[icoly]) {
	  integerStatus = 2;
	}
	if (integerStatus<0) {
	  // can still take in some cases
	  bool canDo=false;
	  double value1 = colels[krowx];
	  double value2 = colels[krowy];
	  double ratio;
	  bool swap=false;
	  double rhsRatio;
	  if (fabs(value1)>fabs(value2)) {
	    ratio = value1/value2;
	    rhsRatio = rhs/value1;
	  } else {
	    ratio = value2/value1;
	    rhsRatio = rhs/value2;
	    swap=true;
	  }
	  ratio=fabs(ratio);
	  if (fabs(ratio-floor(ratio+0.5))<1.0e-12) {
	    // possible
	    integerStatus = swap ? 2 : 1;
	    // but check rhs
	    if (rhsRatio==floor(rhsRatio+0.5))
	      canDo=true;
	  }
#ifdef COIN_DEVELOP2
	  if (canDo)
	    printf("Good CoinPresolveDoubleton icolx %d (%g and bounds %g %g) icoly %d (%g and bound %g %g) - rhs %g\n",
		   icolx,colels[krowx],clo[icolx],cup[icolx],
		   icoly,colels[krowy],clo[icoly],cup[icoly],rhs);
	  else
	  printf("Bad CoinPresolveDoubleton icolx %d (%g) icoly %d (%g) - rhs %g\n",
		 icolx,colels[krowx],icoly,colels[krowy],rhs);
#endif
	  if (!canDo)
	  continue;
	}
	if (integerStatus == 2) {
	  CoinSwap(icoly,icolx);
	  CoinSwap(krowy,krowx);
	}

	// HAVE TO JIB WITH ABOVE swapS
	// if x's coefficient is something like 1000, but y's only something like -1,
	// then when we postsolve, if x's is close to being out of tolerance,
	// then y is very likely to be (because y==1000x) . (55)
	// It it interesting that the number of doubletons found may depend
	// on which column is substituted away (this is true of baxter.mps).
	if (!integerStatus) {
	  if (fabs(colels[krowy]) < fabs(colels[krowx])) {
	    CoinSwap(icoly,icolx);
	    CoinSwap(krowy,krowx);
	  }
	}

#if 0
	//?????
	if (integerType[icolx] &&
	    clo[icoly] != -PRESOLVE_INF &&
	    cup[icoly] != PRESOLVE_INF) {
	  continue;
	}
#endif

	{
	  CoinBigIndex kcs = mcstrt[icoly];
	  CoinBigIndex kce = kcs + hincol[icoly];
	  for (k=kcs; k<kce; k++) {
	    if (hinrow[hrow[k]] == 1) {
	      break;
	    }
	  }
	  // let singleton rows be taken care of first
	  if (k<kce)
	    continue;
	}

	coeffx = colels[krowx];
	coeffy = colels[krowy];

	// it is possible that both x and y are singleton columns
	// that can cause problems
	if (hincol[icolx] == 1 && hincol[icoly] == 1)
	  continue;

	// BE CAUTIOUS and avoid very large relative differences
	// if this is not done in baxter, then the computed solution isn't optimal,
	// but gets it in 11995 iterations; the postsolve goes to iteration 16181.
	// with this, the solution is optimal, but takes 18825 iters; postsolve 18871.
#if 0
	if (fabs(coeffx) * max_coeff_factor <= fabs(coeffy))
	  continue;
#endif

#if 0
	if (only_zero_rhs && rhs != 0)
	  continue;

	if (reject_doubleton(mcstrt, colels, hrow, hincol,
			     -coeffx / coeffy,
			     max_coeff_ratio,
			     irow, icolx, icoly))
	  continue;
#endif

	// common equations are of the form ax + by = 0, or x + y >= lo
	{
	  PRESOLVE_DETAIL_PRINT(printf("pre_doubleton %dC %dC %dR E\n",
				       icoly,icolx,irow));
	  action *s = &actions[nactions];
	  nactions++;

	  s->row = irow;
	  s->icolx = icolx;

	  s->clox = clo[icolx];
	  s->cupx = cup[icolx];
	  s->costx = cost[icolx];

	  s->icoly = icoly;
	  s->costy = cost[icoly];

	  s->rlo = rlo[irow];

	  s->coeffx = coeffx;

	  s->coeffy = coeffy;

	  s->ncolx	= hincol[icolx];

	  s->ncoly	= hincol[icoly];
	  if (s->ncoly<s->ncolx) {
	    // Take out row
	    s->colel	= presolve_dupmajor(colels,hrow,hincol[icoly],
					    mcstrt[icoly],irow) ;
	    s->ncolx=0;
	  } else {
	    s->colel	= presolve_dupmajor(colels,hrow,hincol[icolx],
					    mcstrt[icolx],irow) ;
	    s->ncoly=0;
	  }
	}

	/*
	 * This moves the bounds information for y onto x,
	 * making y free and allowing us to substitute it away.
	 *
	 * a x + b y = c
	 * l1 <= x <= u1
	 * l2 <= y <= u2	==>
	 *
	 * l2 <= (c - a x) / b <= u2
	 * b/-a > 0 ==> (b l2 - c) / -a <= x <= (b u2 - c) / -a
	 * b/-a < 0 ==> (b u2 - c) / -a <= x <= (b l2 - c) / -a
	 */
	{
	  double lo1 = -PRESOLVE_INF;
	  double up1 = PRESOLVE_INF;

	  //PRESOLVEASSERT((coeffx < 0) == (coeffy/-coeffx < 0));
	  // (coeffy/-coeffx < 0) == (coeffy<0 == coeffx<0)
	  if (-PRESOLVE_INF < clo[icoly]) {
	    if (coeffx * coeffy < 0)
	      lo1 = (coeffy * clo[icoly] - rhs) / -coeffx;
	    else
	      up1 = (coeffy * clo[icoly] - rhs) / -coeffx;
	  }

	  if (cup[icoly] < PRESOLVE_INF) {
	    if (coeffx * coeffy < 0)
	      up1 = (coeffy * cup[icoly] - rhs) / -coeffx;
	    else
	      lo1 = (coeffy * cup[icoly] - rhs) / -coeffx;
	  }

	  // costy y = costy ((c - a x) / b) = (costy c)/b + x (costy -a)/b
	  // the effect of maxmin cancels out
	  cost[icolx] += cost[icoly] * (-coeffx / coeffy);

	  prob->change_bias(cost[icoly] * rhs / coeffy);

	  if (0    /*integerType[icolx]*/) {
	    abort();
	    /* no change possible for now */
#if 0
	    lo1 = trunc(lo1);
	    up1 = trunc(up1);

	    /* trunc(3.5) == 3.0 */
	    /* trunc(-3.5) == -3.0 */

	    /* I think this is ok */
	    if (lo1 > clo[icolx]) {
	      (clo[icolx] <= 0.0)
		clo[icolx] =  ? ilo

		clo[icolx] = ilo;
	      cup[icolx] = iup;
	    }
#endif
	  } else {
	    double lo2 = CoinMax(clo[icolx], lo1);
	    double up2 = CoinMin(cup[icolx], up1);
	    if (lo2 > up2) {
	      if (lo2 <= up2 + prob->feasibilityTolerance_||fixInfeasibility) {
		// If close to integer then go there
		double nearest = floor(lo2+0.5);
		if (fabs(nearest-lo2)<2.0*prob->feasibilityTolerance_) {
		  lo2 = nearest;
		  up2 = nearest;
		} else {
		  lo2 = up2;
		}
	      } else {
		prob->status_ |= 1;
		prob->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
							 prob->messages())
							   <<icolx
							   <<lo2
							   <<up2
							   <<CoinMessageEol;
		break;
	      }
	    }
	    clo[icolx] = lo2;
	    cup[icolx] = up2;

	    if (rowstat&&sol) {
	      // update solution and basis
              int basisChoice=0;
	      int numberBasic=0;
	      double movement = 0 ;
	      if (prob->columnIsBasic(icolx))
		numberBasic++;
	      if (prob->columnIsBasic(icoly))
		numberBasic++;
	      if (prob->rowIsBasic(irow))
		numberBasic++;
              if (sol[icolx]<=lo2+ztolzb) {
		movement = lo2-sol[icolx] ;
		sol[icolx] = lo2;
		prob->setColumnStatus(icolx,CoinPrePostsolveMatrix::atLowerBound);
	      } else if (sol[icolx]>=up2-ztolzb) {
		movement = up2-sol[icolx] ;
		sol[icolx] = up2;
		prob->setColumnStatus(icolx,CoinPrePostsolveMatrix::atUpperBound);
	      } else {
		basisChoice=1;
	      }
	      if (numberBasic>1)
		prob->setColumnStatus(icolx,CoinPrePostsolveMatrix::basic);
/*
  We need to compensate if x was forced to move. Beyond that, even if x didn't
  move, we've forced y = (c-ax)/b, and that might not have been true before. So
  even if x didn't move, y may have moved. Note that the constant term c/b is
  subtracted out as the constraints are modified, so we don't include it when
  calculating movement for y.
*/
	      if (movement)
	      { CoinBigIndex k;
		for (k = mcstrt[icolx] ; k < mcstrt[icolx]+hincol[icolx] ; k++)
		{ int row = hrow[k];
		  if (hinrow[row])
		    acts[row] += movement*colels[k]; } }
	      movement = (-coeffx*sol[icolx]/coeffy)-sol[icoly] ;
	      if (movement)
	      { for (k = mcstrt[icoly] ;
		     k < mcstrt[icoly]+hincol[icoly] ;
		     k++)
		{ int row = hrow[k];
		  if (hinrow[row])
		    acts[row] += movement*colels[k]; } }
	    }
	    if (lo2 == up2)
	      fixed[nfixed++] = icolx;
	  }
	}

	// Update next set of actions
	{
	  prob->addCol(icolx);
	  int i,kcs,kce;
	  kcs = mcstrt[icoly];
	  kce = kcs + hincol[icoly];
	  for (i=kcs;i<kce;i++) {
	    int row = hrow[i];
	    prob->addRow(row);
	  }
	  kcs = mcstrt[icolx];
	  kce = kcs + hincol[icolx];
	  for (i=kcs;i<kce;i++) {
	    int row = hrow[i];
	    prob->addRow(row);
	  }
	}

/*
  Empty irow in the column-major matrix.  Deleting the coefficient for
  (irow,icoly) is a bit costly (given that we're about to drop the whole
  column), but saves the trouble of checking for it in elim_doubleton.
*/
	presolve_delete_from_col(irow,icolx,mcstrt,hincol,hrow,colels) ;
	presolve_delete_from_col(irow,icoly,mcstrt,hincol,hrow,colels) ;
/*
  Drop irow in the row-major representation: set the length to 0
  and reclaim the major vector space in bulk storage.
*/
	hinrow[irow] = 0;
	PRESOLVE_REMOVE_LINK(rlink,irow);

	/* transfer the colx factors to coly */
	bool no_mem = elim_doubleton("ELIMD",
				     mcstrt, rlo, rup, colels,
				     hrow, hcol, hinrow, hincol,
				     clink, ncols,
				     mrstrt, rowels,
				     -coeffx / coeffy,
				     rhs / coeffy,
				     irow, icolx, icoly);
	if (no_mem)
	  throwCoinError("out of memory",
			 "doubleton_action::presolve");


	// eliminate coly entirely from the col rep
	hincol[icoly] = 0;
	PRESOLVE_REMOVE_LINK(clink, icoly);
	cost[icoly] = 0.0;

	rlo[irow] = 0.0;
	rup[irow] = 0.0;

	zeros[nzeros++] = icolx;	// check for zeros

	// strictly speaking, since we didn't adjust {clo,cup}[icoly]
	// or {rlo,rup}[irow], this col/row may be infeasible,
	// because the solution/activity would be 0, whereas the
	// bounds may be non-zero.
      }

#     if PRESOLVE_CONSISTENCY
      presolve_consistent(prob) ;
      presolve_links_ok(prob) ;
#     endif
    }
  }

  if (nactions) {
#   if PRESOLVE_SUMMARY
    printf("NDOUBLETONS:  %d\n", nactions);
#   endif
    action *actions1 = new action[nactions];
    CoinMemcpyN(actions, nactions, actions1);

    next = new doubleton_action(nactions, actions1, next);

    if (nzeros)
      next = drop_zero_coefficients_action::presolve(prob, zeros, nzeros, next);
    if (nfixed)
      next = remove_fixed_action::presolve(prob, fixed, nfixed, next);
  }

  //delete[]zeros;
  //delete[]fixed;
  deleteAction(actions,action*);

  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    int droppedRows = prob->countEmptyRows() - startEmptyRows ;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
    printf("CoinPresolveDoubleton(4) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_);
  }
  return (next);
}

/*
  Reintroduce the column (y) and doubleton row (irow) removed in presolve.
  Correct the other column (x) involved in the doubleton, update the solution,
  etc.

  A fair amount of complication arises because the presolve transform saves the
  shorter of x or y. Postsolve thus includes portions to restore either.
*/
void doubleton_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *dcost	= prob->cost_;

  double *sol	= prob->sol_;
  double *acts	= prob->acts_;
  double *rowduals = prob->rowduals_;
  double *rcosts = prob->rcosts_;

  unsigned char *colstat = prob->colstat_;
  unsigned char *rowstat = prob->rowstat_;

  const double maxmin	= prob->maxmin_;

# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
  char *cdone	= prob->cdone_;
  char *rdone	= prob->rdone_;
# endif

  CoinBigIndex &free_list = prob->free_list_;

  const double ztolzb	= prob->ztolzb_;
  const double ztoldj	= prob->ztoldj_;

  int nrows = prob->nrows_;
  // Arrays to rebuild the unsaved column.
  int * index1 = new int[nrows];
  double * element1 = new double[nrows];
  CoinZeroN(element1,nrows);

# if PRESOLVE_CONSISTENCY
  presolve_check_threads(prob) ;
# endif
# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
# endif
/*
  The outer loop: step through the doubletons in this array of actions.
  The first activity is to unpack the doubleton.
*/
  for (const action *f = &actions[nactions-1]; actions<=f; f--) {

    int irow = f->row;
    double lo0 = f->clox;
    double up0 = f->cupx;


    double coeffx = f->coeffx;
    double coeffy = f->coeffy;
    int jcolx = f->icolx;
    int jcoly = f->icoly;

    double rhs = f->rlo;
/*
  jcolx is in the problem (for whatever reason), and the doubleton row (irow)
  and column (jcoly) have only been processed by empty row/column postsolve
  (i.e., reintroduced with length 0).
*/
    PRESOLVEASSERT(cdone[jcolx] && rdone[irow]==DROP_ROW);
    PRESOLVEASSERT(cdone[jcoly]==DROP_COL);

/*
  Restore bounds for doubleton row, bounds and objective coefficient for x,
  objective for y.

  Original comment: restoration of rlo and rup likely isn't necessary.
*/
    rlo[irow] = f->rlo;
    rup[irow] = f->rlo;

    clo[jcolx] = lo0;
    cup[jcolx] = up0;

    dcost[jcolx] = f->costx;
    dcost[jcoly] = f->costy;

#if	PRESOLVE_DEBUG
/* Original comment: I've forgotten what this is about

   Loss of significant digits through cancellation, with possible inflation
   when divided by coeffy below? -- lh, 040831 --
*/
    if ((rhs < 0) == ((coeffx * sol[jcolx]) < 0) &&
	fabs(rhs - coeffx * sol[jcolx]) * 100 < rhs &&
	fabs(rhs - coeffx * sol[jcolx]) * 100 < (coeffx * sol[jcolx]))
      printf("DANGEROUS RHS??? %g %g %g\n",
	     rhs, coeffx * sol[jcolx],
	     (rhs - coeffx * sol[jcolx]));
#endif
/*
  Set primal solution for y (including status) and row activity for the
  doubleton row. The motivation (up in presolve) for wanting coeffx < coeffy
  is to avoid inflation into sol[y]. Since this is a (satisfied) equality,
  activity is the rhs value and the logical is nonbasic.
*/
    sol[jcoly] = (rhs-coeffx*sol[jcolx])/coeffy;
    acts[irow] = rhs;
    if (rowstat)
      prob->setRowStatus(irow,CoinPrePostsolveMatrix::atLowerBound);
/*
  Time to get into the correction/restoration of coefficients for columns x
  and y, with attendant correction of row bounds and activities. Accumulate
  partial reduced costs (missing the contribution from the doubleton row) so
  that we can eventually calculate a dual for the doubleton row.
*/
    double djy = maxmin * dcost[jcoly];
    double djx = maxmin * dcost[jcolx];
    double bounds_factor = rhs/coeffy;
/*
  We saved column y in the action, so we'll use it to reconstruct column x.
  There are two aspects: correction of existing x coefficients, and fill in.
  Given
    coeffx'[k] = coeffx[k]+coeffy[k]*coeff_factor
  we have
    coeffx[k] = coeffx'[k]-coeffy[k]*coeff_factor
  where
    coeff_factor = -coeffx[dblton]/coeffy[dblton].

  Keep in mind that the major vector stored in the action does not include
  the coefficient from the doubleton row --- the doubleton coefficients are
  held in coeffx and coeffy.
*/
    if (f->ncoly) {
      int ncoly=f->ncoly-1; // as row taken out
      double multiplier = coeffx/coeffy;
      //printf("Current colx %d\n",jcolx);
      int * indy = reinterpret_cast<int *>(f->colel+ncoly);
/*
  Rebuild a threaded column y, starting with the end of the thread and working
  back to the beginning. In the process, accumulate corrections to column x
  in element1 and index1. Fix row bounds and activity as we go (add back the
  constant correction removed in presolve), and accumulate contributions to
  the reduced cost for y.

  The PRESOLVEASSERT says this row should already be present.
*/
      int ystart = NO_LINK;
      int nX=0;
      int i,iRow;
      for (i=0; i<ncoly; ++i) {
	int iRow = indy[i];
	PRESOLVEASSERT(rdone[iRow]);

	double yValue = f->colel[i];

	// undo elim_doubleton(1)
	if (-PRESOLVE_INF < rlo[iRow])
	  rlo[iRow] += yValue * bounds_factor;

	// undo elim_doubleton(2)
	if (rup[iRow] < PRESOLVE_INF)
	  rup[iRow] += yValue * bounds_factor;

	acts[iRow] += yValue * bounds_factor;

	djy -= rowduals[iRow] * yValue;
/*
  Link the coefficient into column y: Acquire the first free slot in the
  bulk arrays and store the row index and coefficient. Then link the slot
  in front of coefficients we've already processed.
*/
	CoinBigIndex k = free_list;
	assert(k >= 0 && k < prob->bulk0_) ;
	free_list = link[free_list];
	hrow[k] = iRow;
	colels[k] = yValue;
	link[k] = ystart;
	ystart = k;
/*
  Calculate and store the correction to the x coefficient.
*/
	yValue *= multiplier;
	element1[iRow]=yValue;
	index1[nX++]=iRow;
      }
#     if PRESOLVE_CONSISTENCY
      presolve_check_free_list(prob) ;
#     endif
/*
  Handle the coefficients of the doubleton row.
*/
      {
	double yValue = coeffy;

	CoinBigIndex k = free_list;
	assert(k >= 0 && k < prob->bulk0_) ;
	free_list = link[free_list];
	hrow[k] = irow;
	colels[k] = yValue;
	link[k] = ystart;
	ystart = k;

	yValue *= multiplier;
	element1[irow]=yValue;
	index1[nX++]=irow;
      }
/*
  Attach the threaded column y to mcstrt and record the length.
*/
      mcstrt[jcoly] = ystart;
      hincol[jcoly] = f->ncoly;
/*
  Now integrate the corrections to column x. First thing to do is find the
  end of the column. While we're doing that, correct any existing entries.
  This complicates life because the correction could cancel the existing
  coefficient and we don't want to leave an explicit zero. In this case we
  relink the column around it. (last is a little misleading --- it's actually
  `last nonzero'. If we haven't seen a nonzero yet, the relink goes to mcstrt.)
  The freed slot is linked at the beginning of the free list.
*/
      CoinBigIndex k=mcstrt[jcolx];
      CoinBigIndex last = NO_LINK;
      int numberInColumn = hincol[jcolx];
      int numberToDo=numberInColumn;
      for (i=0; i<numberToDo; ++i) {
	iRow = hrow[k];
	assert (iRow>=0&&iRow<nrows);
	double value = colels[k]+element1[iRow];
	element1[iRow]=0.0;
	if (fabs(value)>=1.0e-15) {
	  colels[k]=value;
	  last=k;
	  k = link[k];
	  if (iRow != irow)
	    djx -= rowduals[iRow] * value;
	} else {
	  numberInColumn--;
	  // add to free list
	  int nextk = link[k];
	  assert(free_list>=0);
	  link[k]=free_list;
	  free_list=k;
	  assert (k>=0);
	  k=nextk;
	  if (last!=NO_LINK)
	    link[last]=k;
	  else
	    mcstrt[jcolx]=k;
	}
      }
/*
  We've found the end of column x. Any remaining nonzeros in element1 will be
  fill in, which we link at the end of the column thread.
*/
      for (i=0;i<nX;i++) {
	int iRow = index1[i];
	double xValue = element1[iRow];
	element1[iRow]=0.0;
	if (fabs(xValue)>=1.0e-15) {
	  if (iRow != irow)
	    djx -= rowduals[iRow] * xValue;
	  numberInColumn++;
	  CoinBigIndex k = free_list;
	  assert(k >= 0 && k < prob->bulk0_) ;
	  free_list = link[free_list];
	  hrow[k] = iRow;
	  PRESOLVEASSERT(rdone[hrow[k]] || hrow[k] == irow);
	  colels[k] = xValue;
	  if (last!=NO_LINK)
            link[last] = k;
          else
            mcstrt[jcolx]=k;
	  last = k;
	}
      }

#     if PRESOLVE_CONSISTENCY
      presolve_check_free_list(prob) ;
#     endif

/*
  Whew! Tidy up column x and we're done.
*/
      link[last]=NO_LINK;
      assert(numberInColumn);
      hincol[jcolx] = numberInColumn;
/*
  Of course, we could have saved column x in the action. Now we need to
  regenerate coefficients of column y.
  Given
    coeffx'[k] = coeffx[k]+coeffy[k]*coeff_factor
  we have
    coeffy[k] = (coeffx'[k]-coeffx[k])*(1/coeff_factor)
  where
    coeff_factor = -coeffx[dblton]/coeffy[dblton].
*/
    } else {
      int ncolx=f->ncolx-1;
      double multiplier = -coeffy/coeffx;
      int * indx = reinterpret_cast<int *> (f->colel+ncolx);
      //printf("Current colx %d\n",jcolx);
/*
  Scan existing column x to find the end. While we're at it, accumulate part
  of the new y coefficients in index1 and element1.
*/
      CoinBigIndex k=mcstrt[jcolx];
      int nX=0;
      int i,iRow;
      for (i=0; i<hincol[jcolx]-1; ++i) {
	if (colels[k]) {
	  iRow = hrow[k];
	  index1[nX++]=iRow;
	  element1[iRow]=multiplier*colels[k];
	}
	k = link[k];
      }
      if (colels[k]) {
        iRow = hrow[k];
        index1[nX++]=iRow;
        element1[iRow]=multiplier*colels[k];
      }
/*
  Replace column x with the the original column x held in the doubleton
  action. We first move column x to the free list, then thread a column with
  the original coefficients, back to front.  While we're at it, add the
  second part of the y coefficients to index1 and element1.
*/
      multiplier = - multiplier;
      link[k] = free_list;
      free_list = mcstrt[jcolx];
      int xstart = NO_LINK;
      for (i=0; i<ncolx; ++i) {
	int iRow = indx[i];
	PRESOLVEASSERT(rdone[iRow]);

	double xValue = f->colel[i];
	//printf("x %d %d %g\n",i,indx[i],f->colel[i]);
	CoinBigIndex k = free_list;
	assert(k >= 0 && k < prob->bulk0_) ;
	free_list = link[free_list];
	hrow[k] = iRow;
	colels[k] = xValue;
	link[k] = xstart;
	xstart = k;

	djx -= rowduals[iRow] * xValue;

	xValue *= multiplier;
	if (!element1[iRow]) {
	  element1[iRow]=xValue;
	  index1[nX++]=iRow;
	} else {
	  element1[iRow]+=xValue;
	}
      }
#     if PRESOLVE_CONSISTENCY
      presolve_check_free_list(prob) ;
#     endif
/*
  The same, for the doubleton row.
*/
      {
	double xValue = coeffx;
	CoinBigIndex k = free_list;
	assert(k >= 0 && k < prob->bulk0_) ;
	free_list = link[free_list];
	hrow[k] = irow;
	colels[k] = xValue;
	link[k] = xstart;
	xstart = k;

	xValue *= multiplier;
	if (!element1[irow]) {
	  element1[irow]=xValue;
	  index1[nX++]=irow;
	} else {
	  element1[irow]+=xValue;
	}
      }
/*
  Link the new column x to mcstrt and set the length.
*/
      mcstrt[jcolx] = xstart;
      hincol[jcolx] = f->ncolx;
/*
  Now get to work building a threaded column y from the nonzeros in element1.
  As before, build the thread in reverse.
*/
      int ystart = NO_LINK;
      int n=0;
      for (i=0;i<nX;i++) {
	int iRow = index1[i];
	PRESOLVEASSERT(rdone[iRow] || iRow == irow);
	double yValue = element1[iRow];
	element1[iRow]=0.0;
	if (fabs(yValue)>=1.0e-12) {
	  n++;
	  CoinBigIndex k = free_list;
	  assert(k >= 0 && k < prob->bulk0_) ;
	  free_list = link[free_list];
	  hrow[k] = iRow;
	  colels[k] = yValue;
	  link[k] = ystart;
	  ystart = k;
	}
      }
#     if PRESOLVE_CONSISTENCY
      presolve_check_free_list(prob) ;
#     endif
/*
  Tidy up --- link the new column into mcstrt and set the length.
*/
      mcstrt[jcoly] = ystart;
      assert(n);
      hincol[jcoly] = n;
/*
  Now that we have the original y, we can scan it and do the corrections to
  the row bounds and activity, and get a start on a reduced cost for y.
*/
      k = mcstrt[jcoly];
      int ny = hincol[jcoly];
      for (i=0; i<ny; ++i) {
	int row = hrow[k];
	double coeff = colels[k];
	k = link[k];

	if (row != irow) {

	  // undo elim_doubleton(1)
	  if (-PRESOLVE_INF < rlo[row])
	    rlo[row] += coeff * bounds_factor;

	  // undo elim_doubleton(2)
	  if (rup[row] < PRESOLVE_INF)
	    rup[row] += coeff * bounds_factor;

	  acts[row] += coeff * bounds_factor;

	  djy -= rowduals[row] * coeff;
	}
      }
/*
  Scan the new column x and calculate reduced costs. This could be integrated
  into the previous section where the original column x is restored.

  ok --- let's try it, then.

      k = mcstrt[jcolx];
      int nx = hincol[jcolx];

      for ( i=0; i<nx; ++i) {
	int row = hrow[k];
	double coeff = colels[k];
	k = link[k];

	if (row != irow) {
	  djx -= rowduals[row] * coeff;
	}
      }
*/
    }
/*
  Sanity? The only assignment to coeffx is f->coeffx! Ditto for coeffy.
*/
    assert (fabs(coeffx-f->coeffx)<1.0e-6&&fabs(coeffy-f->coeffy)<1.0e-6);
/*
  Time to calculate a dual for the doubleton row, and settle the status of x
  and y. Ideally, we'll leave x at whatever nonbasic status it currently has
  and make y basic. There's a potential problem, however: Remember that we
  transferred bounds from y to x when we eliminated y. If those bounds were
  tighter than x's original bounds, we may not be able to maintain x at its
  present status, or even as nonbasic.

  We'll make two claims here:

    * If the dual value for the doubleton row is chosen to keep the reduced
      cost djx of col x at its prior value, then the reduced cost djy of col
      y will be 0. (Crank through the linear algebra to convince yourself.)

    * If the bounds on x have loosened, then it must be possible to make y
      nonbasic, because we've transferred the tight bound back to y. (Yeah,
      I'm waving my hands. But it sounds good.  -- lh, 040907 --)

  So ... if we can maintain x nonbasic, then we need to set y basic, which
  means we should calculate rowduals[dblton] so that rcost[jcoly] == 0. We
  may need to change the status of x (an artifact of loosening a bound when
  x was previously a fixed variable).

  If we need to push x into the basis, then we calculate rowduals[dblton] so
  that rcost[jcolx] == 0 and make y nonbasic.
*/
    // printf("djs x - %g (%g), y - %g (%g)\n",djx,coeffx,djy,coeffy);
    if (colstat)
    { bool basicx = prob->columnIsBasic(jcolx) ;
      bool nblbxok = (fabs(lo0 - sol[jcolx]) < ztolzb) &&
		     (rcosts[jcolx] >= -ztoldj) ;
      bool nbubxok = (fabs(up0 - sol[jcolx]) < ztolzb) &&
		     (rcosts[jcolx] <= ztoldj) ;
      if (basicx || nblbxok || nbubxok)
      { if (!basicx)
	{ if (nblbxok)
	  { prob->setColumnStatus(jcolx,
				  CoinPrePostsolveMatrix::atLowerBound) ; }
	  else
	  if (nbubxok)
	  { prob->setColumnStatus(jcolx,
				  CoinPrePostsolveMatrix::atUpperBound) ; } }
	prob->setColumnStatus(jcoly,CoinPrePostsolveMatrix::basic);
	rowduals[irow] = djy / coeffy;
	rcosts[jcolx] = djx - rowduals[irow] * coeffx;
#if 0
	if (prob->columnIsBasic(jcolx))
	  assert (fabs(rcosts[jcolx])<1.0e-5);
#endif
	rcosts[jcoly] = 0.0;
      } else {
	prob->setColumnStatus(jcolx,CoinPrePostsolveMatrix::basic);
	prob->setColumnStatusUsingValue(jcoly);
	rowduals[irow] = djx / coeffx;
	rcosts[jcoly] = djy - rowduals[irow] * coeffy;
	rcosts[jcolx] = 0.0;
      }
    } else {
      // No status array
      // this is the coefficient we need to force col y's reduced cost to 0.0;
      // for example, this is obviously true if y is a singleton column
      rowduals[irow] = djy / coeffy;
      rcosts[jcoly] = 0.0;
    }

# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
/*
  Mark the column and row as processed by doubleton action. The check integrity
  of the threaded matrix.
*/
    cdone[jcoly] = DOUBLETON;
    rdone[irow] = DOUBLETON;
    presolve_check_threads(prob) ;
#endif
# if PRESOLVE_DEBUG
/*
  Confirm accuracy of reduced cost for columns x and y.
*/
    {
      CoinBigIndex k = mcstrt[jcolx];
      int nx = hincol[jcolx];
      double dj = maxmin * dcost[jcolx];

      for (int i=0; i<nx; ++i) {
	int row = hrow[k];
	double coeff = colels[k];
	k = link[k];

	dj -= rowduals[row] * coeff;
      }
      if (! (fabs(rcosts[jcolx] - dj) < 100*ZTOLDP))
	printf("BAD DOUBLE X DJ:  %d %d %g %g\n",
	       irow, jcolx, rcosts[jcolx], dj);
      rcosts[jcolx]=dj;
    }
    {
      CoinBigIndex k = mcstrt[jcoly];
      int ny = hincol[jcoly];
      double dj = maxmin * dcost[jcoly];

      for (int i=0; i<ny; ++i) {
	int row = hrow[k];
	double coeff = colels[k];
	k = link[k];

	dj -= rowduals[row] * coeff;
	//printf("b %d coeff %g dual %g dj %g\n",
	// row,coeff,rowduals[row],dj);
      }
      if (! (fabs(rcosts[jcoly] - dj) < 100*ZTOLDP))
	printf("BAD DOUBLE Y DJ:  %d %d %g %g\n",
	       irow, jcoly, rcosts[jcoly], dj);
      rcosts[jcoly]=dj;
    }
# endif
  }
/*
  Done at last. Delete the scratch arrays.
*/

  delete [] index1;
  delete [] element1;
}


doubleton_action::~doubleton_action()
{
  for (int i=nactions_-1; i>=0; i--) {
    delete[]actions_[i].colel;
  }
  deleteAction(actions_,action*);
}



static double *doubleton_mult;
static int *doubleton_id;
void check_doubletons(const CoinPresolveAction * paction)
{
  const CoinPresolveAction * paction0 = paction;

  if (paction) {
    check_doubletons(paction->next);

    if (strcmp(paction0->name(), "doubleton_action") == 0) {
      const doubleton_action *daction =
	reinterpret_cast<const doubleton_action *>(paction0);
      for (int i=daction->nactions_-1; i>=0; --i) {
	int icolx = daction->actions_[i].icolx;
	int icoly = daction->actions_[i].icoly;
	double coeffx = daction->actions_[i].coeffx;
	double coeffy = daction->actions_[i].coeffy;

	doubleton_mult[icoly] = -coeffx/coeffy;
	doubleton_id[icoly] = icolx;
      }
    }
  }
}

#if	PRESOLVE_DEBUG
void check_doubletons1(const CoinPresolveAction * paction,
		       int ncols)
#else
void check_doubletons1(const CoinPresolveAction * /*paction*/,
		       int /*ncols*/)
#endif
{
#if	PRESOLVE_DEBUG
  doubleton_mult = new double[ncols];
  doubleton_id = new int[ncols];
  int i;
  for ( i=0; i<ncols; ++i)
    doubleton_id[i] = i;
  check_doubletons(paction);
  double minmult = 1.0;
  int minid = -1;
  for ( i=0; i<ncols; ++i) {
    double mult = 1.0;
    int j = i;
    if (doubleton_id[j] != j) {
      printf("MULTS (%d):  ", j);
      while (doubleton_id[j] != j) {
	printf("%d %g, ", doubleton_id[j], doubleton_mult[j]);
	mult *= doubleton_mult[j];
	j = doubleton_id[j];
      }
      printf(" == %g\n", mult);
      if (minmult > fabs(mult)) {
	minmult = fabs(mult);
	minid = i;
      }
    }
  }
  if (minid != -1)
    printf("MIN MULT:  %d %g\n", minid, minmult);
#endif
}
