/* $Id: CoinPresolveDual.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveDual.hpp"
#include "CoinMessage.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinFloatEqual.hpp"
//#define PRESOLVE_TIGHTEN_DUALS 1
//#define PRESOLVE_DEBUG 1

#ifdef PRESOLVE_DEBUG
#include "CoinPresolvePsdebug.hpp"
#endif

// this looks for "dominated columns"
// following ekkredc
// rdmin == dwrow2
// rdmax == dwrow
// this transformation is applied in: k4.mps, lp22.mps

// make inferences using this constraint on reduced cost:
//	at optimality	dj>0 ==> var must be at lb
//			dj<0 ==> var must be at ub
//
// This implies:
//	there is no lb ==> dj<=0 at optimality
//	there is no ub ==> dj>=0 at optimality

/*
  This routine looks to be something of a work in progres. See the comment
  that begins with `Gack!'. And down in the bound propagation loop, why do we
  only work with variables with u_j = infty? The corresponding section of code
  for l_j = -infty is ifdef'd away. And why exclude the code protected by
  PRESOLVE_TIGHTEN_DUALS? And why are we using ekkinf instead of PRESOLVE_INF?

  There is no postsolve action because once we've identified a variable to fix
  we can invoke make_fixed_action.
*/
const CoinPresolveAction *remove_dual_action::presolve(CoinPresolveMatrix *prob,
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
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int ncols		= prob->ncols_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  unsigned char *colstat = prob->colstat_ ;

  // Used only in `fix if simple' section below. Remove dec'l to avoid
  // GCC compile warning.
  // double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  CoinBigIndex *mrstrt		= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
  double *csol	= prob->sol_;
  int nrows		= prob->nrows_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *dcost	= prob->cost_;
  const unsigned char *integerType = prob->integerType_;

  const double maxmin	= prob->maxmin_;

  const double ekkinf = 1e28;
  const double ekkinf2 = 1e20;
  const double ztoldj = prob->ztoldj_;

  CoinRelFltEq relEq(prob->ztolzb_) ;

  double *rdmin	= prob->usefulRowDouble_; //new double[nrows];
  double *rdmax	= reinterpret_cast<double *> (prob->usefulRowInt_); //new double[nrows];

# if PRESOLVE_DEBUG
  std::cout << "Entering remove_dual_action::presolve, " << nrows << " X " << ncols << "." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  // This combines the initialization of rdmin/rdmax to extreme values
  // (PRESOLVE_INF/-PRESOLVE_INF) with a version of the next loop specialized
  // for row slacks.
  // In this case, it is always the case that dprice==0.0 and coeff==1.0.
  int i;
  for ( i = 0; i < nrows; i++) {
    double sup = -rlo[i];	// slack ub; corresponds to cup[j]
    double slo = -rup[i];	// slack lb; corresponds to clo[j]
    bool no_lb = (slo <= -ekkinf);
    bool no_ub = (sup >= ekkinf);

    // here, dj==-row dual
    // the row slack has no lower bound, but it does have an upper bound,
    // then the reduced cost must be <= 0, so the row dual must be >= 0
    rdmin[i] = (no_lb && !no_ub) ? 0.0 : -PRESOLVE_INF;

    rdmax[i] = (no_ub && !no_lb) ? 0.0 :  PRESOLVE_INF;
  }

  // Look for col singletons and update bounds on dual costs
  // Take the min of the maxes and max of the mins
  int j;
  for ( j = 0; j<ncols; j++) {
    if (integerType[j])
      continue; // even if it has infinite bound now ....
    bool no_ub = (cup[j] >= ekkinf);
    bool no_lb = (clo[j] <= -ekkinf);

    if (hincol[j] == 1 &&

	// we need singleton cols that have exactly one bound
	(no_ub ^ no_lb)) {
      int row = hrow[mcstrt[j]];
      double coeff = colels[mcstrt[j]];

      PRESOLVEASSERT(fabs(coeff) > ZTOLDP);

      // I don't think the sign of dcost[j] matters

      // this row dual would make this col's reduced cost be 0
      double dprice = maxmin * dcost[j] / coeff;

      // no_ub == !no_lb
      // no_ub ==> !(dj<0)
      // no_lb ==> !(dj>0)
      // I don't think the case where !no_ub has actually been tested
      if ((coeff > 0.0) == no_ub) {
	// coeff>0 ==> making the row dual larger would make dj *negative*
	//		==> dprice is an upper bound on dj if no *ub*
	// coeff<0 ==> making the row dual larger would make dj *positive*
	//		==> dprice is an upper bound on dj if no *lb*
	if (rdmax[row] > dprice)	// reduced cost may be positive
	  rdmax[row] = dprice;
      } else if ((coeff < 0.0) == no_lb) { // no lower bound
	if (rdmin[row] < dprice) 	// reduced cost may be negative
	  rdmin[row] = dprice;
      }
    }
  }

  int *fix_cols	= prob->usefulColumnInt_; //new int[ncols];

  //int *fixdown_cols	= new int[ncols];

#if	PRESOLVE_TIGHTEN_DUALS
  double *djmin	= new double[ncols];
  double *djmax	= new double[ncols];
#endif
  int nfixup_cols	= 0;
  int nfixdown_cols	= ncols;

  int nPass=100;
  while (nPass-- > 0) {
    int tightened = 0;
    /* Perform duality tests */
    for (int j = 0; j<ncols; j++) {
      if (hincol[j] > 0) {
	CoinBigIndex kcs = mcstrt[j];
	CoinBigIndex kce = kcs + hincol[j];
	// Number of infinite rows
	int nflagu = 0;
	int nflagl = 0;
	// Number of ordinary rows
	int nordu = 0;
	int nordl = 0;
	double ddjlo = maxmin * dcost[j];
	double ddjhi = ddjlo;

	for (CoinBigIndex k = kcs; k < kce; k++) {
	  int i = hrow[k];
	  double coeff = colels[k];

	  if (coeff > 0.0) {
	    if (rdmin[i] >= -ekkinf2) {
	      ddjhi -= coeff * rdmin[i];
	      nordu ++;
	    } else {
	      nflagu ++;
	    }
	    if (rdmax[i] <= ekkinf2) {
	      ddjlo -= coeff * rdmax[i];
	      nordl ++;
	    } else {
	      nflagl ++;
	    }
	  } else {
	    if (rdmax[i] <= ekkinf2) {
	      ddjhi -= coeff * rdmax[i];
	      nordu ++;
	    } else {
	      nflagu ++;
	    }
	    if (rdmin[i] >= -ekkinf2) {
	      ddjlo -= coeff * rdmin[i];
	      nordl ++;
	    } else {
	      nflagl ++;
	    }
	  }
	}
	// See if we may be able to tighten a dual
	if (!integerType[j]) {
	  if (cup[j]>ekkinf) {
	    // dj can not be negative
	    if (nflagu==1&&ddjhi<-ztoldj) {
	      // We can make bound finite one way
	      for (CoinBigIndex k = kcs; k < kce; k++) {
		int i = hrow[k];
		double coeff = colels[k];

		if (coeff > 0.0&&rdmin[i] < -ekkinf2) {
		  // rdmax[i] has upper bound
		  if (ddjhi<rdmax[i]*coeff-ztoldj) {
		    double newValue = ddjhi/coeff;
		    // re-compute lo
		    if (rdmax[i] > ekkinf2 && newValue <= ekkinf2) {
		      nflagl--;
		      ddjlo -= coeff * newValue;
		    } else if (rdmax[i] <= ekkinf2) {
		      ddjlo -= coeff * (newValue-rdmax[i]);
		    }
		    rdmax[i] = newValue;
		    tightened++;
#if	PRESOLVE_DEBUG
		    printf("Col %d, row %d max pi now %g\n",j,i,rdmax[i]);
#endif
		  }
		} else if (coeff < 0.0 && rdmax[i] > ekkinf2) {
		  // rdmin[i] has lower bound
		  if (ddjhi<rdmin[i]*coeff-ztoldj) {
		    double newValue = ddjhi/coeff;
		    // re-compute lo
		    if (rdmin[i] < -ekkinf2 && newValue >= -ekkinf2) {
		      nflagl--;
		      ddjlo -= coeff * newValue;
		    } else if (rdmax[i] >= -ekkinf2) {
		      ddjlo -= coeff * (newValue-rdmin[i]);
		    }
		    rdmin[i] = newValue;
		    tightened++;
#if	PRESOLVE_DEBUG
		    printf("Col %d, row %d min pi now %g\n",j,i,rdmin[i]);
#endif
		    ddjlo = 0.0;
		  }
		}
	      }
	    } else if (nflagl==0&&nordl==1&&ddjlo<-ztoldj) {
	      // We may be able to tighten
	      for (CoinBigIndex k = kcs; k < kce; k++) {
		int i = hrow[k];
		double coeff = colels[k];

		if (coeff > 0.0) {
		  rdmax[i] += ddjlo/coeff;
		  ddjlo =0.0;
		  tightened++;
#if	PRESOLVE_DEBUG
		  printf("Col %d, row %d max pi now %g\n",j,i,rdmax[i]);
#endif
		} else if (coeff < 0.0 ) {
		  rdmin[i] += ddjlo/coeff;
		  ddjlo =0.0;
		  tightened++;
#if	PRESOLVE_DEBUG
		  printf("Col %d, row %d min pi now %g\n",j,i,rdmin[i]);
#endif
		}
	      }
	    }
	  }
#if 0
	  if (clo[j]<-ekkinf) {
	    // dj can not be positive
	    if (ddjlo>ztoldj&&nflagl==1) {
	      // We can make bound finite one way
	      for (CoinBigIndex k = kcs; k < kce; k++) {
		int i = hrow[k];
		double coeff = colels[k];

		if (coeff < 0.0&&rdmin[i] < -ekkinf2) {
		  // rdmax[i] has upper bound
		  if (ddjlo>rdmax[i]*coeff+ztoldj) {
		    double newValue = ddjlo/coeff;
		    // re-compute hi
		    if (rdmax[i] > ekkinf2 && newValue <= ekkinf2) {
		      nflagu--;
		      ddjhi -= coeff * newValue;
		    } else if (rdmax[i] <= ekkinf2) {
		      ddjhi -= coeff * (newValue-rdmax[i]);
		    }
		    rdmax[i] = newValue;
		    tightened++;
#if	PRESOLVE_DEBUG
		    printf("Col %d, row %d max pi now %g\n",j,i,rdmax[i]);
#endif
		  }
		} else if (coeff > 0.0 && rdmax[i] > ekkinf2) {
		  // rdmin[i] has lower bound
		  if (ddjlo>rdmin[i]*coeff+ztoldj) {
		    double newValue = ddjlo/coeff;
		    // re-compute lo
		    if (rdmin[i] < -ekkinf2 && newValue >= -ekkinf2) {
		      nflagu--;
		      ddjhi -= coeff * newValue;
		    } else if (rdmax[i] >= -ekkinf2) {
		      ddjhi -= coeff * (newValue-rdmin[i]);
		    }
		    rdmin[i] = newValue;
		    tightened++;
#if	PRESOLVE_DEBUG
		    printf("Col %d, row %d min pi now %g\n",j,i,rdmin[i]);
#endif
		  }
		}
	      }
	    } else if (nflagu==0&&nordu==1&&ddjhi>ztoldj) {
	      // We may be able to tighten
	      for (CoinBigIndex k = kcs; k < kce; k++) {
		int i = hrow[k];
		double coeff = colels[k];

		if (coeff < 0.0) {
		  rdmax[i] += ddjhi/coeff;
		  ddjhi =0.0;
		  tightened++;
#if	PRESOLVE_DEBUG
		  printf("Col %d, row %d max pi now %g\n",j,i,rdmax[i]);
#endif
		} else if (coeff > 0.0 ) {
		  rdmin[i] += ddjhi/coeff;
		  ddjhi =0.0;
		  tightened++;
#if	PRESOLVE_DEBUG
		  printf("Col %d, row %d min pi now %g\n",j,i,rdmin[i]);
#endif
		}
	      }
	    }
	  }
#endif
	}

#if	PRESOLVE_TIGHTEN_DUALS
	djmin[j] = (nflagl ?  -PRESOLVE_INF : ddjlo);
	djmax[j] = (nflagu ? PRESOLVE_INF : ddjhi);
#endif

	if (ddjlo > ztoldj && nflagl == 0&&!prob->colProhibited2(j)) {
	  // dj>0 at optimality ==> must be at lower bound
	  if (clo[j] <= -ekkinf) {
	    prob->messageHandler()->message(COIN_PRESOLVE_COLUMNBOUNDB,
					    prob->messages())
					      <<j
					      <<CoinMessageEol;
	    prob->status_ |= 2;
	    break;
	  } else {
	    fix_cols[--nfixdown_cols] = j;
	    PRESOLVE_DETAIL_PRINT(printf("pre_duallo %dC E\n",j));
#	    if PRESOLVE_DEBUG
	    printf("NDUAL: fixing x<%d>",fix_cols[nfixdown_cols]) ;
	    if (csol) printf(" = %g",csol[j]) ;
	    printf(" at lb = %g.\n",clo[j]) ;
#	    endif
	    //if (csol[j]-clo[j]>1.0e-7)
	    //printf("down %d row %d nincol %d\n",j,hrow[mcstrt[j]],hincol[j]);
	    // User may have given us feasible solution - move if simple
	    if (csol) {
# if 0
	/*
	  Except it's not simple. The net result is that we end up with an
	  excess of basic variables.
	*/
	      if (csol[j]-clo[j]>1.0e-7&&hincol[j]==1) {
		double value_j = colels[mcstrt[j]];
		double distance_j = csol[j]-clo[j];
		int row=hrow[mcstrt[j]];
		// See if another column can take value
		for (CoinBigIndex kk=mrstrt[row];kk<mrstrt[row]+hinrow[row];kk++) {
		  int k = hcol[kk];
		  if (colstat[k] == CoinPrePostsolveMatrix::superBasic)
		    continue ;

		  if (hincol[k]==1&&k!=j) {
		    double value_k = rowels[kk];
		    double movement;
		    if (value_k*value_j>0.0) {
		      // k needs to increase
		      double distance_k = cup[k]-csol[k];
		      movement = CoinMin((distance_j*value_j)/value_k,distance_k);
		    } else {
		      // k needs to decrease
		      double distance_k = clo[k]-csol[k];
		      movement = CoinMax((distance_j*value_j)/value_k,distance_k);
		    }
		    if (relEq(movement,0)) continue ;

		    csol[k] += movement;
		    if (relEq(csol[k],clo[k]))
		    { colstat[k] = CoinPrePostsolveMatrix::atLowerBound ; }
		    else
		    if (relEq(csol[k],cup[k]))
		    { colstat[k] = CoinPrePostsolveMatrix::atUpperBound ; }
		    else
		    if (colstat[k] != CoinPrePostsolveMatrix::isFree)
		    { colstat[k] = CoinPrePostsolveMatrix::basic ; }
		    printf("NDUAL: x<%d> moved %g to %g; ",
			   k,movement,csol[k]) ;
		    printf("lb = %g, ub = %g, status now %s.\n",
			   clo[k],cup[k],columnStatusString(colstat[k])) ;
		    distance_j -= (movement*value_k)/value_j;
		    csol[j] -= (movement*value_k)/value_j;
		    if (distance_j<1.0e-7)
		      break;
		  }
		}
	      }
# endif		// repair solution.

	      csol[j] = clo[j] ;	// but the bottom line is we've changed x<j>
	      colstat[j] = CoinPrePostsolveMatrix::atLowerBound ;
	    }
	  }
	} else if (ddjhi < -ztoldj && nflagu == 0&&!prob->colProhibited2(j)) {
	  // dj<0 at optimality ==> must be at upper bound
	  if (cup[j] >= ekkinf) {
	    prob->messageHandler()->message(COIN_PRESOLVE_COLUMNBOUNDA,
					    prob->messages())
					      <<j
					      <<CoinMessageEol;
	    prob->status_ |= 2;
	    break;
	  } else {
	    PRESOLVE_DETAIL_PRINT(printf("pre_dualup %dC E\n",j));
	    fix_cols[nfixup_cols++] = j;
#	    if PRESOLVE_DEBUG
	    printf("NDUAL: fixing x<%d>",fix_cols[nfixup_cols-1]) ;
	    if (csol) printf(" = %g",csol[j]) ;
	    printf(" at ub = %g.\n",cup[j]) ;
#	    endif
	    // User may have given us feasible solution - move if simple
	    // See comments for `fix at lb' case above.
	    //if (cup[j]-csol[j]>1.0e-7)
	    //printf("up %d row %d nincol %d\n",j,hrow[mcstrt[j]],hincol[j]);
	    if (csol) {
# if 0
	// See comments above.
	      if (cup[j]-csol[j]>1.0e-7&&hincol[j]==1) {
		double value_j = colels[mcstrt[j]];
		double distance_j = csol[j]-cup[j];
		int row=hrow[mcstrt[j]];
		// See if another column can take value
		for (CoinBigIndex kk=mrstrt[row];kk<mrstrt[row]+hinrow[row];kk++) {
		  int k = hcol[kk];
		  if (colstat[k] == CoinPrePostsolveMatrix::superBasic)
		    continue ;

		  if (hincol[k]==1&&k!=j) {
		    double value_k = rowels[kk];
		    double movement;
		    if (value_k*value_j<0.0) {
		      // k needs to increase
		      double distance_k = cup[k]-csol[k];
		      movement = CoinMin((distance_j*value_j)/value_k,distance_k);
		    } else {
		      // k needs to decrease
		      double distance_k = clo[k]-csol[k];
		      movement = CoinMax((distance_j*value_j)/value_k,distance_k);
		    }
		    if (relEq(movement,0)) continue ;

		    csol[k] += movement;
		    if (relEq(csol[k],clo[k]))
		    { colstat[k] = CoinPrePostsolveMatrix::atLowerBound ; }
		    else
		    if (relEq(csol[k],cup[k]))
		    { colstat[k] = CoinPrePostsolveMatrix::atUpperBound ; }
		    else
		    if (colstat[k] != CoinPrePostsolveMatrix::isFree)
		    { colstat[k] = CoinPrePostsolveMatrix::basic ; }
		    printf("NDUAL: x<%d> moved %g to %g; ",
			   k,movement,csol[k]) ;
		    printf("lb = %g, ub = %g, status now %s.\n",
			   clo[k],cup[k],columnStatusString(colstat[k])) ;
		    distance_j -= (movement*value_k)/value_j;
		    csol[j] -= (movement*value_k)/value_j;
		    if (distance_j>-1.0e-7)
		      break;
		  }
		}
	      }
# endif
	      csol[j] = cup[j] ;	// but the bottom line is we've changed x<j>
	      colstat[j] = CoinPrePostsolveMatrix::atUpperBound ;
	    }
	  }
	}
      }
    }

    // I don't know why I stopped doing this.
#if	PRESOLVE_TIGHTEN_DUALS
    const double *rowels	= prob->rowels_;
    const int *hcol	= prob->hcol_;
    const CoinBigIndex *mrstrt	= prob->mrstrt_;
    int *hinrow	= prob->hinrow_;
    // tighten row dual bounds, as described on p. 229
    for (int i = 0; i < nrows; i++) {
      bool no_ub = (rup[i] >= ekkinf);
      bool no_lb = (rlo[i] <= -ekkinf);

      if ((no_ub ^ no_lb) == true) {
	CoinBigIndex krs = mrstrt[i];
	CoinBigIndex kre = krs + hinrow[i];
	double rmax  = rdmax[i];
	double rmin  = rdmin[i];

	// all row columns are non-empty
	for (CoinBigIndex k=krs; k<kre; k++) {
	  double coeff = rowels[k];
	  int icol = hcol[k];
	  double djmax0 = djmax[icol];
	  double djmin0 = djmin[icol];

	  if (no_ub) {
	    // dj must not be negative
	    if (coeff > ZTOLDP2 && djmax0 <PRESOLVE_INF && cup[icol]>=ekkinf) {
	      double bnd = djmax0 / coeff;
	      if (rmax > bnd) {
#if	PRESOLVE_DEBUG
		printf("MAX TIGHT[%d,%d]:  %g --> %g\n", i,hrow[k], rdmax[i], bnd);
#endif
		rdmax[i] = rmax = bnd;
		tightened ++;
	      }
	    } else if (coeff < -ZTOLDP2 && djmax0 <PRESOLVE_INF && cup[icol] >= ekkinf) {
	      double bnd = djmax0 / coeff ;
	      if (rmin < bnd) {
#if	PRESOLVE_DEBUG
		printf("MIN TIGHT[%d,%d]:  %g --> %g\n", i, hrow[k], rdmin[i], bnd);
#endif
		rdmin[i] = rmin = bnd;
		tightened ++;
	      }
	    }
	  } else {	// no_lb
	    // dj must not be positive
	    if (coeff > ZTOLDP2 && djmin0 > -PRESOLVE_INF && clo[icol]<=-ekkinf) {
	      double bnd = djmin0 / coeff ;
	      if (rmin < bnd) {
#if	PRESOLVE_DEBUG
		printf("MIN1 TIGHT[%d,%d]:  %g --> %g\n", i, hrow[k], rdmin[i], bnd);
#endif
		rdmin[i] = rmin = bnd;
		tightened ++;
	      }
	    } else if (coeff < -ZTOLDP2 && djmin0 > -PRESOLVE_INF && clo[icol] <= -ekkinf) {
	      double bnd = djmin0 / coeff ;
	      if (rmax > bnd) {
#if	PRESOLVE_DEBUG
		printf("MAX TIGHT1[%d,%d]:  %g --> %g\n", i,hrow[k], rdmax[i], bnd);
#endif
		rdmax[i] = rmax = bnd;
		tightened ++;
	      }
	    }
	  }
	}
      }
    }
#endif

    if (tightened<100||nfixdown_cols<ncols||nfixup_cols)
      break;
#if	PRESOLVE_TIGHTEN_DUALS
    else
      printf("DUAL TIGHTENED!  %d\n", tightened);
#endif
  }
  assert (nfixup_cols<=nfixdown_cols);
  if (nfixup_cols) {
#if	PRESOLVE_DEBUG
    printf("NDUAL: %d up", nfixup_cols);
    for (i = 0 ; i < nfixup_cols ; i++) printf(" %d",fix_cols[i]) ;
    printf(".\n") ;
#endif
    next = make_fixed_action::presolve(prob, fix_cols, nfixup_cols, false, next);
  }

  if (nfixdown_cols<ncols) {
    int * fixdown_cols = fix_cols+nfixdown_cols;
    nfixdown_cols = ncols-nfixdown_cols;
#if	PRESOLVE_DEBUG
    printf("NDUAL: %d down", nfixdown_cols);
    for (i = 0 ; i < nfixdown_cols ; i++) printf(" %d",fixdown_cols[i]) ;
    printf(".\n") ;
#endif
    next = make_fixed_action::presolve(prob, fixdown_cols, nfixdown_cols, true, next);
  }
  // If dual says so then we can make equality row
  // Also if cost is in right direction and only one binding row for variable
  // We may wish to think about giving preference to rows with 2 or 3 elements
/*
  Gack! Ok, I can appreciate the thought here, but I'm seriously skeptical
  about writing canFix[0] before reading rdmin[0]. After that, we should be out
  of the interference zone for the typical situation where sizeof(double) is
  twice sizeof(int).
*/
  int * canFix = reinterpret_cast<int *> (rdmin);
  for ( i = 0; i < nrows; i++) {
    bool no_lb = (rlo[i] <= -ekkinf);
    bool no_ub = (rup[i] >= ekkinf);
    canFix[i]=0;
    if (no_ub && !no_lb ) {
      if ( rdmin[i]>0.0)
	canFix[i]=-1;
      else
	canFix[i]=-2;
    } else if (no_lb && !no_ub ) {
      if (rdmax[i]<0.0)
	canFix[i]=1;
      else
	canFix[i]=2;
    }
  }
  for (j = 0; j<ncols; j++) {
    if (hincol[j]<=1)
      continue;
    if (integerType[j])
      continue; // even if it has infinite bound now ....
    CoinBigIndex kcs = mcstrt[j];
    CoinBigIndex kce = kcs + hincol[j];
    int bindingUp=-1;
    int bindingDown=-1;
    if (cup[j]<ekkinf)
      bindingUp=-2;
    if (clo[j]>-ekkinf)
      bindingDown=-2;
    for (CoinBigIndex k = kcs; k < kce; k++) {
      int i = hrow[k];
      if (abs(canFix[i])!=2) {
	bindingUp=-2;
	bindingDown=-2;
	break;
      }
      double coeff = colels[k];
      if (coeff>0.0) {
	if (canFix[i]==2) {
	  // binding up
	  if (bindingUp==-1)
	    bindingUp = i;
	  else
	    bindingUp = -2;
	} else {
	  // binding down
	  if (bindingDown==-1)
	    bindingDown = i;
	  else
	    bindingDown = -2;
	}
      } else {
	if (canFix[i]==2) {
	  // binding down
	  if (bindingDown==-1)
	    bindingDown = i;
	  else
	    bindingDown = -2;
	} else {
	  // binding up
	  if (bindingUp==-1)
	    bindingUp = i;
	  else
	    bindingUp = -2;
	}
      }
    }
    double cost = maxmin * dcost[j];
    if (bindingUp>-2&&cost<=0.0) {
      // might as well make equality
      if (bindingUp>=0) {
	canFix[bindingUp] /= 2; //So -2 goes to -1 etc
	//printf("fixing row %d to ub by %d\n",bindingUp,j);
      } else {
	//printf("binding up row by %d\n",j);
      }
    } else if (bindingDown>-2 &&cost>=0.0) {
      // might as well make equality
      if (bindingDown>=0) {
	canFix[bindingDown] /= 2; //So -2 goes to -1 etc
	//printf("fixing row %d to lb by %d\n",bindingDown,j);
      } else {
	//printf("binding down row by %d\n",j);
      }
    }
  }
  // can't fix if integer and non-unit coefficient
  //const double *rowels	= prob->rowels_;
  //const int *hcol	= prob->hcol_;
  //const CoinBigIndex *mrstrt	= prob->mrstrt_;
  //int *hinrow	= prob->hinrow_;
  for ( i = 0; i < nrows; i++) {
    if (abs(canFix[i])==1) {
      CoinBigIndex krs = mrstrt[i];
      CoinBigIndex kre = krs + hinrow[i];
      for (CoinBigIndex k=krs; k<kre; k++) {
	int icol = hcol[k];
	if (cup[icol]>clo[icol]&&integerType[icol]) {
	  canFix[i]=0; // not safe
#ifdef COIN_DEVELOP
	  printf("no dual something CoinPresolveDual row %d col %d\n",
		 i,icol);
#endif
	}
      }
    }
    if (canFix[i]==1) {
      rlo[i]=rup[i];
      prob->addRow(i);
    } else if (canFix[i]==-1) {
      rup[i]=rlo[i];
      prob->addRow(i);
    }
  }

  //delete[]rdmin;
  //delete[]rdmax;

  //delete[]fixup_cols;
  //delete[]fixdown_cols;

#if	PRESOLVE_TIGHTEN_DUALS
  delete[]djmin;
  delete[]djmax;
#endif

  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    int droppedRows = prob->countEmptyRows() - startEmptyRows;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
    printf("CoinPresolveDual(1) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_);
  }

# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
  std::cout << "Leaving remove_dual_action::presolve." << std::endl ;
# endif

  return (next);
}
