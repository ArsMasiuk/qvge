/* $Id: CoinPresolveSingleton.cpp 1463 2011-07-30 16:31:31Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"

#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveSingleton.hpp"
#include "CoinPresolvePsdebug.hpp"
#include "CoinMessage.hpp"
#include "CoinFinite.hpp"

// #define PRESOLVE_DEBUG 1
// #define PRESOLVE_SUMMARY 1

/*
 * Transfers singleton row bound information to the corresponding column bounds.
 * What I refer to as a row singleton would be called a doubleton
 * in the paper, since my terminology doesn't refer to the slacks.
 * In terms of the paper, we transfer the bounds of the slack onto
 * the variable (vii) and then "substitute" the slack out of the problem
 * (which is a noop).
 */
const CoinPresolveAction *
slack_doubleton_action::presolve(CoinPresolveMatrix *prob,
				 const CoinPresolveAction *next,
				 bool & notFinished)
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
  //int ncols		= prob->ncols_;

  double *clo		= prob->clo_;
  double *cup		= prob->cup_;

  double *rowels	= prob->rowels_;
  const int *hcol	= prob->hcol_;
  const CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
  //  int nrows		= prob->nrows_;

  double *rlo		= prob->rlo_;
  double *rup		= prob->rup_;

  // If rowstat exists then all do
  unsigned char *rowstat	= prob->rowstat_;
  double *acts	= prob->acts_;
  double * sol = prob->sol_;
  //  unsigned char * colstat = prob->colstat_;

