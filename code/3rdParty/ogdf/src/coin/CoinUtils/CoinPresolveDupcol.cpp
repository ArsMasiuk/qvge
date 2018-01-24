/* $Id: CoinPresolveDupcol.cpp 1448 2011-06-19 15:34:41Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

//#define PRESOLVE_DEBUG 1
// Debugging macros/functions
//#define PRESOLVE_DETAIL 1
#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveDupcol.hpp"
#include "CoinSort.hpp"
#include "CoinFinite.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinMessage.hpp"
#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

#define DSEED2 2147483647.0
// Can be used from anywhere
void coin_init_random_vec(double *work, int n)
{
  double deseed = 12345678.0;

  for (int i = 0; i < n; ++i) {
    deseed *= 16807.;
    int jseed = static_cast<int> (deseed /    DSEED2);
    deseed -= static_cast<double> (jseed) * DSEED2;
    double random = deseed /  DSEED2;

    work[i]=random;
  }
}

namespace {	// begin unnamed file-local namespace

/*
  For each candidate major-dimension vector in majcands, calculate the sum
  over the vector, with each minor dimension weighted by a random amount.
  (E.g., calculate column sums with each row weighted by a random amount.)
  The sums are returned in the corresponding entries of majsums.
*/

  void compute_sums (int /*n*/, const int *majlens, const CoinBigIndex *majstrts,
		   int *minndxs, double *elems, const double *minmuls,
		   int *majcands, double *majsums, int nlook)

{ for (int cndx = 0 ; cndx < nlook ; ++cndx)
  { int i = majcands[cndx] ;
    PRESOLVEASSERT(majlens[i] > 0) ;

    CoinBigIndex kcs = majstrts[i] ;
    CoinBigIndex kce = kcs + majlens[i] ;

    double value = 0.0 ;

    for (CoinBigIndex k = kcs ; k < kce ; k++)
    { int irow = minndxs[k] ;
      value += minmuls[irow]*elems[k] ; }

    majsums[cndx] = value ; }

  return ; }


void create_col (int col, int n, double *els,
		 CoinBigIndex *mcstrt, double *colels, int *hrow, int *link,
		 CoinBigIndex *free_listp)
{
  int *rows = reinterpret_cast<int *>(els+n) ;
  CoinBigIndex free_list = *free_listp;
  int xstart = NO_LINK;
  for (int i=0; i<n; ++i) {
    CoinBigIndex k = free_list;
    assert(k >= 0) ;
    free_list = link[free_list];
    hrow[k]   = rows[i];
    colels[k] = els[i];
    link[k] = xstart;
    xstart = k;
  }
  mcstrt[col] = xstart;
  *free_listp = free_list;
}


} // end unnamed file-local namespace



const char *dupcol_action::name () const
{
  return ("dupcol_action");
}


/*
  Original comment: This is just ekkredc5, adapted into the new framework.
	The datasets scorpion.mps and allgrade.mps have duplicate columns.

  In case you don't have your OSL manual handy, a somewhat more informative
  explanation: We're looking for an easy-to-detect special case of linearly
  dependent columns, where the coefficients of the duplicate columns are
  exactly equal. The idea for locating such columns is to generate pseudo-
  random weights for each row and then calculate the weighted sum of
  coefficients of each column. Columns with equal sums are checked more
  thoroughly.

  Analysis of the situation says there are two major cases:
    * If the columns have equal objective coefficients, we can combine
      them.
    * If the columns have unequal objective coefficients, we may be able to
      fix one at bound. If the required bound doesn't exist, we have dual
      infeasibility (hence one of primal infeasibility or unboundedness).

  In the comments below are a few fragments of code from the original
  routine. I don't think they make sense, but I've left them for the nonce in
  case someone else recognises the purpose.   -- lh, 040909 --
*/

