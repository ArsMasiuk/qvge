/* $Id: CoinPresolveSubst.cpp 1448 2011-06-19 15:34:41Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolvePsdebug.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveZeros.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinMessage.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "CoinError.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

namespace {	// begin unnamed file-local namespace

inline void prepend_elem(int jcol, double coeff, int irow,
		    CoinBigIndex *mcstrt,
		    double *colels,
		    int *hrow,
		    int *link, CoinBigIndex *free_listp)
{
  CoinBigIndex kk = *free_listp;
  assert(kk >= 0) ;
  *free_listp = link[*free_listp];
  link[kk] = mcstrt[jcol];
  mcstrt[jcol] = kk;
  colels[kk] = coeff;
  hrow[kk] = irow;
}

// add coeff_factor * rowy to rowx
static bool add_row(CoinBigIndex *mrstrt,
	     double *rlo, double * acts, double *rup,
	     double *rowels,
	     int *hcol,
	     int *hinrow,
	     presolvehlink *rlink, int nrows,
	     double coeff_factor,
	     int irowx, int irowy,
	     int *x_to_y)
{
  CoinBigIndex krs = mrstrt[irowy];
  CoinBigIndex kre = krs + hinrow[irowy];
  CoinBigIndex krsx = mrstrt[irowx];
  CoinBigIndex krex = krsx + hinrow[irowx];
  //  const int maxk = mrstrt[nrows];	// (22)

  // if irowx is very long, the searching gets very slow,
  // so we always sort.
  // whatever sorts rows should handle almost-sorted data efficiently
  // (quicksort may not)
  CoinSort_2(hcol+krsx,hcol+krsx+hinrow[irowx],rowels+krsx);
  CoinSort_2(hcol+krs,hcol+krs+hinrow[irowy],rowels+krs);
  //ekk_sort2(hcol+krsx, rowels+krsx, hinrow[irowx]);
  //ekk_sort2(hcol+krs,  rowels+krs,  hinrow[irowy]);

  //printf("%s x=%d y=%d cf=%g nx=%d ny=%d\n",
  // "ADD_ROW:",
  //  irowx, irowy, coeff_factor, hinrow[irowx], hinrow[irowy]);

# if PRESOLVE_DEBUG
  printf("%s x=%d y=%d cf=%g nx=%d ycols=(",
	 "ADD_ROW:",
	  irowx, irowy, coeff_factor, hinrow[irowx]);
# endif

  // adjust row bounds of rowx;
  // analogous to adjusting bounds info of colx in doubleton,
  // or perhaps adjustment to rlo/rup in elim_doubleton
  //
  // I believe that since we choose a column that is implied free,
  // no other column bounds need to be updated.
  // This is what would happen in doubleton if y's bounds were implied free;
  // in that case,
  // lo1 would never improve clo[icolx] and
  // up1 would never improve cup[icolx].
  {
    double rhsy = rlo[irowy];

    // (1)
    if (-PRESOLVE_INF < rlo[irowx]) {
#     if PRESOLVE_DEBUG
      if (rhsy * coeff_factor)
	printf("ELIM_ROW RLO:  %g -> %g\n",
	       rlo[irowx],
	       rlo[irowx] + rhsy * coeff_factor);
#     endif
      rlo[irowx] += rhsy * coeff_factor;
    }
    // (2)
    if (rup[irowx] < PRESOLVE_INF) {
#     if PRESOLVE_DEBUG
      if (rhsy * coeff_factor)
	printf("ELIM_ROW RUP:  %g -> %g\n",
	       rup[irowx],
	       rup[irowx] + rhsy * coeff_factor);
#     endif
      rup[irowx] += rhsy * coeff_factor;
    }
    if (acts)
    { acts[irowx] += rhsy * coeff_factor ; }
  }

  CoinBigIndex kcolx = krsx;
  CoinBigIndex krex0 = krex;
  int x_to_y_i = 0;

  for (CoinBigIndex krowy=krs; krowy<kre; krowy++) {
    int jcol = hcol[krowy];

    // even though these values are updated, they remain consistent
    PRESOLVEASSERT(krex == krsx + hinrow[irowx]);

    // see if row appears in colx
    // do NOT look beyond the original elements of rowx
    //CoinBigIndex kcolx = presolve_find_col1(jcol, krsx, krex, hcol);
    while (kcolx < krex0 && hcol[kcolx] < jcol)
      kcolx++;

#   if PRESOLVE_DEBUG
    printf("%d%s ", jcol, (kcolx < krex0 && hcol[kcolx] == jcol) ? "+" : "");
#   endif

    if (kcolx < krex0 && hcol[kcolx] == jcol) {
      // before:  both x and y are in the jcol
      // after:   only x is in the jcol
      // so: number of elems in col x unchanged, and num elems in jcol is one less

      // update row rep - just modify coefficent
      // column y is deleted as a whole at the end of the loop
#     if PRESOLVE_DEBUG
      printf("CHANGING %g + %g -> %g\n",
	     rowels[kcolx],
	     rowels[krowy],
	     rowels[kcolx] + rowels[krowy] * coeff_factor);
#     endif
      rowels[kcolx] += rowels[krowy] * coeff_factor;

      // this is where this element in rowy ended up
      x_to_y[x_to_y_i++] = kcolx - krsx;
      kcolx++;
    } else {
      // before:  only y is in the jcol
      // after:   only x is in the jcol
      // so: number of elems in col x is one greater, but num elems in jcol remains same
      {
	bool outOfSpace=presolve_expand_row(mrstrt,rowels,hcol,hinrow,rlink,nrows,irowx) ;
        if (outOfSpace)
          return true;
	// this may force a compaction
	// this will be called excessively if the rows are packed too tightly

	// have to adjust various induction variables
	krowy = mrstrt[irowy] + (krowy - krs);
	krs = mrstrt[irowy];			// do this for ease of debugging
	kre = mrstrt[irowy] + hinrow[irowy];

	kcolx = mrstrt[irowx] + (kcolx - krsx);	// don't really need to do this
	krex0 = mrstrt[irowx] + (krex0 - krsx);
	krsx = mrstrt[irowx];
	krex = mrstrt[irowx] + hinrow[irowx];
      }
      // this is where this element in rowy ended up
      x_to_y[x_to_y_i++] = krex - krsx;

      // there is now an unused entry in the memory after the column - use it
      // mrstrt[nrows] == penultimate index of arrays hcol/rowels
      hcol[krex] = jcol;
      rowels[krex] = rowels[krowy] * coeff_factor;
      hinrow[irowx]++, krex++;	// expand the col

      // do NOT increment kcolx
    }
  }

# if PRESOLVE_DEBUG
  printf(")\n");
# endif
  return false;
}


// It is common in osl to copy from one representation to another
// (say from a col rep to a row rep).
// One such routine is ekkclcp.
// This is similar, except that it does not assume that the
// representation is packed, and it adds some slack space
// in the target rep.
// It assumes both hincol/hinrow are correct.
// Note that such routines automatically sort the target rep by index,
// because they sweep the rows in ascending order.
void copyrep(const int * mrstrt, const int *hcol, const double *rowels,
	     const int *hinrow, int nrows,
	     int *mcstrt, int *hrow, double *colels,
	     int *hincol, int ncols)
{
  int pos = 0;
  for (int j = 0; j < ncols; ++j) {
    mcstrt[j] = pos;
    pos += hincol[j];
    pos += CoinMin(hincol[j], 10); // slack
    hincol[j] = 0;
  }

  for (int i = 0; i < nrows; ++i) {
    CoinBigIndex krs = mrstrt[i];
    CoinBigIndex kre = krs + hinrow[i];
    for (CoinBigIndex kr = krs; kr < kre; ++kr) {
      int icol = hcol[kr];
      int iput = hincol[icol];
      hincol[icol] = iput + 1;
      iput += mcstrt[icol];

      hrow[iput] = i;
      colels[iput] = rowels[kr];
    }
  }
}

} // end unnamed file-local namespace


const char *subst_constraint_action::name() const
{
  return ("subst_constraint_action");
}

// add -x/y times row y to row x, thus cancelling out one column of rowx;
// afterwards, that col will be singleton for rowy, so we drop the row.
//
// This no longer maintains the col rep as it goes along.
// Instead, it reconstructs it from scratch afterward.
//
// This implements the functionality of ekkrdc3.

/*
  This routine is called only from implied_free_action. There are several
  oddities and redundancies in the relationship. The two routines need a good
  grooming.

  try_fill_level limits the allowable number of coefficients in a column
  under consideration for substitution. There's some sort of hack going on
  that has the following effect: if try_fill_level comes in as 2, and that
  seems overly limiting (number of substitutions < 30), try increasing it to
  3. To trigger a wider examination of columns, this is actually passed back
  as -3. The next entry of implied_free_action (and then this routine) will
  override ColsToDo and examine all columns.

  Hence the initial loop triggered when try_fill_level < 0. Other positive
  values of fill_level will have no effect. A value of -3 will be converted
  (and passed back out) as +3. Arbitrary negative values of try_fill_level
  will also trigger the expansion of search and be converted to positive
  values.

  I would have thought that the columns considered by implied_free_action
  should also be limited by fill_level, but that's not currently the case.
  It's hard-wired to consider columns with 1 to 3 coefficients.

  There must be a better way.     -- lh, 040818 --
*/