# if PRESOLVE_DEBUG
  std::cout << "Entering slack_doubleton_action::presolve." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  const unsigned char *integerType = prob->integerType_;

  const double ztolzb	= prob->ztolzb_;

  int numberLook = prob->numberRowsToDo_;
  int iLook;
  int * look = prob->rowsToDo_;
  bool fixInfeasibility = (prob->presolveOptions_&16384)!=0;

  action * actions = new action[numberLook];
  int nactions = 0;
  notFinished = false;

  int * fixed_cols = prob->usefulColumnInt_; //new int [ncols];
  int nfixed_cols	= 0;

  // this loop can apparently create new singleton rows;
  // I haven't bothered to detect this.
  // wasfor (int irow=0; irow<nrows; irow++)
  for (iLook=0;iLook<numberLook;iLook++) {
    int irow = look[iLook];
    if (hinrow[irow] == 1) {
      int jcol = hcol[mrstrt[irow]];
      double coeff = rowels[mrstrt[irow]];
      double lo = rlo[irow];
      double up = rup[irow];
      double acoeff = fabs(coeff);
      //      const bool singleton_col = (hincol[jcol] == 1);

      if (acoeff < ZTOLDP2)
	continue;

      // don't bother with fixed cols
      if (fabs(cup[jcol] - clo[jcol]) < ztolzb)
	continue;

      {
	// put column on stack of things to do next time
	prob->addCol(jcol);
	PRESOLVE_DETAIL_PRINT(printf("pre_singleton %dC %dR E\n",jcol,irow));
	action *s = &actions[nactions];
	nactions++;

	s->col = jcol;
	s->clo = clo[jcol];
	s->cup = cup[jcol];

	s->row = irow;
	s->rlo = rlo[irow];
	s->rup = rup[irow];

	s->coeff = coeff;
#if 0
	if (prob->tuning_) {
	  // really for gcc 4.6 compiler bug
	  printf("jcol %d %g %g irow %d %g %g coeff %g\n",
		 jcol,clo[jcol],cup[jcol],irow,rlo[irow],rup[irow],coeff);
	}
#endif
      }

      if (coeff < 0.0) {
	CoinSwap(lo, up);
	lo = -lo;
	up = -up;
      }

      if (lo <= -PRESOLVE_INF)
	lo = -PRESOLVE_INF;
      else {
	lo /= acoeff;
	if (lo <= -PRESOLVE_INF)
	  lo = -PRESOLVE_INF;
      }

      if (up > PRESOLVE_INF)
	up = PRESOLVE_INF;
      else {
	up /= acoeff;
	if (up > PRESOLVE_INF)
	  up = PRESOLVE_INF;
      }

      if (clo[jcol] < lo) {
	// If integer be careful
	if (integerType[jcol]) {
	  //#define COIN_DEVELOP
#ifdef COIN_DEVELOP
	  double lo2=lo;
#endif
	  if (fabs(lo-floor(lo+0.5))<0.000001)
	    lo=floor(lo+0.5);
#ifdef COIN_DEVELOP
	  if (lo!=lo2&&fabs(lo-floor(lo+0.5))<0.01)
	    printf("first lo %g second %g orig %g\n",lo2,lo,clo[jcol]);
#endif
	  if (clo[jcol] < lo)
	    clo[jcol] = lo;
	} else {
	  clo[jcol] = lo;
	}
      }

      if (cup[jcol] > up) {
	// If integer be careful
	if (integerType[jcol]) {
#ifdef COIN_DEVELOP
	  double up2=up;
#endif
	  if (fabs(up-floor(up+0.5))<0.000001)
	    up=floor(up+0.5);
#ifdef COIN_DEVELOP
	  if (up!=up2&&fabs(up-floor(up+0.5))<0.01)
	    printf("first up %g second %g orig %g\n",up2,up,cup[jcol]);
#endif
	  if (cup[jcol] > up)
	    cup[jcol] = up;
	} else {
	  cup[jcol] = up;
	}
      }
      if (fabs(cup[jcol] - clo[jcol]) < ZTOLDP) {
	fixed_cols[nfixed_cols++] = jcol;
      }

      if (lo > up) {
	if (lo <= up + prob->feasibilityTolerance_||fixInfeasibility) {
	  // If close to integer then go there
	  double nearest = floor(lo+0.5);
	  if (fabs(nearest-lo)<2.0*prob->feasibilityTolerance_) {
	    lo = nearest;
	    up = nearest;
	  } else {
	    lo = up;
	  }
	  clo[jcol] = lo;
	  cup[jcol] = up;
	} else {
	  prob->status_ |= 1;
	  prob->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
					     prob->messages())
					       <<jcol
					       <<lo
					       <<up
					       <<CoinMessageEol;
	  break;
	}
      }

#     if PRESOLVE_DEBUG
      printf("SINGLETON R-%d C-%d\n", irow, jcol);
#     endif

      // eliminate the row entirely from the row rep
      hinrow[irow] = 0;
      PRESOLVE_REMOVE_LINK(prob->rlink_,irow) ;

      // just to make things squeeky
      rlo[irow] = 0.0;
      rup[irow] = 0.0;

      if (rowstat&&sol) {
	// update solution and basis
	int basisChoice=0;
	int numberBasic=0;
	double movement = 0 ;
	if (prob->columnIsBasic(jcol)) {
	  numberBasic++;
	  basisChoice=2; // move to row to keep consistent
	}
	if (prob->rowIsBasic(irow))
	  numberBasic++;
	if (sol[jcol] <= clo[jcol]+ztolzb) {
	  movement = clo[jcol]-sol[jcol] ;
	  sol[jcol] = clo[jcol];
	  prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atLowerBound);
	} else if (sol[jcol] >= cup[jcol]-ztolzb) {
	  movement = cup[jcol]-sol[jcol] ;
	  sol[jcol] = cup[jcol];
	  prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atUpperBound);
	} else {
	  basisChoice=1;
	}
	if (numberBasic>1||basisChoice==1)
	  prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::basic);
	else if (basisChoice==2)
	  prob->setRowStatus(irow,CoinPrePostsolveMatrix::basic);
	if (movement) {
	  CoinBigIndex k;
	  for (k = mcstrt[jcol] ; k < mcstrt[jcol]+hincol[jcol] ; k++) {
	    int row = hrow[k];
	    if (hinrow[row])
	      acts[row] += movement*colels[k];
	  }
	}
      }