const CoinPresolveAction
    *dupcol_action::presolve (CoinPresolveMatrix *prob,
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

  double maxmin	= prob->maxmin_ ;

  double *colels	= prob->colels_ ;
  int *hrow		= prob->hrow_ ;
  CoinBigIndex *mcstrt	= prob->mcstrt_ ;
  int *hincol		= prob->hincol_ ;
  int ncols		= prob->ncols_ ;
  int nrows		= prob->nrows_ ;

  double *clo	= prob->clo_ ;
  double *cup	= prob->cup_ ;
  double *sol	= prob->sol_ ;
  double *rlo	= prob->rlo_ ;
  double *rup	= prob->rup_ ;

  // If all coefficients positive do more simply
  bool allPositive=true;
  double * rhs = prob->usefulRowDouble_; //new double[nrows];
  CoinMemcpyN(rup,nrows,rhs);
/*
  Scan the columns for candidates, and write the indices into sort. We're not
  interested in columns that are empty, prohibited, or integral.

  Question: Should we exclude singletons, which are useful in other transforms?
  Question: Why are we excluding integral columns?
*/
  // allow integral columns if asked for
  bool allowIntegers = ( prob->presolveOptions_&1)!=0;
  int *sort = prob->usefulColumnInt_; //new int[ncols] ;
  int nlook = 0 ;
  for (int j = 0 ; j < ncols ; j++) {
    if (hincol[j] == 0) continue ;
    // sort
    CoinSort_2(hrow+mcstrt[j],hrow+mcstrt[j]+hincol[j],
	       colels+mcstrt[j]);
    // check all positive and adjust rhs
    if (allPositive) {
      double lower = clo[j];
      if (lower<cup[j]) {
        for (int k=mcstrt[j];k<mcstrt[j]+hincol[j];k++) {
          double value=colels[k];
          if (value<0.0)
            allPositive=false;
          else
            rhs[hrow[k]] -= lower*value;
        }
      } else {
        for (int k=mcstrt[j];k<mcstrt[j]+hincol[j];k++) {
          double value=colels[k];
          rhs[hrow[k]] -= lower*value;
        }
      }
    }
    if (prob->colProhibited2(j)) continue ;
    //#define PRESOLVE_INTEGER_DUPCOL
#ifndef PRESOLVE_INTEGER_DUPCOL
    if (prob->isInteger(j)&&!allowIntegers) continue ;
#endif
    sort[nlook++] = j ; }
  if (nlook == 0)
    { //delete[] sort ;
      //delete [] rhs;
    return (next) ; }
/*
  Prep: add the coefficients of each candidate column. To reduce false
  positives, multiply each row by a `random' multiplier when forming the
  sums.  On return from compute_sums, sort and colsum are loaded with the
  indices and column sums, respectively, of candidate columns.  The pair of
  arrays are then sorted by sum so that equal sums are adjacent.
*/
  double *colsum = prob->usefulColumnDouble_; //new double[ncols] ;
  double *rowmul;
  if (!prob->randomNumber_) {
    rowmul = new double[nrows] ;
    coin_init_random_vec(rowmul,nrows) ;
  } else {
    rowmul = prob->randomNumber_;
  }
  compute_sums(ncols,hincol,mcstrt,hrow,colels,rowmul,sort,colsum,nlook) ;
  CoinSort_2(colsum,colsum+nlook,sort) ;
/*
  General prep --- unpack the various vectors we'll need, and allocate arrays
  to record the results.
*/
  presolvehlink *clink	= prob->clink_ ;

  double *rowels	= prob->rowels_ ;
  int *hcol		= prob->hcol_ ;
  const CoinBigIndex *mrstrt = prob->mrstrt_ ;
  int *hinrow		= prob->hinrow_ ;

  double *dcost	= prob->cost_ ;

  action *actions	= new action [nlook] ;
  int nactions = 0 ;
# ifdef ZEROFAULT
  memset(actions,0,nlook*sizeof(action)) ;
# endif

  int *fixed_down	= new int[nlook] ;
  int nfixed_down	= 0 ;
  int *fixed_up		= new int[nlook] ;
  int nfixed_up		= 0 ;

#if 0
  // Excluded in the original routine. I'm guessing it's excluded because
  // it's just not cost effective to worry about this. -- lh, 040908 --

  // It may be the case that several columns are duplicate.
  // If not all have the same cost, then we have to make sure
  // that we set the most expensive one to its minimum
  // now sort in each class by cost
  {
    double dval = colsum[0] ;
    int first = 0 ;
    for (int jj = 1; jj < nlook; jj++) {
      while (colsum[jj]==dval)
	jj++ ;

      if (first + 1 < jj) {
	double buf[jj - first] ;
	for (int i=first; i<jj; ++i)
	  buf[i-first] = dcost[sort[i]]*maxmin ;

	CoinSort_2(buf,buf+jj-first,sort+first) ;
	//ekk_sortonDouble(buf,&sort[first],jj-first) ;
      }
    }
  }
#endif
  // We will get all min/max but only if needed
  bool gotStuff=false;
/*
  Original comment: It appears to be the case that this loop is finished,
	there may still be duplicate cols left. I haven't done anything
	about that yet.

  Open the main loop to compare column pairs. We'll compare sort[jj] to
  sort[tgt]. This allows us to accumulate multiple columns into one. But
  we don't manage all-pairs comparison when we can't combine columns.

  We can quickly dismiss pairs which have unequal sums or lengths.
*/
  int isorted = -1 ;
  int tgt = 0 ;
  for (int jj = 1 ;  jj < nlook ; jj++)
    { if (colsum[jj] != colsum[jj-1]) {
      tgt = jj; // Must update before continuing
      continue ;
    }

    int j2 = sort[jj] ;
    int j1 = sort[tgt] ;
    int len2 = hincol[j2] ;
    int len1 =  hincol[j1] ;

    if (len2 != len1)
    { tgt = jj ;
      continue ; }
/*
  The final test: sort the columns by row index and compare each index and
  coefficient.
*/
    CoinBigIndex kcs = mcstrt[j2] ;
    CoinBigIndex kce = kcs+hincol[j2] ;
    int ishift = mcstrt[j1]-kcs ;

    if (len1 > 1 && isorted < j1)
    { CoinSort_2(hrow+mcstrt[j1],hrow+mcstrt[j1]+len1,
		 colels+mcstrt[j1]) ;
      isorted = j1 ; }
    if (len2 > 1 && isorted < j2)
    { CoinSort_2(hrow+kcs,hrow+kcs+len2,colels+kcs) ;
      isorted = j2 ; }

    CoinBigIndex k ;
    for (k = kcs ; k < kce ; k++)
    { if (hrow[k] != hrow[k+ishift] || colels[k] != colels[k+ishift])
      { break ; } }
    if (k != kce)
    { tgt = jj ;
      continue ; }
/*
  These really are duplicate columns. Grab values for convenient reference.
  Convert the objective coefficients for minimization.
*/
    double clo1 = clo[j1] ;
    double cup1 = cup[j1] ;
    double clo2 = clo[j2] ;
    double cup2 = cup[j2] ;
    double c1 = dcost[j1]*maxmin ;
    double c2 = dcost[j2]*maxmin ;
    PRESOLVEASSERT(!(clo1 == cup1 || clo2 == cup2)) ;
    // Get reasonable bounds on sum of two variables
    double lowerBound=-COIN_DBL_MAX;
    double upperBound=COIN_DBL_MAX;
    // For now only if lower bounds are zero
    if (!clo1&&!clo2) {
      // Only need bounds if c1 != c2
      if (c1!=c2) {
	if (!allPositive) {
#if 0

	  for (k=kcs;k<kce;k++) {
	    int iRow = hrow[k];
	    bool posinf = false;
	    bool neginf = false;
	    double maxup = 0.0;
	    double maxdown = 0.0;

	    // compute sum of all bounds except for j1,j2
	    CoinBigIndex kk;
	    CoinBigIndex kre = mrstrt[iRow]+hinrow[iRow];
	    double value1=0.0;
	    for (kk=mrstrt[iRow]; kk<kre; kk++) {
	      int col = hcol[kk];
	      if (col == j1||col==j2) {
		value1=rowels[kk];
		continue;
	      }
	      double coeff = rowels[kk];
	      double lb = clo[col];
	      double ub = cup[col];

	      if (coeff > 0.0) {
		if (PRESOLVE_INF <= ub) {
		  posinf = true;
		  if (neginf)
		    break;	// pointless
		} else {
		  maxup += ub * coeff;
		}
		if (lb <= -PRESOLVE_INF) {
		  neginf = true;
		  if (posinf)
		    break;	// pointless
		} else {
		  maxdown += lb * coeff;
		}
	      } else {
		if (PRESOLVE_INF <= ub) {
		  neginf = true;
		  if (posinf)
		    break;	// pointless
		} else {
		  maxdown += ub * coeff;
		}
		if (lb <= -PRESOLVE_INF) {
		  posinf = true;
		  if (neginf)
		    break;	// pointless
		} else {
		  maxup += lb * coeff;
		}
	      }
	    }

	    if (kk==kre) {
	      assert (value1);
	      if (value1>1.0e-5) {
		if (!neginf&&rup[iRow]<1.0e10)
		  if (upperBound*value1>rup[iRow]-maxdown)
		    upperBound = (rup[iRow]-maxdown)/value1;
		if (!posinf&&rlo[iRow]>-1.0e10)
		  if (lowerBound*value1<rlo[iRow]-maxup)
		    lowerBound = (rlo[iRow]-maxup)/value1;
	      } else if (value1<-1.0e-5) {
		if (!neginf&&rup[iRow]<1.0e10)
		  if (lowerBound*value1>rup[iRow]-maxdown) {
#ifndef NDEBUG
		    double x=lowerBound;
#endif
		    lowerBound = (rup[iRow]-maxdown)/value1;
		    assert (lowerBound == CoinMax(x,(rup[iRow]-maxdown)/value1));
		  }
		if (!posinf&&rlo[iRow]>-1.0e10)
		  if (upperBound*value1<rlo[iRow]-maxup) {
#ifndef NDEBUG
		    double x=upperBound;
#endif
		    upperBound = (rlo[iRow]-maxup)/value1;
		    assert(upperBound == CoinMin(x,(rlo[iRow]-maxup)/value1));
		  }
	      }
	    }
	  }
	  double l=lowerBound;
	  double u=upperBound;
#endif
	  if (!gotStuff) {
	    prob->recomputeSums(-1); // get min max
	    gotStuff=true;
	  }
	  int positiveInf=0;
	  int negativeInf=0;
	  double lo=0;
	  double up=0.0;
	  if (clo1<-PRESOLVE_INF)
	    negativeInf++;
	  else
	    lo+=clo1;
	  if (clo2<-PRESOLVE_INF)
	    negativeInf++;
	  else
	    lo+=clo2;
	  if (cup1>PRESOLVE_INF)
	    positiveInf++;
	  else
	    up+=cup1;
	  if (cup2>PRESOLVE_INF)
	    positiveInf++;
	  else
	    up+=cup2;
	  for (k=kcs;k<kce;k++) {
	    int iRow = hrow[k];
	    double value = colels[k];
	    int pInf = (value>0.0) ? positiveInf : negativeInf;
	    int nInf = (value>0.0) ? negativeInf : positiveInf;
	    int posinf = prob->infiniteUp_[iRow]-pInf;
	    int neginf = prob->infiniteDown_[iRow]-nInf;
	    if (posinf>0&&neginf>0)
	      continue; // this row can't bound
	    double maxup = prob->sumUp_[iRow];
	    double maxdown = prob->sumDown_[iRow];

	    if (value>0.0) {
	      maxdown -= value*lo;
	      maxup -= value*up;
	    } else {
	      maxdown -= value*up;
	      maxup -= value*lo;
	    }
	    if (value>1.0e-5) {
	      if (!neginf&&rup[iRow]<1.0e10)
		if (upperBound*value>rup[iRow]-maxdown)
		  upperBound = (rup[iRow]-maxdown)/value;
	      if (!posinf&&rlo[iRow]>-1.0e10)
		if (lowerBound*value<rlo[iRow]-maxup)
		  lowerBound = (rlo[iRow]-maxup)/value;
	    } else if (value<-1.0e-5) {
	      if (!neginf&&rup[iRow]<1.0e10)
		if (lowerBound*value>rup[iRow]-maxdown) {
		  lowerBound = (rup[iRow]-maxdown)/value;
		}
	      if (!posinf&&rlo[iRow]>-1.0e10)
		if (upperBound*value<rlo[iRow]-maxup) {
		  upperBound = (rlo[iRow]-maxup)/value;
		}
	    }
	  }
	  //assert (fabs(l-lowerBound)<1.0e-5&&fabs(u-upperBound)<1.0e-5);
	} else {
	  // can do faster
	  lowerBound=0.0;
	  for (k=kcs;k<kce;k++) {
	    int iRow = hrow[k];
	    double value=colels[k];
	    if (upperBound*value>rhs[iRow])
		upperBound = rhs[iRow]/value;
	  }
	}
      }
      // relax a bit
      upperBound -= 1.0e-9;
    } else {
      // Not sure what to do so give up
      continue;
    }
/*
  There are two main cases: The objective coefficients are equal or unequal.

  For equal objective coefficients c1 == c2, we can combine the columns by
  making the substitution x<j1> = x'<j1> - x<j2>. This will eliminate column
  sort[jj] = j2 and leave the new variable x' in column sort[tgt] = j1. tgt
  doesn't move.
*/
    if (c1 == c2)
    {
#ifdef PRESOLVE_INTEGER_DUPCOL
      if (!allowIntegers) {
	if (prob->isInteger(j1)) {
	  if (!prob->isInteger(j2)) {
	    if (cup2 < upperBound) //if (!prob->colInfinite(j2))
	      continue;
	    else
	      cup2 = COIN_DBL_MAX;
	  }
	} else if (prob->isInteger(j2)) {
	  if (cup1 < upperBound) //if (!prob->colInfinite(j1))
	    continue;
	  else
	    cup1 = COIN_DBL_MAX;
	}
	//printf("TakingINTeq\n");
      }
#endif
/*
  As far as the presolved lp, there's no difference between columns. But we
  need this relation to hold in order to guarantee that we can split the
  value of the combined column during postsolve without damaging the basis.
  (The relevant case is when the combined column is basic --- we need to be
  able to retain column j1 in the basis and make column j2 nonbasic.)
*/
      if (!(clo2+cup1 <= clo1+cup2))
      { CoinSwap(j1,j2) ;
	CoinSwap(clo1,clo2) ;
	CoinSwap(cup1,cup2) ;
	tgt = jj ; }
/*
  Create the postsolve action before we start to modify the columns.
*/
      PRESOLVE_STMT(printf("DUPCOL: (%d,%d) %d += %d\n",j1,j2,j1,j2)) ;
      PRESOLVE_DETAIL_PRINT(printf("pre_dupcol %dC %dC E\n",j2,j1));

      action *s = &actions[nactions++] ;
      s->thislo = clo[j2] ;
      s->thisup = cup[j2] ;
      s->lastlo = clo[j1] ;
      s->lastup = cup[j1] ;
      s->ithis  = j2 ;
      s->ilast  = j1 ;
      s->nincol = hincol[j2] ;
      s->colels = presolve_dupmajor(colels,hrow,hincol[j2],mcstrt[j2]) ;
/*
  Combine the columns into column j1. Upper and lower bounds and solution
  simply add, and the coefficients are unchanged.

  I'm skeptical of pushing a bound to infinity like this, but leave it for now.
  -- lh, 040908 --
*/
      clo1 += clo2 ;
      if (clo1 < -1.0e20)
      { clo1 = -PRESOLVE_INF ; }
      clo[j1] = clo1 ;
      cup1 += cup2 ;
      if (cup1 > 1.0e20)
      { cup1 = PRESOLVE_INF ; }
      cup[j1] = cup1 ;
      if (sol)
      { sol[j1] += sol[j2] ; }
      if (prob->colstat_)
      { if (prob->getColumnStatus(j1) == CoinPrePostsolveMatrix::basic ||
	    prob->getColumnStatus(j2) == CoinPrePostsolveMatrix::basic)
	{ prob->setColumnStatus(j1,CoinPrePostsolveMatrix::basic); } }
/*
  Empty column j2.
*/
      dcost[j2] = 0.0 ;
      if (sol)
      { sol[j2] = clo2 ; }
      CoinBigIndex k2cs = mcstrt[j2] ;
      CoinBigIndex k2ce = k2cs + hincol[j2] ;
      for (CoinBigIndex k = k2cs ; k < k2ce ; ++k)
      { presolve_delete_from_row(hrow[k],j2,mrstrt,hinrow,hcol,rowels) ; }
      hincol[j2] = 0 ;
      PRESOLVE_REMOVE_LINK(clink,j2) ;
      continue ; }
/*
  Unequal reduced costs. In this case, we may be able to fix one of the columns
  or prove dual infeasibility. Given column a_k, duals y, objective
  coefficient c_k, the reduced cost cbar_k = c_k - dot(y,a_k). Given
  a_1 = a_2, substitute for dot(y,a_1) in the relation for cbar_2 to get
    cbar_2 = (c_2 - c_1) + cbar_1
  Independent elements here are variable bounds l_k, u_k, and difference
  (c_2 - c_1). Infinite bounds for l_k, u_k will constrain the sign of cbar_k.
  Assume minimization. If you do the case analysis, you find these cases of
  interest:

        l_1	u_1	l_2	u_2	cbar_1	c_2-c_1	cbar_2	result

    A    any	finite	-inf	any	<= 0	  > 0	  <= 0	x_1 -> NBUB
    B   -inf	 any	 any	finite	<= 0	  < 0	  < 0	x_2 -> NBUB

    C   finite	 any	 any	+inf	>= 0	  < 0	  >= 0	x_1 -> NBLB
    D    any	+inf	finite	 any	>= 0	  > 0	  >= 0	x_2 -> NBLB

    E   -inf	any	 any	+inf	<= 0	  < 0	  >= 0	dual infeas
    F    any	inf	-inf	 any	>= 0	  > 0	  <= 0  dual infeas

    G    any	finite	finite	 any		  > 0		no inference
    H   finite	 any	 any	finite		  < 0		no inference

  The cases labelled dual infeasible are primal unbounded.

  To keep the code compact, we'll always aim to take x_2 to bound. In the cases
  where x_1 should go to bound, we'll swap. The implementation is boolean
  algebra. Define bits for infinite bounds and (c_2 > c_1), then look for the
  correct patterns.
*/
    else
    { int minterm = 0 ;
#ifdef PRESOLVE_INTEGER_DUPCOL
      if (!allowIntegers) {
	if (c2 > c1) {
	  if (cup1 < upperBound/*!prob->colInfinite(j1)*/ && (prob->isInteger(j1)||prob->isInteger(j2)))
	    continue ;
	} else {
	  if (cup2 < upperBound/*!prob->colInfinite(j2)*/ && (prob->isInteger(j1)||prob->isInteger(j2)))
	    continue ;
	}
	//printf("TakingINTne\n");
      }
#endif
      bool swapped = false ;
#if PRESOLVE_DEBUG
      printf("bounds %g %g\n",lowerBound,upperBound);
#endif
      if (c2 > c1) minterm |= 1<<0 ;
      if (cup2 >= PRESOLVE_INF/*prob->colInfinite(j2)*/) minterm |= 1<<1 ;
      if (clo2 <= -PRESOLVE_INF) minterm |= 1<<2 ;
      if (cup1 >= PRESOLVE_INF/*prob->colInfinite(j1)*/) minterm |= 1<<3 ;
      if (clo1 <= -PRESOLVE_INF) minterm |= 1<<4 ;
      // for now be careful - just one special case
      if (!clo1&&!clo2) {
        if (c2 > c1 && cup1 >= upperBound)
          minterm |= 1<<3;
        else if (c2 < c1 && cup2 >= upperBound)
          minterm |= 1<<1;
      }
/*
  The most common case in a well-formed system should be no inference. We're
  looking for x00x1 (case G) and 0xx00 (case H). This is where we have the
  potential to miss inferences: If there are three or more columns with the
  same sum, sort[tgt] == j1 will only be compared to the second in the
  group.
*/
      if ((minterm&0x0d) == 0x1 || (minterm&0x13) == 0)
      { tgt = jj ;
	continue ; }
/*
  Next remove the unbounded cases, 1xx10 and x11x1.
*/
      if ((minterm&0x13) == 0x12 || (minterm&0x0d) == 0x0d)
      { prob->setStatus(2) ;
	PRESOLVE_STMT(printf("DUPCOL: (%d,%d) Unbounded\n",j1,j2)) ;
	break ; }
/*
  With the no inference and unbounded cases removed, all that's left are the
  cases where we can push a variable to bound. Swap if necessary (x01x1 or
  0xx10) so that we're always fixing index j2.  This means that column
  sort[tgt] = j1 will be fixed. Unswapped, we fix column sort[jj] = j2.
*/
      if ((minterm&0x0d) == 0x05 || (minterm&0x13) == 0x02)
      { CoinSwap(j1, j2) ;
	CoinSwap(clo1, clo2) ;
	CoinSwap(cup1, cup2) ;
	CoinSwap(c1, c2) ;
	int tmp1 = minterm&0x18 ;
	int tmp2 = minterm&0x06 ;
	int tmp3 = minterm&0x01 ;
	minterm = (tmp1>>2)|(tmp2<<2)|(tmp3^0x01) ;
	swapped = true ; }
/*
  Force x_2 to upper bound? (Case B, boolean 1X100, where X == don't care.)
*/
      if ((minterm&0x13) == 0x10)
      { fixed_up[nfixed_up++] = j2 ;
	PRESOLVE_STMT(printf("DUPCOL: (%d,%d) %d -> NBUB\n",j1,j2,j2)) ;
	if (prob->colstat_)
	{ if (prob->getColumnStatus(j1) == CoinPrePostsolveMatrix::basic ||
	      prob->getColumnStatus(j2) == CoinPrePostsolveMatrix::basic)
	  { prob->setColumnStatus(j1,CoinPrePostsolveMatrix::basic) ; }
	  prob->setColumnStatus(j2,CoinPrePostsolveMatrix::atUpperBound) ; }
	if (sol)
	{ double delta2 = cup2-sol[j2] ;
	  sol[j2] = cup2 ;
	  sol[j1] -= delta2 ; }
	if (swapped)
	{ tgt = jj ; }
	continue ; }
/*
  Force x_2 to lower bound? (Case C, boolean X1011.)
*/
      if ((minterm&0x0d) == 0x09)
      { fixed_down[nfixed_down++] = j2 ;
	PRESOLVE_STMT(printf("DUPCOL: (%d,%d) %d -> NBLB\n",j1,j2,j2)) ;
	if (prob->colstat_)
	{ if (prob->getColumnStatus(j1) == CoinPrePostsolveMatrix::basic ||
	      prob->getColumnStatus(j2) == CoinPrePostsolveMatrix::basic)
	  { prob->setColumnStatus(j1,CoinPrePostsolveMatrix::basic) ; }
	  prob->setColumnStatus(j2,CoinPrePostsolveMatrix::atLowerBound) ; }
	if (sol)
	{ double delta2 = clo2-sol[j2] ;
	  sol[j2] = clo2 ;
	  sol[j1] -= delta2 ; }
	if (swapped)
	{ tgt = jj ; }
	continue ; } }
/*
  We should never reach this point in the loop --- all cases force a new
  iteration or loop termination. If we get here, something happened that we
  didn't anticipate.
*/
    PRESOLVE_STMT(printf("DUPCOL: (%d,%d) UNEXPECTED!\n",j1,j2)) ; }
/*
  What's left? Deallocate vectors, and call make_fixed_action to handle any
  variables that were fixed to bound.
*/
  if (rowmul != prob->randomNumber_)
    delete[] rowmul ;
  //delete[] colsum ;
  //delete[] sort ;
  //delete [] rhs;

# if PRESOLVE_SUMMARY || PRESOLVE_DEBUG
  if (nactions+nfixed_down+nfixed_up > 0)
  { printf("DUPLICATE COLS: %d combined, %d lb, %d ub\n",
	   nactions,nfixed_down,nfixed_up) ; }
# endif
  if (nactions)
  { next = new dupcol_action(nactions,CoinCopyOfArray(actions,nactions),next) ;
    // we can't go round again in integer
    prob->presolveOptions_ |= 0x80000000;
}
  deleteAction(actions,action*) ;

  if (nfixed_down)
  { next =
      make_fixed_action::presolve(prob,fixed_down,nfixed_down,true,next) ; }
  delete[]fixed_down ;

  if (nfixed_up)
  { next =
      make_fixed_action::presolve(prob,fixed_up,nfixed_up,false,next) ; }
  delete[]fixed_up ;

  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    int droppedRows = prob->countEmptyRows() - startEmptyRows ;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
    printf("CoinPresolveDupcol(128) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_);
  }
  return (next) ; }


