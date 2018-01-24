/* $Id: CoinPresolveZeros.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveZeros.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

namespace {	// begin unnamed file-local namespace

// Sees how many zeros there are
static int count_col_zeros (int ncheckcols, int * checkcols,
                            const CoinBigIndex *mcstrt, double *colels,// int *hrow,
                            int *hincol)
{
  int nactions = 0;
  int i;

  for (i=0; i<ncheckcols; i++) {
    int col = checkcols[i];
    CoinBigIndex kcs = mcstrt[col];
    CoinBigIndex kce = mcstrt[col] + hincol[col];
    CoinBigIndex k;

    for (k=kcs; k<kce; ++k) {
      if (fabs(colels[k]) < ZTOLDP) {
	nactions++;
      }
    }
  }
  return (nactions);
}

// Sees how many zeros there are
static int count_col_zeros2 (int ncheckcols, int * checkcols,
                            const CoinBigIndex *mcstrt, double *colels,// int *hrow,
                            int *hincol)
{
  int nactions = 0;

  for (int col=0; col<ncheckcols; col++) {
    CoinBigIndex kcs = mcstrt[col];
    CoinBigIndex kce = mcstrt[col] + hincol[col];
    CoinBigIndex k;

    for (k=kcs; k<kce; ++k) {
      if (fabs(colels[k]) < ZTOLDP) {
	checkcols[nactions++]=col;
      }
    }
  }
  return (nactions);
}

// searches the cols in checkcols for zero entries.
// creates a dropped_zero entry for each one; doesn't check for out-of-memory.
// returns number of zeros found.

static int drop_col_zeros (int ncheckcols, int *checkcols,
		    const CoinBigIndex *mcstrt, double *colels, int *hrow,
		    int *hincol, presolvehlink *clink,
		    dropped_zero *actions)
{
  typedef dropped_zero action;
  int nactions = 0;
  int i;

  for (i=0; i<ncheckcols; i++) {
    int col = checkcols[i];
    CoinBigIndex kcs = mcstrt[col];
    CoinBigIndex kce = mcstrt[col] + hincol[col];
    CoinBigIndex k;

    for (k=kcs; k<kce; ++k) {
      if (fabs(colels[k]) < ZTOLDP) {
	actions[nactions].col = col;
	actions[nactions].row = hrow[k];

#       if PRESOLVE_DEBUG
	if (nactions == 0)
	  printf("ZEROS:  ");
	else
	if (nactions%10 == 0)
	  printf("\n") ;
	printf("(%d,%d) ", hrow[k], col);
#       endif

	nactions++;

	colels[k] = colels[kce-1];
	hrow[k]   = hrow[kce-1];
	kce--;
	hincol[col]--;

	--k;	// redo this position
      }
    }
  if (hincol[col] == 0)
    PRESOLVE_REMOVE_LINK(clink,col) ;
  }

# if PRESOLVE_DEBUG
  if (nactions)
    printf("\n");
# endif

  return (nactions);
}

// very similar to col, but without the buffer and reads zeros

static void drop_row_zeros(int nzeros, const dropped_zero *zeros,
		    const CoinBigIndex *mrstrt, double *rowels, int *hcol,
		    int *hinrow, presolvehlink *rlink)
{
  int i;
  for (i=0; i<nzeros; i++) {
    int row = zeros[i].row;
    CoinBigIndex krs = mrstrt[row];
    CoinBigIndex kre = mrstrt[row] + hinrow[row];
    CoinBigIndex k;

    for (k=krs; k<kre; k++) {
      if (fabs(rowels[k]) < ZTOLDP) {
	rowels[k] = rowels[kre-1];
	hcol[k]   = hcol[kre-1];
	kre--;
	hinrow[row]--;

	--k;	// redo this position
      }
    }
  if (hinrow[row] == 0)
    PRESOLVE_REMOVE_LINK(rlink,row) ;
  }
}

}	// end unnamed file-local namespace

const CoinPresolveAction
  *drop_zero_coefficients_action::presolve (CoinPresolveMatrix *prob,
					    int *checkcols,
					    int ncheckcols,
					    const CoinPresolveAction *next)
{
  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  presolvehlink *clink	= prob->clink_ ;
  presolvehlink *rlink	= prob->rlink_ ;

  //  int i;
  int nzeros;
  if (ncheckcols==prob->ncols_) {
    // can do faster
    nzeros = count_col_zeros2(ncheckcols,checkcols,
                               mcstrt,colels,/*hrow,*/hincol);
  } else {
    nzeros = count_col_zeros(ncheckcols,checkcols,
                               mcstrt,colels,/*hrow,*/hincol);
  }
  if (nzeros == 0) {
    return (next);
  } else {
    dropped_zero * zeros = new dropped_zero[nzeros];

    nzeros=drop_col_zeros((ncheckcols==prob->ncols_) ? nzeros : ncheckcols,
			  checkcols,
			  mcstrt,colels,hrow,hincol,clink,
			  zeros);
    double *rowels	= prob->rowels_;
    int *hcol		= prob->hcol_;
    CoinBigIndex *mrstrt		= prob->mrstrt_;
    int *hinrow		= prob->hinrow_;
    //    int nrows		= prob->nrows_;

#   if PRESOLVE_SUMMARY
    printf("NZEROS:  %d\n", nzeros);
#   endif

    // make the row rep consistent
    drop_row_zeros(nzeros,zeros,mrstrt,rowels,hcol,hinrow,rlink) ;

    dropped_zero *zeros1 = new dropped_zero[nzeros];
    CoinMemcpyN(zeros, nzeros, zeros1);

    delete [] zeros;
    return (new drop_zero_coefficients_action(nzeros, zeros1, next));
  }
}


const CoinPresolveAction *drop_zero_coefficients(CoinPresolveMatrix *prob,
					      const CoinPresolveAction *next)
{
  int ncheck		= prob->ncols_;
  int *checkcols	= new int[ncheck];

  if (prob->anyProhibited()) {
    // some prohibited
    ncheck=0;
    for (int i=0; i<prob->ncols_; i++)
      if (!prob->colProhibited(i))
	checkcols[ncheck++] = i;
  }

  const CoinPresolveAction *retval = drop_zero_coefficients_action::presolve(prob,
							 checkcols, ncheck,
							 next);
  delete[]checkcols;
  return (retval);
}

void drop_zero_coefficients_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const int nzeros	= nzeros_;
  const dropped_zero *const zeros = zeros_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;
  CoinBigIndex &free_list		= prob->free_list_;

  for (const dropped_zero *z = &zeros[nzeros-1]; zeros<=z; z--) {
    int irow	= z->row;
    int jcol	= z->col;

    {
      CoinBigIndex k = free_list;
      assert(k >= 0 && k < prob->bulk0_) ;
      free_list = link[free_list];
      hrow[k] = irow;
      colels[k] = 0.0;
      link[k] = mcstrt[jcol];
      mcstrt[jcol] = k;
    }

    hincol[jcol]++;
  }

# if PRESOLVE_CONSISTENCY
  presolve_check_free_list(prob) ;
# endif

}