/*
  Remove the row from this col in the col rep. It can happen that this will
  empty the column, in which case we can delink it.
*/
      presolve_delete_from_col(irow,jcol,mcstrt,hincol,hrow,colels) ;
      if (hincol[jcol] == 0)
      { PRESOLVE_REMOVE_LINK(prob->clink_,jcol) ; }

      if (nactions >= numberLook) {
	notFinished=true;
	break;
      }
    }
  }

  if (nactions) {
#   if PRESOLVE_SUMMARY
    printf("SINGLETON ROWS:  %d\n", nactions);
#   endif
    action *save_actions = new action[nactions];
    CoinMemcpyN(actions, nactions, save_actions);
    next = new slack_doubleton_action(nactions, save_actions, next);

    if (nfixed_cols)
      next = make_fixed_action::presolve(prob, fixed_cols, nfixed_cols,
					 true, // arbitrary
					 next);
  }
  //delete [] fixed_cols;
  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    int droppedRows = prob->countEmptyRows() - startEmptyRows ;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
    printf("CoinPresolveSingleton(2) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_);
  }

# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
  std::cout << "Leaving slack_doubleton_action::presolve." << std::endl ;
# endif
  delete [] actions;

  return (next);
}

void slack_doubleton_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;
  //  int ncols		= prob->ncols_;

  double *clo		= prob->clo_;
  double *cup		= prob->cup_;

  double *rlo		= prob->rlo_;
  double *rup		= prob->rup_;

  double *sol		= prob->sol_;
  double *rcosts	= prob->rcosts_;

  double *acts		= prob->acts_;
  double *rowduals 	= prob->rowduals_;

  unsigned char *colstat		= prob->colstat_;
  //  unsigned char *rowstat		= prob->rowstat_;

# if PRESOLVE_DEBUG
  char *rdone		= prob->rdone_;

  std::cout << "Entering slack_doubleton_action::postsolve." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
  CoinBigIndex &free_list		= prob->free_list_;

  const double ztolzb	= prob->ztolzb_;

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int irow = f->row;
    double lo0 = f->clo;
    double up0 = f->cup;
    double coeff = f->coeff;
    int jcol = f->col;

    rlo[irow] = f->rlo;
    rup[irow] = f->rup;

    clo[jcol] = lo0;
    cup[jcol] = up0;

    acts[irow] = coeff * sol[jcol];

    // add new element
    {
      CoinBigIndex k = free_list;
      assert(k >= 0 && k < prob->bulk0_) ;
      free_list = link[free_list];
      hrow[k] = irow;
      colels[k] = coeff;
      link[k] = mcstrt[jcol];
      mcstrt[jcol] = k;
    }
    hincol[jcol]++;	// right?

    /*
     * Have to compute rowstat and rowduals, since we are adding the row.
     * that satisfy complentarity slackness.
     *
     * We may also have to modify colstat and rcosts if bounds
     * have been relaxed.
     */
    if (!colstat) {
      // ????
      rowduals[irow] = 0.0;
    } else {
      if (prob->columnIsBasic(jcol)) {
	/* variable is already basic, so slack in this row must be basic */
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::basic);

	rowduals[irow] = 0.0;
	/* hence no reduced costs change */
	/* since the column was basic, it doesn't matter if it is now
	   away from its bounds. */
	/* the slack is basic and its reduced cost is 0 */
      } else if ((fabs(sol[jcol] - lo0) <= ztolzb &&
		  rcosts[jcol] >= 0) ||

		 (fabs(sol[jcol] - up0) <= ztolzb &&
		  rcosts[jcol] <= 0)) {
	/* up against its bound but the rcost is still ok */
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::basic);

	rowduals[irow] = 0.0;
	/* hence no reduced costs change */
      } else if (! (fabs(sol[jcol] - lo0) <= ztolzb) &&
		 ! (fabs(sol[jcol] - up0) <= ztolzb)) {
	/* variable not marked basic,
	 * so was at a bound in the reduced problem,
	 * but its bounds were tighter in the reduced problem,
	 * so now it has to be made basic.
	 */
	prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::basic);
	prob->setRowStatusUsingValue(irow);

	/* choose a value for rowduals[irow] that forces rcosts[jcol] to 0.0 */
	/* new reduced cost = 0 = old reduced cost - new dual * coeff */
	rowduals[irow] = rcosts[jcol] / coeff;
	rcosts[jcol] = 0.0;

	/* this value is provably of the right sign for the slack */
	/* SHOULD SHOW THIS */
      } else {
	/* the solution is at a bound, but the rcost is wrong */

	prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::basic);
	prob->setRowStatusUsingValue(irow);

	/* choose a value for rowduals[irow] that forcesd rcosts[jcol] to 0.0 */
	rowduals[irow] = rcosts[jcol] / coeff;
	rcosts[jcol] = 0.0;

	/* this value is provably of the right sign for the slack */
	/*rcosts[irow] = -rowduals[irow];*/
      }
    }

