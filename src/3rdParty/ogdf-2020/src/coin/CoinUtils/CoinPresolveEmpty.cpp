/* $Id: CoinPresolveEmpty.cpp 1448 2011-06-19 15:34:41Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinFinite.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"

#include "CoinPresolveEmpty.hpp"
#include "CoinMessage.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif


/* \file

  Routines to remove/reinsert empty columns and rows.
*/


/*
  Physically remove empty columns, compressing mcstrt and hincol. The major
  side effect is that columns are renumbered, thus clink_ is no longer valid
  and must be rebuilt.

  It's necessary to rebuild clink_ in order to do direct conversion of a
  CoinPresolveMatrix to a CoinPostsolveMatrix by transferring the data arrays.
  Without clink_, it's impractical to build link_ to match the transferred bulk
  storage.
*/
const CoinPresolveAction
  *drop_empty_cols_action::presolve (CoinPresolveMatrix *prob,
				     int *ecols,
				     int necols,
				     const CoinPresolveAction *next)
{
  int ncols		= prob->ncols_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol		= prob->hincol_;

  presolvehlink *clink	= prob->clink_;

  double *clo		= prob->clo_;
  double *cup		= prob->cup_;
  double *dcost		= prob->cost_;

  const double ztoldj	= prob->ztoldj_;

  unsigned char * integerType     = prob->integerType_;
  int * originalColumn  = prob->originalColumn_;

  const double maxmin	= prob->maxmin_;

  double * sol = prob->sol_;
  unsigned char * colstat = prob->colstat_;

  action *actions 	= new action[necols];
  int * colmapping = new int [ncols+1];
  bool fixInfeasibility = (prob->presolveOptions_&16384)!=0;

  CoinZeroN(colmapping,ncols);
  int i;
  for (i=necols-1; i>=0; i--) {
    int jcol = ecols[i];
    colmapping[jcol]=-1;
    action &e = actions[i];

    e.jcol	= jcol;
    e.clo	= clo[jcol];
    e.cup	= cup[jcol];
    // adjust if integer
    if (integerType[jcol]) {
      e.clo = ceil(e.clo-1.0e-9);
      e.cup = floor(e.cup+1.0e-9);
      if (e.clo>e.cup&&!fixInfeasibility) {
        // infeasible
	prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
					     prob->messages())
				 	       <<jcol
					       <<e.clo
					       <<e.cup
					       <<CoinMessageEol;
      }
    }
    e.cost	= dcost[jcol];

    // there are no more constraints on this variable,
    // so we had better be able to compute the answer now
    if (fabs(dcost[jcol])<ztoldj)
      dcost[jcol]=0.0;
    if (dcost[jcol] * maxmin == 0.0) {
      // hopefully, we can make this non-basic
      // what does OSL currently do in this case???
      e.sol = (-PRESOLVE_INF < e.clo
	       ? e.clo
	       : e.cup < PRESOLVE_INF
	       ? e.cup
	       : 0.0);
    } else if (dcost[jcol] * maxmin > 0.0) {
      if (-PRESOLVE_INF < e.clo)
	e.sol = e.clo;
      else {
	  prob->messageHandler()->message(COIN_PRESOLVE_COLUMNBOUNDB,
					     prob->messages())
					       <<jcol
					       <<CoinMessageEol;
	prob->status_ |= 2;
	break;
      }
    } else {
      if (e.cup < PRESOLVE_INF)
	e.sol = e.cup;
      else {
	  prob->messageHandler()->message(COIN_PRESOLVE_COLUMNBOUNDA,
					     prob->messages())
					       <<jcol
					       <<CoinMessageEol;
	prob->status_ |= 2;
	break;
      }
    }

#if	PRESOLVE_DEBUG
    if (e.sol * dcost[jcol]) {
      //printf("\a>>>NON-ZERO COST FOR EMPTY COL %d:  %g\n", jcol, dcost[jcol]);
    }
#endif
    prob->change_bias(e.sol * dcost[jcol]);


  }
  int ncols2=0;

  // now move remaining ones down
  for (i=0;i<ncols;i++) {
    if (!colmapping[i]) {
      mcstrt[ncols2] = mcstrt[i];
      hincol[ncols2] = hincol[i];

      clo[ncols2]   = clo[i];
      cup[ncols2]   = cup[i];

      dcost[ncols2] = dcost[i];
      if (sol) {
	sol[ncols2] = sol[i];
	colstat[ncols2] = colstat[i];
      }

      integerType[ncols2] = integerType[i];
      originalColumn[ncols2] = originalColumn[i];
      colmapping[i] = ncols2++;
    }
  }
  mcstrt[ncols2] = mcstrt[ncols] ;
  colmapping[ncols] = ncols2 ;