void dupcol_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  double *sol	= prob->sol_;
  double *dcost	= prob->cost_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;

  double *rcosts	= prob->rcosts_;
  double tolerance = prob->ztolzb_;

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int icol  = f->ithis;	// was fixed
    int icol2 = f->ilast;	// was kept

    dcost[icol] = dcost[icol2];
    clo[icol] = f->thislo;
    cup[icol] = f->thisup;
    clo[icol2] = f->lastlo;
    cup[icol2] = f->lastup;

    create_col(icol,f->nincol,f->colels,mcstrt,colels,hrow,link,
	       &prob->free_list_) ;
#   if PRESOLVE_CONSISTENCY
    presolve_check_free_list(prob) ;
#   endif
    // hincol[icol] = hincol[icol2]; // right? - no - has to match number in create_col
    hincol[icol] = f->nincol;

    double l_j = f->thislo;
    double u_j = f->thisup;
    double l_k = f->lastlo;
    double u_k = f->lastup;
    double x_k_sol = sol[icol2];
    PRESOLVE_DETAIL_PRINT(printf("post icol %d %g %g %g icol2 %d %g %g %g\n",
	   icol,clo[icol],sol[icol],cup[icol],
				 icol2,clo[icol2],sol[icol2],cup[icol2]));
    if (l_j>-PRESOLVE_INF&& x_k_sol-l_j>=l_k-tolerance&&x_k_sol-l_j<=u_k+tolerance) {
      // j at lb, leave k
      prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atLowerBound);
      sol[icol] = l_j;
      sol[icol2] = x_k_sol - sol[icol];
    } else if (u_j<PRESOLVE_INF&& x_k_sol-u_j>=l_k-tolerance&&x_k_sol-u_j<=u_k+tolerance) {
      // j at ub, leave k
      prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atUpperBound);
      sol[icol] = u_j;
      sol[icol2] = x_k_sol - sol[icol];
    } else if (l_k>-PRESOLVE_INF&& x_k_sol-l_k>=l_j-tolerance&&x_k_sol-l_k<=u_j+tolerance) {
      // k at lb make j basic
      prob->setColumnStatus(icol,prob->getColumnStatus(icol2));
      sol[icol2] = l_k;
      sol[icol] = x_k_sol - l_k;
      prob->setColumnStatus(icol2,CoinPrePostsolveMatrix::atLowerBound);
    } else if (u_k<PRESOLVE_INF&& x_k_sol-u_k>=l_j-tolerance&&x_k_sol-u_k<=u_j+tolerance) {
      // k at ub make j basic
      prob->setColumnStatus(icol,prob->getColumnStatus(icol2));
      sol[icol2] = u_k;
      sol[icol] = x_k_sol - u_k;
      prob->setColumnStatus(icol2,CoinPrePostsolveMatrix::atUpperBound);
    } else {
      // both free!  superbasic time
      sol[icol] = 0.0;	// doesn't matter
      prob->setColumnStatus(icol,CoinPrePostsolveMatrix::isFree);
    }
    PRESOLVE_DETAIL_PRINT(printf("post2 icol %d %g icol2 %d %g\n",
	   icol,sol[icol],
				 icol2,sol[icol2]));
    // row activity doesn't change
    // dj of both variables is the same
    rcosts[icol] = rcosts[icol2];
    // leave until destructor
    //    deleteAction(f->colels,double *);