#   if PRESOLVE_DEBUG
    rdone[irow] = SLACK_DOUBLETON;
#   endif
  }

# if PRESOLVE_CONSISTENCY
  presolve_check_threads(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
  std::cout << "Leaving slack_doubleton_action::postsolve." << std::endl ;
# endif

  return ;
}
/*
    If we have a variable with one entry and no cost then we can
    transform the row from E to G etc.
    If there is a row objective region then we may be able to do
    this even with a cost.
*/
const CoinPresolveAction *
slack_singleton_action::presolve(CoinPresolveMatrix *prob,
				 const CoinPresolveAction *next,
                                 double * rowObjective)
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
  //int ncols		= prob->ncols_;

  double *clo		= prob->clo_;
  double *cup		= prob->cup_;

  double *rowels	= prob->rowels_;
  int *hcol	= prob->hcol_;
  CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
  int nrows		= prob->nrows_;

  double *rlo		= prob->rlo_;
  double *rup		= prob->rup_;

  // If rowstat exists then all do
  unsigned char *rowstat	= prob->rowstat_;
  double *acts	= prob->acts_;
  double * sol = prob->sol_;
  //unsigned char * colstat = prob->colstat_;

  const unsigned char *integerType = prob->integerType_;

  const double ztolzb	= prob->ztolzb_;
  double *dcost	= prob->cost_;
  //const double maxmin	= prob->maxmin_;