/*
  Rebuild clink_. At this point, all empty columns are linked out, so the
  only columns left are columns that are to be saved, hence available in
  colmapping.  All we need to do is walk clink_ and write the new entries
  into a new array.
*/

  { presolvehlink *newclink = new presolvehlink [ncols2+1] ;
    for (int oldj = ncols ; oldj >= 0 ; oldj = clink[oldj].pre)
    { presolvehlink &oldlnk = clink[oldj] ;
      int newj = colmapping[oldj] ;
      assert(newj >= 0 && newj <= ncols2) ;
      presolvehlink &newlnk = newclink[newj] ;
      if (oldlnk.suc >= 0)
      { newlnk.suc = colmapping[oldlnk.suc] ; }
      else
      { newlnk.suc = NO_LINK ; }
      if (oldlnk.pre >= 0)
      { newlnk.pre = colmapping[oldlnk.pre] ; }
      else
      { newlnk.pre = NO_LINK ; } }
    delete [] clink ;
    prob->clink_ = newclink ; }

  delete [] colmapping;
  prob->ncols_ = ncols2;

  return (new drop_empty_cols_action(necols, actions, next));
}


const CoinPresolveAction *drop_empty_cols_action::presolve(CoinPresolveMatrix *prob,
							  const CoinPresolveAction *next)
{
  const int *hincol	= prob->hincol_;
  //  const double *clo	= prob->clo_;
  //  const double *cup	= prob->cup_;
  int ncols		= prob->ncols_;
  int i;
  int nempty		= 0;
  int * empty = new int [ncols];
  CoinBigIndex nelems2 = 0 ;

  // count empty cols
  for (i=0; i<ncols; i++)
  { nelems2 += hincol[i] ;
    if (hincol[i] == 0) {
#if	PRESOLVE_DEBUG
      if (nempty==0)
	printf("UNUSED COLS:  ");
      else
      if (i < 100 && nempty%25 == 0)
	printf("\n") ;
      else
      if (i >= 100 && i < 1000 && nempty%19 == 0)
	printf("\n") ;
      else
      if (i >= 1000 && nempty%15 == 0)
	printf("\n") ;
      printf("%d ", i);
#endif
      empty[nempty++] = i;
    }
  }
  prob->nelems_ = nelems2 ;

  if (nempty) {
#if	PRESOLVE_DEBUG
      printf("\ndropped %d cols\n", nempty);
#endif
    next = drop_empty_cols_action::presolve(prob, empty, nempty, next);
  }
  delete [] empty;
  return (next);
}


void drop_empty_cols_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const int nactions	= nactions_;
  const action *const actions = actions_;

  int ncols		= prob->ncols_;

  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;
  //  int *hrow	= prob->hrow_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  double *sol	= prob->sol_;
  double *cost	= prob->cost_;
  double *rcosts	= prob->rcosts_;
  unsigned char *colstat	= prob->colstat_;
  const double maxmin = prob->maxmin_;

  int ncols2 = ncols+nactions;
  int * colmapping = new int [ncols2];

  CoinZeroN(colmapping,ncols2);
# if PRESOLVE_DEBUG
  char *cdone	= prob->cdone_;
# endif
  int action_i;
  for (action_i = 0; action_i < nactions; action_i++) {
    const action *e = &actions[action_i];
    int jcol = e->jcol;
    colmapping[jcol]=-1;
  }

  int i;

  // now move remaining ones up
  for (i=ncols2-1;i>=0;i--) {
    if (!colmapping[i]) {
      ncols--;
      mcstrt[i] = mcstrt[ncols];
      hincol[i] = hincol[ncols];

      clo[i]   = clo[ncols];
      cup[i]   = cup[ncols];

      cost[i] = cost[ncols];

      if (sol)
	sol[i] = sol[ncols];

      if (rcosts)
        rcosts[i] = rcosts[ncols];

      if (colstat)
	colstat[i] = colstat[ncols];
#     if PRESOLVE_DEBUG
      cdone[i] = cdone[ncols];
#     endif
    }
  }
  assert (!ncols);

  delete [] colmapping;

  for (action_i = 0; action_i < nactions; action_i++) {
    const action *e = &actions[action_i];
    int jcol = e->jcol;

    // now recreate jcol
    clo[jcol] = e->clo;
    cup[jcol] = e->cup;
    if (sol)
      sol[jcol] = e->sol;
    cost[jcol] = e->cost;

    if (rcosts)
      rcosts[jcol] = maxmin*cost[jcol];

    hincol[jcol] = 0;
    mcstrt[jcol] = NO_LINK ;
# if PRESOLVE_DEBUG
    cdone[jcol] = DROP_COL;
# endif
    if (colstat)
      prob->setColumnStatusUsingValue(jcol);
  }

  prob->ncols_ += nactions;

# if PRESOLVE_CONSISTENCY
  presolve_check_threads(prob) ;
# endif

}