const CoinPresolveAction *
subst_constraint_action::presolve(CoinPresolveMatrix *prob,
				  const int *implied_free,
				  const int * whichFree,
				  int numberFree,
				  const CoinPresolveAction *next,
				  int &try_fill_level)
{
  double *colels	= prob->colels_;
  int *hrow	= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;
  const int ncols	= prob->ncols_;

  double *rowels	= prob->rowels_;
  int *hcol	= prob->hcol_;
  CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow	= prob->hinrow_;
  const int nrows	= prob->nrows_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;
  double *acts	= prob->acts_;

  double *dcost		= prob->cost_;

  presolvehlink *clink = prob->clink_;
  presolvehlink *rlink = prob->rlink_;

  const double tol = prob->feasibilityTolerance_;

  action *actions	= new action [ncols];
# ifdef ZEROFAULT
  CoinZeroN(reinterpret_cast<char *>(actions),ncols*sizeof(action)) ;
# endif
  int nactions = 0;

  int *zerocols	= new int[ncols];
  int nzerocols	= 0;

  int *x_to_y	= new int[ncols];

#if 0
  // follmer.mps presents a challenge, since it has some very
  // long rows.  I started experimenting with how to deal with it,
  // but haven't yet finished.
  // The idea was to space out the rows to add some padding between them.
  // Ideally, we shouldn't have to do this just here, but could try to
  // do it a little everywhere.

  // sort the row rep by reconstructing from col rep
  copyrep(mcstrt, hrow, colels, hincol, ncols,
	  mrstrt, hcol, rowels, hinrow, nrows);
  presolve_make_memlists(/*mrstrt,*/ hinrow, rlink, nrows);
  // NEED SOME ASSERTION ABOUT NELEMS

  copyrep(mrstrt, hcol, rowels, hinrow, nrows,
	  mcstrt, hrow, colels, hincol, ncols);
  presolve_make_memlists(/*mcstrt,*/ hincol, clink, ncols);
#endif

  // in the original presolve, I don't think the two representations were
  // kept in sync.  It may be useful not to do that here, either,
  // but rather just keep the columns with nfill_level rows accurate
  // and resync at the end of the function.

  // DEBUGGING
#if	PRESOLVE_DEBUGx
  int maxsubst = atoi(getenv("MAXSUBST"));
#else
  const int maxsubst = 1000000;
#endif

  int nsubst = 0;

  // This loop does very nearly the same thing as
  // the first loop in implied_free_action::presolve.
  // We have to do it again in case constraints change while we process
  // them (???).
/*
  No --- given the hack with -3 coming in to implied_free_action and overriding
  ColsToDo, we could have columns in implied_free that aren't in ColsToDo.
  -- lh, 040818 --
*/
  int numberLook = prob->numberColsToDo_;
  int iLook;
  int * look = prob->colsToDo_;
  int fill_level = try_fill_level;
  int * look2 = NULL;
  // if gone from 2 to 3 look at all
  if (fill_level<0) {
    //abort();
    fill_level=-fill_level;
    try_fill_level=fill_level;
    look2 = new int[ncols];
    look=look2;
    if (!prob->anyProhibited()) {
      for (iLook=0;iLook<ncols;iLook++)
	look[iLook]=iLook;
      numberLook=ncols;
    } else {
      // some prohibited
      numberLook=0;
      for (iLook=0;iLook<ncols;iLook++)
	if (!prob->colProhibited(iLook))
	  look[numberLook++]=iLook;
    }
 }


  int * rowsUsed = prob->usefulRowInt_+prob->nrows_;
  int nRowsUsed=0;
  for (iLook=0;iLook<numberFree;iLook++) {
    int jcoly=whichFree[iLook];
    int whichRow = implied_free[iLook];
    if (hincol[jcoly] <2 || hincol[jcoly] > fill_level)
      continue;
    CoinBigIndex kcs = mcstrt[jcoly];
    CoinBigIndex kce = kcs + hincol[jcoly];

    int bestrowy_size = 0;
    int bestrowy_row=-1;
    int bestrowy_k=-1;
    double bestrowy_coeff=0.0;
    CoinBigIndex k;
    for (k=kcs; k<kce; ++k) {
      int row = hrow[k];
      double coeffj = colels[k];

      // we don't clean up zeros in the middle of the routine.
      // if there is one, skip this candidate.
      if (fabs(coeffj) <= ZTOLDP2 || prob->rowUsed(row)) {
	bestrowy_size = 0;
	break;
      }

      if (row==whichRow) {
	// if its row is an equality constraint...
	if (hinrow[row] > 1 &&	// don't bother with singleton rows

	    fabs(rlo[row] - rup[row]) < tol &&
	    !prob->rowUsed(row)) {
	  // both column bounds implied by the constraint bounds

	  // we want coeffy to be smaller than x, BACKWARDS from in doubleton
	  bestrowy_size = hinrow[row];
	  bestrowy_row = row;
	  bestrowy_coeff = coeffj;
	  bestrowy_k = k;
	}
      }
    }

    if (bestrowy_size == 0)
      continue;

    bool all_ok = true;
    for (k=kcs; k<kce; ++k) {
      double coeff_factor = fabs(colels[k] / bestrowy_coeff);
      if (fabs(coeff_factor) > 10.0)
	all_ok = false;
    }
#if 0		// block A
    // check fill-in
    if (all_ok && hincol[jcoly] == 3) {
      // compute fill-in
      int row1 = -1;
      int row2=-1;
      CoinBigIndex kk;
      for (kk=kcs; kk<kce; ++kk)
	if (kk != bestrowy_k) {
	  if (row1 == -1)
	    row1 = hrow[kk];
	  else
	    row2 = hrow[kk];
	}


      CoinBigIndex krs = mrstrt[bestrowy_row];
      CoinBigIndex kre = krs + hinrow[bestrowy_row];
      CoinBigIndex krs1 = mrstrt[row1];
      CoinBigIndex kre1 = krs + hinrow[row1];
      CoinBigIndex krs2 = mrstrt[row2];
      CoinBigIndex kre2 = krs + hinrow[row2];

      CoinSort_2(hcol+krs,hcol+krs+hinrow[bestrowy_row],rowels+krs);
      CoinSort_2(hcol+krs1,hcol+krs1+hinrow[row1],rowels+krs1);
      CoinSort_2(hcol+krs2,hcol+krs2+hinrow[row2],rowels+krs2);
      //ekk_sort2(hcol+krs,  rowels+krs,  hinrow[bestrowy_row]);
      //ekk_sort2(hcol+krs1, rowels+krs1, hinrow[row1]);
      //ekk_sort2(hcol+krs2, rowels+krs2, hinrow[row2]);

      int nfill = -hinrow[bestrowy_row];
      CoinBigIndex kcol1 = krs1;
      for (kk=krs; kk<kre; ++kk) {
	int jcol = hcol[kk];

	while (kcol1 < kre1 && hcol[kcol1] < jcol)
	  kcol1++;
	if (! (kcol1 < kre1 && hcol[kcol1] == jcol))
	  nfill++;
      }
      CoinBigIndex kcol2 = krs2;
      for (kk=krs; kk<kre; ++kk) {
	int jcol = hcol[kk];

	while (kcol2 < kre2 && hcol[kcol2] < jcol)
	  kcol2++;
	if (! (kcol2 < kre2 && hcol[kcol2] == jcol))
	  nfill++;
      }
#if	PRESOLVE_DEBUG
      printf("FILL:  %d\n", nfill);
#endif

#if 0
      static int maxfill = atoi(getenv("MAXFILL"));

      if (nfill > maxfill)
	all_ok = false;
#endif

      // not too much
      if (nfill <= 0)
	ngood++;

#if 0
      static int nts = 0;
      if (++nts > atoi(getenv("NTS")))
	all_ok = false;
      else
	nt++;
#endif
    }
#endif		// end block A
    // probably never happens
    if (all_ok && nzerocols + hinrow[bestrowy_row] >= ncols)
      all_ok = false;

    if (nsubst >= maxsubst) {
      all_ok = false;
    }

    if (all_ok) {
      nsubst++;
#if 0
      // debug
      if (numberLook<ncols&&iLook==numberLook-1) {
	printf("found last one?? %d\n", jcoly);
      }
#endif

      CoinBigIndex kcs = mcstrt[jcoly];
      int rowy = bestrowy_row;
      double coeffy = bestrowy_coeff;

      PRESOLVEASSERT(fabs(colels[kcs]) > ZTOLDP);
      PRESOLVEASSERT(fabs(colels[kcs+1]) > ZTOLDP);

      PRESOLVEASSERT(hinrow[rowy] > 1);

      const bool nonzero_cost = (fabs(dcost[jcoly]) > tol);

      double *costsx = nonzero_cost ? new double[hinrow[rowy]] : 0;

      int ntotels = 0;
      for (k=kcs; k<kce; ++k) {
	int irow = hrow[k];
	ntotels += hinrow[irow];
	// mark row as contaminated
	assert (!prob->rowUsed(irow));
	prob->setRowUsed(irow);
	rowsUsed[nRowsUsed++]=irow;
      }

      {
	action *ap = &actions[nactions++];
	int nincol = hincol[jcoly];

	ap->col = jcoly;
	ap->rowy = rowy;
	PRESOLVE_DETAIL_PRINT(printf("pre_subst %dC %dR E\n",jcoly,rowy));

	ap->nincol = nincol;
	ap->rows = new int[nincol];
	ap->rlos = new double[nincol];
	ap->rups = new double[nincol];

	// coefficients in deleted col
	ap->coeffxs = new double[nincol];

	ap->ninrowxs = new int[nincol];
	ap->rowcolsxs = new int[ntotels];
	ap->rowelsxs = new double[ntotels];

	ap->costsx = costsx;

	// copy all the rows for restoring later - wasteful
	{
	  int nel = 0;
	  for (CoinBigIndex k=kcs; k<kce; ++k) {
	    int irow = hrow[k];
	    CoinBigIndex krs = mrstrt[irow];
	    //#define COIN_SAFE_SUBST
#ifdef COIN_SAFE_SUBST
	    CoinBigIndex kre = krs + hinrow[irow];
	    for (CoinBigIndex k1=krs; k1<kre; ++k1) {
	      int jcol = hcol[k1];
	      if (jcol != jcoly) {
		CoinBigIndex kcs = mcstrt[jcol];
		CoinBigIndex kce = kcs + hincol[jcol];
		for (CoinBigIndex k2=kcs; k2<kce; ++k2) {
		  int irow = hrow[k2];
		  if (!prob->rowUsed(irow)) {
		    // mark row as contaminated
		    prob->setRowUsed(irow);
		    rowsUsed[nRowsUsed++]=irow;
		  }
		}
	      }
	    }
#endif

	    prob->addRow(irow);
	    ap->rows[k-kcs] = irow;
	    ap->ninrowxs[k-kcs] = hinrow[irow];
	    ap->rlos[k-kcs] = rlo[irow];
	    ap->rups[k-kcs] = rup[irow];

	    ap->coeffxs[k-kcs] = colels[k];

	    CoinMemcpyN( &hcol[krs],hinrow[irow], &ap->rowcolsxs[nel]);
	    CoinMemcpyN( &rowels[krs],hinrow[irow], &ap->rowelsxs[nel]);
	    nel += hinrow[irow];
	  }
	}
      }

      // rowy is supposed to be an equality row
      PRESOLVEASSERT(fabs(rup[rowy] - rlo[rowy]) < ZTOLDP);

      // now adjust for the implied free row - COPIED
      if (nonzero_cost) {
#if	0&&PRESOLVE_DEBUG
	printf("NONZERO SUBST COST:  %d %g\n", jcoly, dcost[jcoly]);
#endif
	double *cost = dcost;
	double *save_costs = costsx;
	double coeffj = coeffy;
	CoinBigIndex krs = mrstrt[rowy];
	CoinBigIndex kre = krs + hinrow[rowy];

	double rhs = rlo[rowy];
	double costj = cost[jcoly];

	for (CoinBigIndex k=krs; k<kre; k++) {
	  int jcol = hcol[k];
	  prob->addCol(jcol);
	  save_costs[k-krs] = cost[jcol];

	  if (jcol != jcoly) {
	    double coeff = rowels[k];

	    /*
	     * Similar to eliminating doubleton:
	     *   cost1 x = cost1 (c - b y) / a = (c cost1)/a - (b cost1)/a
	     *   cost[icoly] += cost[icolx] * (-coeff2 / coeff1);
	     */
	    cost[jcol] += costj * (-coeff / coeffj);
	  }
	}

	// I'm not sure about this
	prob->change_bias(costj * rhs / coeffj);

	// ??
	cost[jcoly] = 0.0;
      }

#if	0&&PRESOLVE_DEBUG
      if (hincol[jcoly] == 3) {
	CoinBigIndex krs = mrstrt[rowy];
	CoinBigIndex kre = krs + hinrow[rowy];
	printf("HROW0 (%d):  ", rowy);
	for (CoinBigIndex k=krs; k<kre; ++k) {
	  int jcol = hcol[k];
	  double coeff = rowels[k];
	  printf("%d:%g (%d) ", jcol, coeff, hincol[jcol]);
	}
	printf("\n");
      }
#endif

      if (hincol[jcoly] != 2) {
	CoinBigIndex krs = mrstrt[rowy];
	//	      CoinBigIndex kre = krs + hinrow[rowy];
	CoinSort_2(hcol+krs,hcol+krs+hinrow[rowy],rowels+krs);
	//ekk_sort2(hcol+krs,  rowels+krs,  hinrow[rowy]);
      }

      // substitute away jcoly in the other rows
      // Use ap as mcstrt etc may move if compacted
      kce = hincol[jcoly];
      action *ap = &actions[nactions-1];
      for (k=0; k<kce; ++k) {
	int rowx = ap->rows[k];
	//assert(rowx==hrow[k+kcs]);
	//assert(ap->coeffxs[k]==colels[k+kcs]);
	if (rowx != rowy) {
	  double coeffx = ap->coeffxs[k];
	  double coeff_factor = -coeffx / coeffy;	// backwards from doubleton

#if	0&&PRESOLVE_DEBUG
	  {
	    CoinBigIndex krs = mrstrt[rowx];
	    CoinBigIndex kre = krs + hinrow[rowx];
	    printf("HROW (%d %d %d):  ", rowx, hinrow[rowx], jcoly);
	    for (CoinBigIndex k=krs; k<kre; ++k) {
	      int jcol = hcol[k];
	      double coeff = rowels[k];
	      printf("%d ", jcol);
	    }
	    printf("\n");
#if 0
	    for (CoinBigIndex k=krs; k<kre; ++k) {
	      int jcol = hcol[k];
	      prob->addCol(jcol);
	      double coeff = rowels[k];
	      printf("%g ", coeff);
	    }
	    printf("\n");
#endif
	  }
#endif
	  {
	    CoinBigIndex krsx = mrstrt[rowx];
	    CoinBigIndex krex = krsx + hinrow[rowx];
	    int i;
	    for (i=krsx;i<krex;i++)
	      prob->addCol(hcol[i]);
	    if (hincol[jcoly] != 2)
	      CoinSort_2(hcol+krsx,hcol+krsx+hinrow[rowx],rowels+krsx);
	    //ekk_sort2(hcol+krsx, rowels+krsx, hinrow[rowx]);
	  }

	  // add (coeff_factor * <rowy>) to rowx
	  // does not affect rowy
	  // may introduce (or cancel) elements in rowx
	  bool outOfSpace = add_row(mrstrt,
				    rlo, acts, rup,
				    rowels, hcol,
				    hinrow,
				    rlink, nrows,
				    coeff_factor,
				    rowx, rowy,
				    x_to_y);
	  if (outOfSpace)
	    throwCoinError("out of memory",
			   "CoinImpliedFree::presolve");

	  // update col rep of rowx from row rep:
	  // for every col in rowy, copy the elem for that col in rowx
	  // from the row rep to the col rep
	  {
	    CoinBigIndex krs = mrstrt[rowy];
	    //		  CoinBigIndex kre = krs + hinrow[rowy];
	    int niny = hinrow[rowy];

	    CoinBigIndex krsx = mrstrt[rowx];
	    //		  CoinBigIndex krex = krsx + hinrow[rowx];
	    for (CoinBigIndex ki=0; ki<niny; ++ki) {
	      CoinBigIndex k = krs + ki;
	      int jcol = hcol[k];
	      prob->addCol(jcol);
	      CoinBigIndex kcs = mcstrt[jcol];
	      CoinBigIndex kce = kcs + hincol[jcol];

	      //double coeff = rowels[presolve_find_col(jcol, krsx, krex, hcol)];
	      if (hcol[krsx + x_to_y[ki]] != jcol)
		abort();
	      double coeff = rowels[krsx + x_to_y[ki]];

	      // see if rowx appeared in jcol in the col rep
	      CoinBigIndex k2 = presolve_find_row1(rowx, kcs, kce, hrow);

	      //PRESOLVEASSERT(fabs(coeff) > ZTOLDP);

	      if (k2 < kce) {
		// yes - just update the entry
		colels[k2] = coeff;
	      } else {
		// no - make room, then append
		bool outOfSpace=presolve_expand_row(mcstrt,colels,hrow,hincol,
						    clink,ncols,jcol) ;
		if (outOfSpace)
		  throwCoinError("out of memory",
				 "CoinImpliedFree::presolve");
		krsx = mrstrt[rowx];
		krs = mrstrt[rowy];
		kcs = mcstrt[jcol];
		kce = kcs + hincol[jcol];

		hrow[kce] = rowx;
		colels[kce] = coeff;
		hincol[jcol]++;
	      }
	    }
	  }
	  // now colels[k] == 0.0

#if 1
	  // now remove jcoly from rowx in the row rep
	  // better if this were first
	  presolve_delete_from_row(rowx, jcoly, mrstrt, hinrow, hcol, rowels);
#endif
#if	0&&PRESOLVE_DEBUG
	  {
	    CoinBigIndex krs = mrstrt[rowx];
	    CoinBigIndex kre = krs + hinrow[rowx];
	    printf("HROW (%d %d %d):  ", rowx, hinrow[rowx], jcoly);
	    for (CoinBigIndex k=krs; k<kre; ++k) {
	      int jcol = hcol[k];
	      double coeff = rowels[k];
	      printf("%d ", jcol);
	    }
	    printf("\n");
#if 0
	    for (CoinBigIndex k=krs; k<kre; ++k) {
	      int jcol = hcol[k];
	      double coeff = rowels[k];
	      printf("%g ", coeff);
	    }
	    printf("\n");
#endif
	  }
#endif

	  // don't have to update col rep, since entire col deleted later
	}
      }

#if	0&&PRESOLVE_DEBUG
      printf("\n");
#endif

      // the addition of rows may have created zero coefficients
      CoinMemcpyN( &hcol[mrstrt[rowy]],hinrow[rowy], &zerocols[nzerocols]);
      nzerocols += hinrow[rowy];

      // delete rowy in col rep
      {
	CoinBigIndex krs = mrstrt[rowy];
	CoinBigIndex kre = krs + hinrow[rowy];
	for (CoinBigIndex k=krs; k<kre; ++k) {
	  int jcol = hcol[k];

	  // delete rowy from the jcol
	  presolve_delete_from_col(rowy,jcol,mcstrt,hincol,hrow,colels) ;
	  if (hincol[jcol] == 0)
	    { PRESOLVE_REMOVE_LINK(clink,jcol) ; }
	}
      }
      // delete rowy in row rep
      hinrow[rowy] = 0;

      // This last is entirely dual to doubleton, but for the cost adjustment

      // eliminate col entirely from the col rep
      PRESOLVE_REMOVE_LINK(clink, jcoly);
      hincol[jcoly] = 0;

      // eliminate rowy entirely from the row rep
      PRESOLVE_REMOVE_LINK(rlink, rowy);
      //cost[irowy] = 0.0;

      rlo[rowy] = 0.0;
      rup[rowy] = 0.0;

#if	0 && PRESOLVE_DEBUG
      printf("ROWY COLS:  ");
      for (CoinBigIndex k=0; k<save_ninrowy; ++k)
	if (rowycols[k] != col) {
	  printf("%d ", rowycols[k]);
	  (void)presolve_find_col(rowycols[k], mrstrt[rowx], mrstrt[rowx]+hinrow[rowx],
				  hcol);
	}
      printf("\n");
#endif
#       if PRESOLVE_CONSISTENCY
      presolve_links_ok(prob) ;
      presolve_consistent(prob) ;
#       endif
    }

  }

  // Clear row used flags
  for (int i=0;i<nRowsUsed;i++)
    prob->unsetRowUsed(rowsUsed[i]);
  // general idea - only do doubletons until there are almost none left
  if (nactions < 30&&fill_level<prob->maxSubstLevel_)
    try_fill_level = -fill_level-1;
  if (nactions) {
#   if PRESOLVE_SUMMARY
    printf("NSUBSTS:  %d\n", nactions);
    //printf("NT: %d  NGOOD:  %d FILL_LEVEL:  %d\n", nt, ngood, fill_level);
#   endif
    next = new subst_constraint_action(nactions, CoinCopyOfArray(actions,nactions), next);

    next = drop_zero_coefficients_action::presolve(prob, zerocols, nzerocols, next);
  }
  delete [] look2;
  deleteAction(actions,action*);

  delete[]x_to_y;
  delete[]zerocols;

  return (next);
}