# if PRESOLVE_DEBUG
  std::cout << "Entering slack_singleton_action::presolve." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  int numberLook = prob->numberColsToDo_;
  int iLook;
  int * look = prob->colsToDo_;
  // Make sure we allocate at least one action
  int maxActions = CoinMin(numberLook,nrows/10)+1;
  action * actions = new action[maxActions];
  int nactions = 0;
  int * fixed_cols = new int [numberLook];
  int nfixed_cols=0;
  int nWithCosts=0;
  double costOffset=0.0;
  for (iLook=0;iLook<numberLook;iLook++) {
    int iCol = look[iLook];
    if (dcost[iCol])
      continue;
    if (hincol[iCol] == 1) {
      int iRow=hrow[mcstrt[iCol]];
      double coeff = colels[mcstrt[iCol]];
      double acoeff = fabs(coeff);
      if (acoeff < ZTOLDP2)
	continue;
      // don't bother with fixed cols
      if (fabs(cup[iCol] - clo[iCol]) < ztolzb)
	continue;
      if (integerType&&integerType[iCol]) {
	// only possible if everything else integer and unit coefficient
	// check everything else a bit later
	if (acoeff!=1.0)
	  continue;
        double currentLower = rlo[iRow];
        double currentUpper = rup[iRow];
	if (coeff==1.0&&currentLower==1.0&&currentUpper==1.0) {
	  // leave if integer slack on sum x == 1
	  bool allInt=true;
	  for (CoinBigIndex j=mrstrt[iRow];
	       j<mrstrt[iRow]+hinrow[iRow];j++) {
	    int iColumn = hcol[j];
	    double value = fabs(rowels[j]);
	    if (!integerType[iColumn]||value!=1.0) {
	      allInt=false;
	      break;
	    }
	  }
	  if (allInt)
	    continue; // leave as may help search
	}
      }
      if (!prob->colProhibited(iCol)) {
        double currentLower = rlo[iRow];
        double currentUpper = rup[iRow];
        if (!rowObjective) {
          if (dcost[iCol])
            continue;
        } else if ((dcost[iCol]&&currentLower!=currentUpper)||rowObjective[iRow]) {
          continue;
        }
        double newLower=currentLower;
        double newUpper=currentUpper;
        if (coeff<0.0) {
          if (currentUpper>1.0e20||cup[iCol]>1.0e20) {
            newUpper=COIN_DBL_MAX;
          } else {
            newUpper -= coeff*cup[iCol];
            if (newUpper>1.0e20)
              newUpper=COIN_DBL_MAX;
          }
          if (currentLower<-1.0e20||clo[iCol]<-1.0e20) {
            newLower=-COIN_DBL_MAX;
          } else {
            newLower -= coeff*clo[iCol];
            if (newLower<-1.0e20)
              newLower=-COIN_DBL_MAX;
          }
        } else {
          if (currentUpper>1.0e20||clo[iCol]<-1.0e20) {
            newUpper=COIN_DBL_MAX;
          } else {
            newUpper -= coeff*clo[iCol];
            if (newUpper>1.0e20)
              newUpper=COIN_DBL_MAX;
          }
          if (currentLower<-1.0e20||cup[iCol]>1.0e20) {
            newLower=-COIN_DBL_MAX;
          } else {
            newLower -= coeff*cup[iCol];
            if (newLower<-1.0e20)
              newLower=-COIN_DBL_MAX;
          }
        }
	if (integerType&&integerType[iCol]) {
	  // only possible if everything else integer
	  if (newLower>-1.0e30) {
	    if (newLower!=floor(newLower+0.5))
	      continue;
	  }
	  if (newUpper<1.0e30) {
	    if (newUpper!=floor(newUpper+0.5))
	      continue;
	  }
	  bool allInt=true;
	  for (CoinBigIndex j=mrstrt[iRow];
	       j<mrstrt[iRow]+hinrow[iRow];j++) {
	    int iColumn = hcol[j];
	    double value = fabs(rowels[j]);
	    if (!integerType[iColumn]||value!=floor(value+0.5)) {
	      allInt=false;
	      break;
	    }
	  }
	  if (!allInt)
	    continue; // no good
	}
        if (nactions>=maxActions) {
          maxActions += CoinMin(numberLook-iLook,maxActions);
          action * temp = new action[maxActions];
	  memcpy(temp,actions,nactions*sizeof(action));
          // changed as 4.6 compiler bug! CoinMemcpyN(actions,nactions,temp);
          delete [] actions;
          actions=temp;
        }

	action *s = &actions[nactions];
	nactions++;

	s->col = iCol;
	s->clo = clo[iCol];
	s->cup = cup[iCol];

	s->row = iRow;
	s->rlo = rlo[iRow];
	s->rup = rup[iRow];

	s->coeff = coeff;

        presolve_delete_from_row(iRow,iCol,mrstrt,hinrow,hcol,rowels) ;
        if (!hinrow[iRow])
          PRESOLVE_REMOVE_LINK(prob->rlink_,iRow) ;
        // put row on stack of things to do next time
        prob->addRow(iRow);
#ifdef PRINTCOST
        if (rowObjective&&dcost[iCol]) {
          printf("Singleton %d had coeff of %g in row %d - bounds %g %g - cost %g\n",
                 iCol,coeff,iRow,clo[iCol],cup[iCol],dcost[iCol]);
          printf("Row bounds were %g %g now %g %g\n",
                 rlo[iRow],rup[iRow],newLower,newUpper);
        }
#endif
        // Row may be redundant but let someone else do that
        rlo[iRow]=newLower;
        rup[iRow]=newUpper;
        if (rowstat&&sol) {
          // update solution and basis
          if ((sol[iCol] < cup[iCol]-ztolzb&&
	       sol[iCol] > clo[iCol]+ztolzb)||prob->columnIsBasic(iCol))
            prob->setRowStatus(iRow,CoinPrePostsolveMatrix::basic);
          prob->setColumnStatusUsingValue(iCol);
        }
        // Force column to zero
        clo[iCol]=0.0;
        cup[iCol]=0.0;
        if (rowObjective&&dcost[iCol]) {
          rowObjective[iRow]=-dcost[iCol]/coeff;
            nWithCosts++;
          // adjust offset
          costOffset += currentLower*rowObjective[iRow];
          prob->dobias_ -= currentLower*rowObjective[iRow];
        }
	if (sol) {
	  double movement;
	  if (fabs(sol[iCol]-clo[iCol])<fabs(sol[iCol]-cup[iCol])) {
	    movement = clo[iCol]-sol[iCol] ;
	    sol[iCol]=clo[iCol];
	  } else {
	    movement = cup[iCol]-sol[iCol] ;
	    sol[iCol]=cup[iCol];
	  }
	  if (movement)
	    acts[iRow] += movement*coeff;
	}
        /*
          Remove the row from this col in the col rep.and delink it.
        */
        presolve_delete_from_col(iRow,iCol,mcstrt,hincol,hrow,colels) ;
        assert (hincol[iCol] == 0);
        PRESOLVE_REMOVE_LINK(prob->clink_,iCol) ;
	//clo[iCol] = 0.0;
	//cup[iCol] = 0.0;
	fixed_cols[nfixed_cols++] = iCol;
        //presolve_consistent(prob);
      }
    }
  }

  if (nactions) {
#   if PRESOLVE_SUMMARY
    printf("SINGLETON COLS:  %d\n", nactions);
#   endif
#ifdef COIN_DEVELOP
    printf("%d singletons, %d with costs - offset %g\n",nactions,
           nWithCosts, costOffset);
#endif
    action *save_actions = new action[nactions];
    CoinMemcpyN(actions, nactions, save_actions);
    next = new slack_singleton_action(nactions, save_actions, next);

    if (nfixed_cols)
      next = make_fixed_action::presolve(prob, fixed_cols, nfixed_cols,
					 true, // arbitrary
					 next);
  }
  delete [] actions;
  delete [] fixed_cols;
  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    int droppedRows = prob->countEmptyRows() - startEmptyRows ;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
    printf("CoinPresolveSingleton(3) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_);
  }

# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
  std::cout << "Leaving slack_singleton_action::presolve." << std::endl ;
# endif

  return (next);
}

void slack_singleton_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;
  //  int ncols		= prob->ncols_;

  //double *rowels	= prob->rowels_;
  //int *hcol	= prob->hcol_;
  //CoinBigIndex *mrstrt	= prob->mrstrt_;
  //int *hinrow		= prob->hinrow_;

  double *clo		= prob->clo_;
  double *cup		= prob->cup_;

  double *rlo		= prob->rlo_;
  double *rup		= prob->rup_;

  double *sol		= prob->sol_;
  double *rcosts	= prob->rcosts_;

  double *acts		= prob->acts_;
  double *rowduals 	= prob->rowduals_;
  double *dcost	= prob->cost_;
  //const double maxmin	= prob->maxmin_;

  unsigned char *colstat		= prob->colstat_;
  //  unsigned char *rowstat		= prob->rowstat_;

# if PRESOLVE_DEBUG
  char *rdone		= prob->rdone_;

  std::cout << "Entering slack_singleton_action::postsolve." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  CoinBigIndex &free_list		= prob->free_list_;

  const double ztolzb	= prob->ztolzb_;
#ifdef CHECK_ONE_ROW
  {
    double act=0.0;
    for (int i=0;i<prob->ncols_;i++) {
      double solV = sol[i];
      assert (solV>=clo[i]-ztolzb&&solV<=cup[i]+ztolzb);
      int j=mcstrt[i];
      for (int k=0;k<hincol[i];k++) {
	if (hrow[j]==CHECK_ONE_ROW) {
	  act += colels[j]*solV;
	}
	j=link[j];
      }
    }
    assert (act>=rlo[CHECK_ONE_ROW]-ztolzb&&act<=rup[CHECK_ONE_ROW]+ztolzb);
    printf("start %g %g %g %g\n",rlo[CHECK_ONE_ROW],act,acts[CHECK_ONE_ROW],rup[CHECK_ONE_ROW]);
  }