#   if PRESOLVE_DEBUG
    const double ztolzb = prob->ztolzb_;
    if (! (clo[icol] - ztolzb <= sol[icol] && sol[icol] <= cup[icol] + ztolzb))
	     printf("BAD DUPCOL BOUNDS:  %g %g %g\n", clo[icol], sol[icol], cup[icol]);
    if (! (clo[icol2] - ztolzb <= sol[icol2] && sol[icol2] <= cup[icol2] + ztolzb))
	     printf("BAD DUPCOL BOUNDS:  %g %g %g\n", clo[icol2], sol[icol2], cup[icol2]);
#   endif
  }
  // leave until desctructor
  //  deleteAction(actions_,action *);
}

dupcol_action::~dupcol_action()
{
    for (int i = nactions_-1; i >= 0; --i) {
	deleteAction(actions_[i].colels, double *);
    }
    deleteAction(actions_, action*);
}



/*
  Routines for duplicate rows. This is definitely unfinished --- there's no
  postsolve action.
*/

const char *duprow_action::name () const
{
  return ("duprow_action");
}

// This is just ekkredc4, adapted into the new framework.
/*
  I've made minimal changes for compatibility with dupcol: An initial scan to
  accumulate rows of interest in sort.
  -- lh, 040909 --
*/
const CoinPresolveAction
    *duprow_action::presolve (CoinPresolveMatrix *prob,
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
  double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
  int ncols		= prob->ncols_;
  int nrows		= prob->nrows_;

/*
  Scan the rows for candidates, and write the indices into sort. We're not
  interested in rows that are empty or prohibited.

  Question: Should we exclude singletons, which are useful in other transforms?
  Question: Why are we excluding integral columns?
*/
  int *sort = new int[nrows] ;
  int nlook = 0 ;
  for (int i = 0 ; i < nrows ; i++)
  { if (hinrow[i] == 0) continue ;
    if (prob->rowProhibited2(i)) continue ;
    // sort
    CoinSort_2(hcol+mrstrt[i],hcol+mrstrt[i]+hinrow[i],
	       rowels+mrstrt[i]);
    sort[nlook++] = i ; }
  if (nlook == 0)
  { delete[] sort ;
    return (next) ; }

  double * workrow = new double[nrows+1];

  double * workcol;
  if (!prob->randomNumber_) {
    workcol = new double[ncols+1];
    coin_init_random_vec(workcol, ncols);
  } else {
    workcol = prob->randomNumber_;
  }
  compute_sums(nrows,hinrow,mrstrt,hcol,rowels,workcol,sort,workrow,nlook);
  CoinSort_2(workrow,workrow+nlook,sort);

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  int nuseless_rows = 0;
  bool fixInfeasibility = (prob->presolveOptions_&16384)!=0;
  bool allowIntersection = ( prob->presolveOptions_&16)!=0;
  double tolerance = prob->feasibilityTolerance_;

  double dval = workrow[0];
  for (int jj = 1; jj < nlook; jj++) {
    if (workrow[jj]==dval) {
      int ithis=sort[jj];
      int ilast=sort[jj-1];
      CoinBigIndex krs = mrstrt[ithis];
      CoinBigIndex kre = krs + hinrow[ithis];
      if (hinrow[ithis] == hinrow[ilast]) {
	int ishift = mrstrt[ilast] - krs;
	CoinBigIndex k;
	for (k=krs;k<kre;k++) {
	  if (hcol[k] != hcol[k+ishift] ||
	      rowels[k] != rowels[k+ishift]) {
	    break;
	  }
	}
	if (k == kre) {
	  /* now check rhs to see what is what */
	  double rlo1=rlo[ilast];
	  double rup1=rup[ilast];
	  double rlo2=rlo[ithis];
	  double rup2=rup[ithis];

	  int idelete=-1;
	  if (rlo1<=rlo2) {
	    if (rup2<=rup1) {
	      /* this is strictly tighter than last */
	      idelete=ilast;
	      PRESOLVE_DETAIL_PRINT(printf("pre_duprow %dR %dR E\n",ilast,ithis));
	    } else if (fabs(rlo1-rlo2)<1.0e-12) {
	      /* last is strictly tighter than this */
	      idelete=ithis;
	      PRESOLVE_DETAIL_PRINT(printf("pre_duprow %dR %dR E\n",ithis,ilast));
	      // swap so can carry on deleting
	      sort[jj-1]=ithis;
	      sort[jj]=ilast;
	    } else {
	      if (rup1<rlo2-tolerance&&!fixInfeasibility) {
		// infeasible
		prob->status_|= 1;
		// wrong message - correct if works
		prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
						prob->messages())
						  <<ithis
						  <<rlo[ithis]
						  <<rup[ithis]
						  <<CoinMessageEol;
		break;
	      } else if (allowIntersection/*||fabs(rup1-rlo2)<tolerance*/) {
		/* overlapping - could merge */
#ifdef CLP_INVESTIGATE7
		printf("overlapping duplicate row %g %g, %g %g\n",
		       rlo1,rup1,rlo2,rup2);
#	      endif
		// pretend this is stricter than last
		idelete=ilast;
		PRESOLVE_DETAIL_PRINT(printf("pre_duprow %dR %dR E\n",ilast,ithis));
		rup[ithis]=rup1;
	      }
	    }
	  } else {
	    // rlo1>rlo2
	    if (rup1<=rup2) {
	      /* last is strictly tighter than this */
	      idelete=ithis;
	      PRESOLVE_DETAIL_PRINT(printf("pre_duprow %dR %dR E\n",ithis,ilast));
	      // swap so can carry on deleting
	      sort[jj-1]=ithis;
	      sort[jj]=ilast;
	    } else {
	      /* overlapping - could merge */
	      // rlo1>rlo2
	      // rup1>rup2
	      if (rup2<rlo1-tolerance&&!fixInfeasibility) {
		// infeasible
		prob->status_|= 1;
		// wrong message - correct if works
		prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
						prob->messages())
						  <<ithis
						  <<rlo[ithis]
						  <<rup[ithis]
						  <<CoinMessageEol;
		break;
	      } else if (allowIntersection/*||fabs(rup2-rlo1)<tolerance*/) {
#ifdef CLP_INVESTIGATE7
		printf("overlapping duplicate row %g %g, %g %g\n",
		       rlo1,rup1,rlo2,rup2);
#	      endif
		// pretend this is stricter than last
		idelete=ilast;
		PRESOLVE_DETAIL_PRINT(printf("pre_duprow %dR %dR E\n",ilast,ithis));
		rlo[ithis]=rlo1;
	      }
	    }
	  }
	  if (idelete>=0)
	    sort[nuseless_rows++]=idelete;
	}
      }
    }
    dval=workrow[jj];
  }

  delete[]workrow;
  if(workcol != prob->randomNumber_)
    delete[]workcol;


  if (nuseless_rows) {
#   if PRESOLVE_SUMMARY
    printf("DUPLICATE ROWS:  %d\n", nuseless_rows);
#   endif
    next = useless_constraint_action::presolve(prob,
					       sort, nuseless_rows,
					       next);
  }
  delete[]sort;

  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    int droppedRows = prob->countEmptyRows() - startEmptyRows ;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
    printf("CoinPresolveDuprow(256) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_);
  }
  return (next);
}