void subst_constraint_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions	= nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;
  //  int ncols		= prob->ncols_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *dcost	= prob->cost_;

  double *sol	= prob->sol_;
  double *rcosts	= prob->rcosts_;

  double *acts	= prob->acts_;
  double *rowduals = prob->rowduals_;

# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
  char *cdone	= prob->cdone_;
  char *rdone	= prob->rdone_;
# endif

  CoinBigIndex &free_list = prob->free_list_;

  //  const double ztoldj	= prob->ztoldj_;
  const double maxmin = prob->maxmin_;
  int k;

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int icol = f->col;

    int nincoly = f->nincol;
    double *rlos = f->rlos;
    double *rups = f->rups;
    int *rows = f->rows;

    double *coeffxs = f->coeffxs;

    int jrowy = f->rowy;

    int *ninrowxs = f->ninrowxs;
    const int *rowcolsxs = f->rowcolsxs;
    const double *rowelsxs = f->rowelsxs;

    /* the row was in the reduced problem */
    for (int i=0; i<nincoly; ++i) {
      if (rows[i] != jrowy)
	PRESOLVEASSERT(rdone[rows[i]]);
    }
    PRESOLVEASSERT(cdone[icol]==DROP_COL);
    PRESOLVEASSERT(rdone[jrowy]==DROP_ROW);

    // DEBUG CHECK