#endif
  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int iRow = f->row;
    double lo0 = f->clo;
    double up0 = f->cup;
    double coeff = f->coeff;
    int iCol = f->col;
    assert (!hincol[iCol]);
#ifdef CHECK_ONE_ROW
    if (iRow==CHECK_ONE_ROW)
      printf("Col %d coeff %g old bounds %g,%g new %g,%g - new rhs %g,%g - act %g\n",
	     iCol,coeff,clo[iCol],cup[iCol],lo0,up0,f->rlo,f->rup,acts[CHECK_ONE_ROW]);
#endif
    rlo[iRow] = f->rlo;
    rup[iRow] = f->rup;

    clo[iCol] = lo0;
    cup[iCol] = up0;
    double movement=0.0;
    // acts was without coefficient - adjust
    acts[iRow] += coeff*sol[iCol];
    if (acts[iRow]<rlo[iRow]-ztolzb)
      movement = rlo[iRow]-acts[iRow];
    else if (acts[iRow]>rup[iRow]+ztolzb)
      movement = rup[iRow]-acts[iRow];
    double cMove = movement/coeff;
    sol[iCol] += cMove;
    acts[iRow] += movement;
    if (!dcost[iCol]) {
      // and to get column feasible
      cMove=0.0;
      if (sol[iCol]>cup[iCol]+ztolzb)
        cMove = cup[iCol]-sol[iCol];
      else if (sol[iCol]<clo[iCol]-ztolzb)
        cMove = clo[iCol]-sol[iCol];
      sol[iCol] += cMove;
      acts[iRow] += cMove*coeff;
      /*
       * Have to compute status.  At most one can be basic. It's possible that
	 both are nonbasic and nonbasic status must change.
       */
      if (colstat) {
        int numberBasic =0;
        if (prob->columnIsBasic(iCol))
          numberBasic++;
        if (prob->rowIsBasic(iRow))
          numberBasic++;
#ifdef COIN_DEVELOP
        if (numberBasic>1)
          printf("odd in singleton\n");
#endif
        if (sol[iCol]>clo[iCol]+ztolzb&&sol[iCol]<cup[iCol]-ztolzb) {
          prob->setColumnStatus(iCol,CoinPrePostsolveMatrix::basic);
          prob->setRowStatusUsingValue(iRow);
        } else if (acts[iRow]>rlo[iRow]+ztolzb&&acts[iRow]<rup[iRow]-ztolzb) {
          prob->setRowStatus(iRow,CoinPrePostsolveMatrix::basic);
          prob->setColumnStatusUsingValue(iCol);
        } else if (numberBasic) {
          prob->setRowStatus(iRow,CoinPrePostsolveMatrix::basic);
          prob->setColumnStatusUsingValue(iCol);
        } else {
          prob->setRowStatusUsingValue(iRow);
          prob->setColumnStatusUsingValue(iCol);
	}
      }
#     if PRESOLVE_DEBUG
      printf("SLKSING: %d = %g restored %d lb = %g ub = %g.\n",
	     iCol,sol[iCol],prob->getColumnStatus(iCol),clo[iCol],cup[iCol]) ;
#     endif
    } else {
      // must have been equality row
      assert (rlo[iRow]==rup[iRow]);
      double cost = rcosts[iCol];
      // adjust for coefficient
      cost -= rowduals[iRow]*coeff;
      bool basic=true;
      if (fabs(sol[iCol]-cup[iCol])<ztolzb&&cost<-1.0e-6)
        basic=false;
      else if (fabs(sol[iCol]-clo[iCol])<ztolzb&&cost>1.0e-6)
        basic=false;
      //printf("Singleton %d had coeff of %g in row %d (dual %g) - bounds %g %g - cost %g, (dj %g)\n",
      //     iCol,coeff,iRow,rowduals[iRow],clo[iCol],cup[iCol],dcost[iCol],rcosts[iCol]);
      //if (prob->columnIsBasic(iCol))
      //printf("column basic! ");
      //if (prob->rowIsBasic(iRow))
      //printf("row basic ");
      //printf("- make column basic %s\n",basic ? "yes" : "no");
      if (basic&&!prob->rowIsBasic(iRow)) {
#ifdef PRINTCOST
        printf("Singleton %d had coeff of %g in row %d (dual %g) - bounds %g %g - cost %g, (dj %g - new %g)\n",
               iCol,coeff,iRow,rowduals[iRow],clo[iCol],cup[iCol],dcost[iCol],rcosts[iCol],cost);
#endif
#ifdef COIN_DEVELOP
        if (prob->columnIsBasic(iCol))
          printf("column basic!\n");
#endif
        basic=false;
      }
      if (fabs(rowduals[iRow])>1.0e-6&&prob->rowIsBasic(iRow))
        basic=true;
      if (basic) {
        // Make basic have zero reduced cost
	rowduals[iRow] = rcosts[iCol] / coeff;
	rcosts[iCol] = 0.0;
      } else {
        rcosts[iCol]=cost;
        //rowduals[iRow]=0.0;
      }
      if (colstat) {
        if (basic) {
          if (!prob->rowIsBasic(iRow)) {
#if 0
            // find column in row
            int jCol=-1;
            //for (CoinBigIndex j=mrstrt[iRow];j<mrstrt
            for (int k=0;k<prob->ncols0_;k++) {
              CoinBigIndex j=mcstrt[k];
              for (int i=0;i<hincol[k];i++) {
                if (hrow[k]==iRow) {
                  break;
                }
                k=link[k];
              }
            }
#endif
          } else {
            prob->setColumnStatus(iCol,CoinPrePostsolveMatrix::basic);
          }
          prob->setRowStatusUsingValue(iRow);
        } else {
          //prob->setRowStatus(iRow,CoinPrePostsolveMatrix::basic);
          prob->setColumnStatusUsingValue(iCol);
        }
      }
    }