const CoinPresolveAction *drop_empty_rows_action::presolve(CoinPresolveMatrix *prob,
				       const CoinPresolveAction *next)
{
  int ncols	= prob->ncols_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;
  int *hrow	= prob->hrow_;

  int nrows	= prob->nrows_;
  // This is done after row copy needed
  //int *mrstrt	= prob->mrstrt_;
  int *hinrow	= prob->hinrow_;
  //int *hcol	= prob->hcol_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  unsigned char *rowstat	= prob->rowstat_;
  double *acts	= prob->acts_;
  int * originalRow  = prob->originalRow_;

  //presolvehlink *rlink = prob->rlink_;
  bool fixInfeasibility = (prob->presolveOptions_&16384)!=0;
  // Relax tolerance
  double tolerance = 10.0*prob->feasibilityTolerance_;


  int i;
  int nactions = 0;
  for (i=0; i<nrows; i++)
    if (hinrow[i] == 0)
      nactions++;

  if (nactions == 0)
    return next;
  else {
    action *actions 	= new action[nactions];
    int * rowmapping = new int [nrows];

    nactions = 0;
    int nrows2=0;
    for (i=0; i<nrows; i++) {
      if (hinrow[i] == 0) {
	action &e = actions[nactions];

#       if PRESOLVE_DEBUG
	if (nactions == 0)
	  printf("UNUSED ROWS:  ");
	else
	if (i < 100 && nactions%25 == 0)
	  printf("\n") ;
	else
	if (i >= 100 && i < 1000 && nactions%19 == 0)
	  printf("\n") ;
	else
	if (i >= 1000 && nactions%15 == 0)
	  printf("\n") ;
	printf("%d ", i);
#	endif

	nactions++;
	if (rlo[i] > 0.0 || rup[i] < 0.0) {
	  if ((rlo[i]<=tolerance &&
	       rup[i]>=-tolerance)||fixInfeasibility) {
	    rlo[i]=0.0;
	    rup[i]=0.0;
	  } else {
	    prob->status_|= 1;
	  prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					     prob->messages())
					       <<i
					       <<rlo[i]
					       <<rup[i]
					       <<CoinMessageEol;
	    break;
	  }
	}
	e.row	= i;
	e.rlo	= rlo[i];
	e.rup	= rup[i];
	rowmapping[i]=-1;

      } else {
	// move down - we want to preserve order
	rlo[nrows2]=rlo[i];
	rup[nrows2]=rup[i];
	originalRow[nrows2]=i;
	if (acts) {
	  acts[nrows2]=acts[i];
	  rowstat[nrows2]=rowstat[i];
	}
	rowmapping[i]=nrows2++;
      }
    }

    // remap matrix
    for (i=0;i<ncols;i++) {
      int j;
      for (j=mcstrt[i];j<mcstrt[i]+hincol[i];j++)
	hrow[j] = rowmapping[hrow[j]];
    }
    delete [] rowmapping;

    prob->nrows_ = nrows2;

#if	PRESOLVE_DEBUG
    presolve_check_nbasic(prob) ;
    if (nactions)
      printf("\ndropped %d rows\n", nactions);
#endif
    return (new drop_empty_rows_action(nactions, actions, next));
  }
}

void drop_empty_rows_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const int nactions	= nactions_;
  const action *const actions = actions_;

  int ncols	= prob->ncols_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;

  int *hrow	= prob->hrow_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;
  unsigned char *rowstat	= prob->rowstat_;
  double *rowduals = prob->rowduals_;
  double *acts	= prob->acts_;
# if PRESOLVE_DEBUG
  char *rdone	= prob->rdone_;
# endif

  int nrows0	= prob->nrows0_;
  int nrows	= prob->nrows_;

  int * rowmapping = new int [nrows0];
  CoinZeroN(rowmapping,nrows0) ;

  int i, action_i;
  for (action_i = 0; action_i<nactions; action_i++) {
    const action *e = &actions[action_i];
    int hole = e->row;
    rowmapping[hole]=-1;
  }

  // move data
  for (i=nrows0-1; i>=0; i--) {
    if (!rowmapping[i]) {
      // not a hole
      nrows--;
      rlo[i]=rlo[nrows];
      rup[i]=rup[nrows];
      acts[i]=acts[nrows];
      rowduals[i]=rowduals[nrows];
      if (rowstat)
	rowstat[i] = rowstat[nrows];
#     if PRESOLVE_DEBUG
      rdone[i] = rdone[nrows] ;
#     endif
    }
  }
  assert (!nrows);
  // set up mapping for matrix
  for (i=0;i<nrows0;i++) {
    if (!rowmapping[i])
      rowmapping[nrows++]=i;
  }

  for (int j=0; j<ncols; j++) {
    CoinBigIndex start = mcstrt[j];
    CoinBigIndex end   = start + hincol[j];

    for (CoinBigIndex k=start; k<end; ++k) {
      hrow[k] = rowmapping[hrow[k]];
    }
  }


  delete [] rowmapping;

  for (action_i = 0; action_i < nactions; action_i++) {
    const action *e = &actions[action_i];
    int irow = e->row;

    // Now recreate irow
    rlo[irow] = e->rlo;
    rup[irow] = e->rup;

    if (rowstat)
      prob->setRowStatus(irow,CoinPrePostsolveMatrix::basic);
    rowduals[irow] = 0.0;	// ???
    acts[irow] = 0.0;

#   if PRESOLVE_DEBUG
    rdone[irow] = DROP_ROW;
#   endif
  }

  prob->nrows_ = prob->nrows_+nactions;

# if PRESOLVE_DEBUG
  presolve_check_threads(prob) ;
# endif

}