#if	1 && PRESOLVE_DEBUG
    {
      double actx = 0.0;
      const double ztolzb	= prob->ztolzb_;
      for (int j=0; j<prob->ncols_; ++j)
	if (hincol[j] > 0 && cdone[j]) {
	  CoinBigIndex krow = presolve_find_row1(jrowy, mcstrt[j], mcstrt[j] + hincol[j], hrow);
	  if (krow < mcstrt[j] + hincol[j])
	    actx += colels[krow] * sol[j];
      }
      if (! (fabs(acts[jrowy] - actx) < 100*ztolzb))
	printf("BAD ACTSX:  acts[%d]==%g != %g\n",
	       jrowy, acts[jrowy], actx);
      if (! (rlo[jrowy] - 100*ztolzb <= actx && actx <= rup[jrowy] + 100*ztolzb))
	printf("ACTSX NOT IN RANGE:  %d %g %g %g\n",
	       jrowy, rlo[jrowy], actx, rup[jrowy]);
    }
#endif

    int ninrowy=-1;
    const int *rowcolsy=NULL;
    const double *rowelsy=NULL;
    double coeffy=0.0;

    double rloy=1.0e50;
    {
      int nel = 0;
      for (int i=0; i<nincoly; ++i) {
	int row = rows[i];
	rlo[row] = rlos[i];
	rup[row] = rups[i];
	if (row == jrowy) {
	  ninrowy = ninrowxs[i];
	  rowcolsy = &rowcolsxs[nel];
	  rowelsy  = &rowelsxs[nel];

	  coeffy = coeffxs[i];
	  rloy = rlo[row];

	}
	nel += ninrowxs[i];
      }
    }
    double rhsy = rloy;

    // restore costs
    {
      const double *costs = f->costsx;
      if (costs)
	for (int i = 0; i<ninrowy; ++i) {
	  dcost[rowcolsy[i]] = costs[i];
	}
    }

    // solve for the equality to find the solution for the eliminated col
    // this is why we want coeffx < coeffy (55)
    {
      double sol0 = rloy;
      sol[icol] = 0.0;	// to avoid condition in loop
      for (k = 0; k<ninrowy; ++k) {
	int jcolx = rowcolsy[k];
	double coeffx = rowelsy[k];
	sol0 -= coeffx * sol[jcolx];
      }
      sol[icol] = sol0 / coeffy;

#     if PRESOLVE_DEBUG
      const double ztolzb	= prob->ztolzb_;
      double *clo	= prob->clo_;
      double *cup	= prob->cup_;

      if (! (sol[icol] > clo[icol] - ztolzb &&
	     cup[icol] + ztolzb > sol[icol]))
	printf("NEW SOL OUT-OF-TOL:  %g %g %g\n", clo[icol],
	       sol[icol], cup[icol]);
#     endif
    }

    // since this row is fixed
    acts[jrowy] = rloy;

    // acts[irow] always ok, since slack is fixed
    prob->setRowStatus(jrowy,CoinPrePostsolveMatrix::atLowerBound);

    // remove old rowx from col rep
    // we don't explicitly store what the current rowx is;
    // however, after the presolve, rowx contains a col for every
    // col in either the original rowx or the original rowy.
    // If there were cancellations, those were handled in subsequent
    // presolves.
    {
      // erase those cols in the other rows that occur in rowy
      // (with the exception of icol, which was deleted);
      // the other rows *must* contain these cols
      for (k = 0; k<ninrowy; ++k) {
	int col = rowcolsy[k];

	// remove jrowx from col in the col rep
	// icol itself was deleted, so won't be there
	if (col != icol)
	  for (int i = 0; i<nincoly; ++i) {
	    if (rows[i] != jrowy)
	      presolve_delete_from_col2(rows[i],col,mcstrt,hincol,hrow,/*colels,*/
					link,&free_list) ;
	  }
      }
#     if PRESOLVE_CONSISTENCY
      presolve_check_free_list(prob) ;
#     endif

      // initialize this for loops below
      hincol[icol] = 0;

      // now restore the original rows (other than rowy).
      // those cols that were also in rowy were just removed;
      // otherwise, they *must* already be there.
      // This loop and the next automatically create the rep for the new col.
      {
	const int *rowcolsx = rowcolsxs;
	const double *rowelsx = rowelsxs;

	for (int i = 0; i<nincoly; ++i) {
	  int ninrowx = ninrowxs[i];
	  int jrowx = rows[i];

	  if (jrowx != jrowy)
	    for (k = 0; k<ninrowx; ++k) {
	      int col = rowcolsx[k];
	      CoinBigIndex kcolx = presolve_find_row3(jrowx, mcstrt[col], hincol[col], hrow, link);

	      if (kcolx != -1) {
		PRESOLVEASSERT(presolve_find_col1(col, 0, ninrowy, rowcolsy) == ninrowy);
		// overwrite the existing entry
		colels[kcolx] = rowelsx[k];
	      } else {
		PRESOLVEASSERT(presolve_find_col1(col, 0, ninrowy, rowcolsy) < ninrowy);

		{
		  CoinBigIndex kk = free_list;
		  assert(kk >= 0 && kk < prob->bulk0_) ;
		  free_list = link[free_list];

		  link[kk] = mcstrt[col];
		  mcstrt[col] = kk;
		  colels[kk] = rowelsx[k];
		  hrow[kk] = jrowx;
		}
		++hincol[col];
	      }
	    }
	  rowcolsx += ninrowx;
	  rowelsx += ninrowx;
	}
#       if PRESOLVE_CONSISTENCY
        presolve_check_free_list(prob) ;
#       endif
      }

      // finally, add original rowy elements
      for (k = 0; k<ninrowy; ++k) {
	int col = rowcolsy[k];

	{
	  prepend_elem(col, rowelsy[k], jrowy, mcstrt, colels, hrow, link, &free_list);
	  ++hincol[col];
	}
      }
#     if PRESOLVE_CONSISTENCY
      presolve_check_free_list(prob) ;
#     endif
    }

    // my guess is that the CLAIM in doubleton generalizes to
    // equations with more than one x-style variable.
    // Since I can't see how to distinguish among them,
    // I assume that any of them will do.

    {
       //      CoinBigIndex k;
      double dj = maxmin*dcost[icol];
      double bounds_factor = rhsy/coeffy;
      for (int i=0; i<nincoly; ++i)
	if (rows[i] != jrowy) {
	  int row = rows[i];
	  double coeff = coeffxs[i];

	  // PROBABLY DOESN'T MAKE SENSE
	  acts[row] += coeff * bounds_factor;

	  dj -= rowduals[row] * coeff;
	}

      // DEBUG CHECK
      double acty = 0.0;
      for (k = 0; k<ninrowy; ++k) {
	int col = rowcolsy[k];
	acty += rowelsy[k] * sol[col];
      }

       PRESOLVEASSERT(fabs(acty - acts[jrowy]) < 100*ZTOLDP);

      // RECOMPUTING
      {
	const int *rowcolsx = rowcolsxs;
	const double *rowelsx = rowelsxs;

	for (int i=0; i<nincoly; ++i) {
	  int ninrowx = ninrowxs[i];

	  if (rows[i] != jrowy) {
	    int jrowx = rows[i];

	    double actx = 0.0;
	    for (k = 0; k<ninrowx; ++k) {
	      int col = rowcolsx[k];
	      actx += rowelsx[k] * sol[col];
	    }
	    PRESOLVEASSERT(rlo[jrowx] - prob->ztolzb_ <= actx
			   && actx <= rup[jrowx] + prob->ztolzb_);
	    acts[jrowx] = actx;
	    if (prob->getRowStatus(jrowx)!=CoinPrePostsolveMatrix::basic) {
	      if (actx-rlo[jrowx]<rup[jrowx]-actx)
		prob->setRowStatus(jrowx,CoinPrePostsolveMatrix::atLowerBound);
	      else
		prob->setRowStatus(jrowx,CoinPrePostsolveMatrix::atUpperBound);
	    }
	  }
	  rowcolsx += ninrowx;
	  rowelsx += ninrowx;
	}
      }

      // this is the coefficient we need to force col y's reduced cost to 0.0;
      // for example, this is obviously true if y is a singleton column
      rowduals[jrowy] = dj / coeffy;
      rcosts[icol] = 0.0;

      // furthermore, this should leave rcosts[colx] for all colx
      // in jrowx unchanged (????).
    }

    // Unlike doubleton, there should never be a problem with keeping
    // the reduced costs the way they were, because the other
    // variable's bounds are never changed, since col was implied free.
    //rowstat[jrowy] = 0;
    prob->setColumnStatus(icol,CoinPrePostsolveMatrix::basic);

#   if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
    cdone[icol] = SUBST_ROW;
    rdone[jrowy] = SUBST_ROW;
#   endif
  }

# if PRESOLVE_CONSISTENCY
  presolve_check_threads(prob) ;
# endif

  return ;
}



subst_constraint_action::~subst_constraint_action()
{
  const action *actions = actions_;

  for (int i=0; i<nactions_; ++i) {
    delete[]actions[i].rows;
    delete[]actions[i].rlos;
    delete[]actions[i].rups;
    delete[]actions[i].coeffxs;
    delete[]actions[i].ninrowxs;
    delete[]actions[i].rowcolsxs;
    delete[]actions[i].rowelsxs;


    //delete [](double*)actions[i].costsx;
    deleteAction(actions[i].costsx,double*);
  }

  // Must add cast to placate MS compiler
  //delete [] (subst_constraint_action::action*)actions_;
  deleteAction(actions_,subst_constraint_action::action*);
}