#if 0
    int nb=0;
    int kk;
    for (kk=0;kk<prob->nrows_;kk++)
      if (prob->rowIsBasic(kk))
        nb++;
    for (kk=0;kk<prob->ncols_;kk++)
      if (prob->columnIsBasic(kk))
        nb++;
    assert (nb==prob->nrows_);
#endif
    // add new element
    {
      CoinBigIndex k = free_list;
      assert(k >= 0 && k < prob->bulk0_) ;
      free_list = link[free_list];
      hrow[k] = iRow;
      colels[k] = coeff;
      link[k] = mcstrt[iCol];
      mcstrt[iCol] = k;
    }
    hincol[iCol]++;	// right?
#ifdef CHECK_ONE_ROW
    {
      double act=0.0;
      for (int i=0;i<prob->ncols_;i++) {
	double solV = sol[i];
	assert (solV>=clo[i]-ztolzb&&solV<=cup[i]+ztolzb);
	int j=mcstrt[i];
	for (int k=0;k<hincol[i];k++) {
	  if (hrow[j]==CHECK_ONE_ROW) {
	    //printf("c %d el %g sol %g old act %g new %g\n",
	    //   i,colels[j],solV,act, act+colels[j]*solV);
	    act += colels[j]*solV;
	  }
	  j=link[j];
	}
      }
      assert (act>=rlo[CHECK_ONE_ROW]-ztolzb&&act<=rup[CHECK_ONE_ROW]+ztolzb);
      printf("rhs now %g %g %g %g\n",rlo[CHECK_ONE_ROW],act,acts[CHECK_ONE_ROW],rup[CHECK_ONE_ROW]);
    }
#endif

#   if PRESOLVE_DEBUG
    rdone[iRow] = SLACK_SINGLETON;
#   endif
  }

# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
  std::cout << "Leaving slack_singleton_action::postsolve." << std::endl ;
# endif

  return ;
}