void duprow_action::postsolve(CoinPostsolveMatrix *) const
{
  printf("STILL NO POSTSOLVE FOR DUPROW!\n");
  abort();
}



/*
  Routines for gub rows. This is definitely unfinished --- there's no
  postsolve action.
*/

const char *gubrow_action::name () const
{
  return ("gubrow_action");
}


const CoinPresolveAction
    *gubrow_action::presolve (CoinPresolveMatrix *prob,
			      const CoinPresolveAction *next)
{
  double startTime = 0.0;
  int droppedElements=0;
  int affectedRows=0;
  if (prob->tuning_) {
    startTime = CoinCpuTime();
  }
  double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
  double *colels	= prob->colels_ ;
  int *hrow		= prob->hrow_ ;
  CoinBigIndex *mcstrt	= prob->mcstrt_ ;
  int *hincol		= prob->hincol_ ;
  int ncols		= prob->ncols_;
  int nrows		= prob->nrows_;
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

/*
  Scan the rows.  We're not
  interested in rows that are empty or prohibited.

*/
  int *which = prob->usefulRowInt_;
  int * number = which + nrows;
  double * els = prob->usefulRowDouble_;
  char * markCol = reinterpret_cast<char *> (prob->usefulColumnInt_);
  memset(markCol,0,ncols);
  CoinZeroN(els,nrows);
  for (int i = 0 ; i < nrows ; i++) {
    int nInRow = hinrow[i];
    if (nInRow>1 &&!prob->rowProhibited2(i)&&rlo[i]==rup[i]) {
      CoinBigIndex rStart = mrstrt[i];
      CoinBigIndex k = rStart;
      CoinBigIndex rEnd = rStart+nInRow;
      double value1=rowels[k];
      k++;
      for (;k<rEnd;k++) {
	if (rowels[k]!=value1)
	  break;
      }
      if (k==rEnd) {
	// Gub row
	int nLook = 0 ;
	for (k=rStart;k<rEnd;k++) {
	  int iColumn = hcol[k];
	  markCol[iColumn]=1;
	  CoinBigIndex kk = mcstrt[iColumn];
	  CoinBigIndex cEnd = kk+hincol[iColumn];
	  for (;kk<cEnd;kk++) {
	    int iRow = hrow[kk];
	    double value = colels[kk];
	    if (iRow!=i) {
	      double value2 = els[iRow];
	      if (value2) {
		if (value==value2)
		  number[iRow]++;
	      } else {
		// first
		els[iRow]=value;
		number[iRow]=1;
		which[nLook++]=iRow;
	      }
	    }
	  }
	}
	// Now see if any promising
	for (int j=0;j<nLook;j++) {
	  int iRow = which[j];
	  if (number[iRow]==nInRow) {
	    // can delete elements and adjust rhs
	    affectedRows++;
	    droppedElements += nInRow;
	    for (CoinBigIndex kk=rStart; kk<rEnd; kk++)
	      presolve_delete_from_col(iRow,hcol[kk],mcstrt,hincol,hrow,colels) ;
	    int nInRow2 = hinrow[iRow];
	    CoinBigIndex rStart2 = mrstrt[iRow];
	    CoinBigIndex rEnd2 = rStart2+nInRow2;
	    for (CoinBigIndex kk=rStart2; kk<rEnd2; kk++) {
	      int iColumn = hcol[kk];
	      if (markCol[iColumn]==0) {
		hcol[rStart2]=iColumn;
		rowels[rStart2++]=rowels[kk];
	      }
	    }
	    hinrow[iRow] = nInRow2-nInRow;
	    if (!hinrow[iRow])
	      PRESOLVE_REMOVE_LINK(prob->rlink_,iRow) ;
	    double value =(rlo[i]/value1)*els[iRow];
	    // correct rhs
	    if (rlo[iRow]>-1.0e20)
	      rlo[iRow] -= value;
	    if (rup[iRow]<1.0e20)
	      rup[iRow] -= value;
	  }
	  els[iRow]=0.0;
	}
	for (k=rStart;k<rEnd;k++) {
	  int iColumn = hcol[k];
	  markCol[iColumn]=0;
	}
      }
    }
  }
  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    printf("CoinPresolveGubrow(1024) - %d elements dropped (%d rows) in time %g, total %g\n",
	   droppedElements,affectedRows,thisTime-startTime,thisTime-prob->startTime_);
  } else if (droppedElements) {
#ifdef CLP_INVESTIGATE
    printf("CoinPresolveGubrow(1024) - %d elements dropped (%d rows)\n",
	   droppedElements,affectedRows);
#endif
  }
  return (next);
}

void gubrow_action::postsolve(CoinPostsolveMatrix *) const
{
  printf("STILL NO POSTSOLVE FOR GUBROW!\n");
  abort();
}
