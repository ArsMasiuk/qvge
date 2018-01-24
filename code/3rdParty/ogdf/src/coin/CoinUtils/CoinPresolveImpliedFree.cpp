/* $Id: CoinPresolveImpliedFree.cpp 1448 2011-06-19 15:34:41Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinPresolveIsolated.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveImpliedFree.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinPresolveForcing.hpp"
#include "CoinMessage.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

namespace {	// begin unnamed file-local namespace

const CoinPresolveAction *testRedundant (CoinPresolveMatrix *prob,
					 const CoinPresolveAction *next,
					 int & numberInfeasible)
{
  numberInfeasible=0;
  int numberColumns = prob->ncols_;
  double * columnLower	= new double[numberColumns];
  double * columnUpper	= new double[numberColumns];
  CoinMemcpyN(prob->clo_,numberColumns,columnLower);
  CoinMemcpyN(prob->cup_,numberColumns,columnUpper);

  const double *element	= prob->rowels_;
  const int *column	= prob->hcol_;
  const CoinBigIndex *rowStart	= prob->mrstrt_;
  const int *rowLength	= prob->hinrow_;
  int numberRows	= prob->nrows_;
  const int *hrow	= prob->hrow_;
  const CoinBigIndex *mcstrt	= prob->mcstrt_;
  const int *hincol	= prob->hincol_;

  int *useless_rows	= prob->usefulRowInt_+numberRows; //new int[numberRows];
  int nuseless_rows	= 0;

  double *rowLower	= prob->rlo_;
  double *rowUpper	= prob->rup_;

  double tolerance = prob->feasibilityTolerance_;
  int numberChanged=1,iPass=0;
#define USE_SMALL_LARGE
#ifdef USE_SMALL_LARGE
  double large = 1.0e15; // treat bounds > this as infinite
#else
  double large = 1.0e20; // treat bounds > this as infinite
#endif
#ifndef NDEBUG
  double large2= 1.0e10*large;
#endif
  int totalTightened = 0;

  int iRow, iColumn;

  char * markRow = reinterpret_cast<char *>(prob->usefulRowInt_); // wasnew int[numberRows];
  for (iRow=0;iRow<numberRows;iRow++) {
    if ((rowLower[iRow]>-large||rowUpper[iRow]<large)&&rowLength[iRow]>0) {
      markRow[iRow]=-1;
    } else {
      markRow[iRow]=1;
      if (rowLength[iRow]>0) {
	// Row is redundant
	useless_rows[nuseless_rows++] = iRow;
	prob->addRow(iRow);
      }
    }
  }
#define MAXPASS 10
  bool fixInfeasibility = (prob->presolveOptions_&16384)!=0;
  double relaxedTolerance = 100.0*tolerance;

  // Loop round seeing if we can tighten bounds
  // Would be faster to have a stack of possible rows
  // and we put altered rows back on stack
  int numberCheck=-1;
  while(numberChanged>numberCheck) {

    numberChanged = 0; // Bounds tightened this pass

    if (iPass==MAXPASS) break;
    iPass++;

    for (iRow = 0; iRow < numberRows; iRow++) {

      if (markRow[iRow]==-1) {

	// possible row - but mark as useless next time
	markRow[iRow]=-2;
	int infiniteUpper = 0;
	int infiniteLower = 0;
	double maximumUp = 0.0;
	double maximumDown = 0.0;
	double newBound;
	CoinBigIndex rStart = rowStart[iRow];
	CoinBigIndex rEnd = rowStart[iRow]+rowLength[iRow];
	CoinBigIndex j;
	// Compute possible lower and upper ranges

	for (j = rStart; j < rEnd; ++j) {
	  double value=element[j];
	  iColumn = column[j];
	  if (value > 0.0) {
	    if (columnUpper[iColumn] < large)
	      maximumUp += columnUpper[iColumn] * value;
	    else
	      ++infiniteUpper;
	    if (columnLower[iColumn] > -large)
	      maximumDown += columnLower[iColumn] * value;
	    else
	      ++infiniteLower;
	  } else if (value<0.0) {
	    if (columnUpper[iColumn] < large)
	      maximumDown += columnUpper[iColumn] * value;
	    else
	      ++infiniteLower;
	    if (columnLower[iColumn] > -large)
	      maximumUp += columnLower[iColumn] * value;
	    else
	      ++infiniteUpper;
	  }
	}
	// Build in a margin of error
	maximumUp += 1.0e-8*fabs(maximumUp);
	maximumDown -= 1.0e-8*fabs(maximumDown);
	double maxUp = maximumUp+infiniteUpper*1.0e31;
	double maxDown = maximumDown-infiniteLower*1.0e31;
	if (maxUp <= rowUpper[iRow] + tolerance &&
	    maxDown >= rowLower[iRow] - tolerance) {

	} else {
	  if (maxUp < rowLower[iRow] -relaxedTolerance ||
	      maxDown > rowUpper[iRow]+relaxedTolerance) {
            if(!fixInfeasibility) {
              // problem is infeasible - exit at once
              numberInfeasible++;
              prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
                                              prob->messages())
                                                <<iRow
                                                <<rowLower[iRow]
                                                <<rowUpper[iRow]
                                                <<CoinMessageEol;
              break;
            } else {
              continue;
            }
	  }
	  double lower = rowLower[iRow];
	  double upper = rowUpper[iRow];
	  // Clean up
	  if (maximumUp < lower && maximumUp > lower -relaxedTolerance)
	    maximumUp=lower;
	  if (maximumDown > upper && maximumDown < upper +relaxedTolerance)
	    maximumDown=upper;
	  for (j = rStart; j < rEnd; ++j) {
	    double value=element[j];
	    iColumn = column[j];
	    double nowLower = columnLower[iColumn];
	    double nowUpper = columnUpper[iColumn];
	    if (value > 0.0) {
	      // positive value
	      if (lower>-large) {
		if (!infiniteUpper) {
		  assert(nowUpper < large2);
		  newBound = nowUpper +
		    (lower - maximumUp) / value;
		  // relax if original was large
		  if (fabs(maximumUp)>1.0e8)
		    newBound -= 1.0e-12*fabs(maximumUp);
		} else if (infiniteUpper==1&&nowUpper>=large) {
		  newBound = (lower -maximumUp) / value;
		  // relax if original was large
		  if (fabs(maximumUp)>1.0e8)
		    newBound -= 1.0e-12*fabs(maximumUp);
		} else {
		  newBound = -COIN_DBL_MAX;
		}
		if (newBound > nowLower + 1.0e-12&&newBound>-large) {
		  // Tighten the lower bound
		  columnLower[iColumn] = newBound;
		  markRow[iRow]=1;
		  numberChanged++;
		  // Mark rows as possible
		  CoinBigIndex kcs = mcstrt[iColumn];
		  CoinBigIndex kce = kcs + hincol[iColumn];
		  CoinBigIndex k;
		  for (k=kcs; k<kce; ++k) {
		    int row = hrow[k];
		    if (markRow[row]==-2) {
		      // on list for next time
		      markRow[row]=-1;
		    }
		  }
		  // check infeasible (relaxed)
		  if (nowUpper - newBound <
		      -relaxedTolerance) {
		    numberInfeasible++;
		  }
		  // adjust
		  double now;
		  if (nowLower<=-large) {
		    now=0.0;
		    infiniteLower--;
		  } else {
		    now = nowLower;
		  }
		  maximumDown += (newBound-now) * value;
		  nowLower = newBound;
		  //#define FREE_DEBUG 2
#if FREE_DEBUG >1
		  if (fabs((newBound-now)*value)>1.0e8) {
		    // recompute
		    infiniteLower = 0;
		    maximumDown = 0.0;
		    CoinBigIndex j2;
		    // Compute possible lower and upper ranges
		    for (j2 = rStart; j2 < rEnd; ++j2) {
		      double value=element[j2];
		      int iColumn = column[j2];
		      if (value > 0.0) {
			if (columnLower[iColumn] > -large)
			  maximumDown += columnLower[iColumn] * value;
			else
			  ++infiniteLower;
		      } else if (value<0.0) {
			if (columnUpper[iColumn] < large)
			  maximumDown += columnUpper[iColumn] * value;
			else
			  ++infiniteLower;
		      }
		    }
		    // Build in a margin of error
		    maximumDown -= 1.0e-8*fabs(maximumDown);
		    if (maximumDown > upper && maximumDown < upper +relaxedTolerance)
		      maximumDown=upper;
		  }
#endif
#if FREE_DEBUG
#define DEBUG_TOLERANCE 1.0e-10
		  { // DEBUG
		    int infiniteUpper2 = 0;
		    int infiniteLower2 = 0;
		    double maximumUp2 = 0.0;
		    double maximumDown2 = 0.0;
		    CoinBigIndex j2;
		    // Compute possible lower and upper ranges
		    for (j2 = rStart; j2 < rEnd; ++j2) {
		      double value=element[j2];
		      int iColumn = column[j2];
		      if (value > 0.0) {
			if (columnUpper[iColumn] < large)
			  maximumUp2 += columnUpper[iColumn] * value;
			else
			  ++infiniteUpper2;
			if (columnLower[iColumn] > -large)
			  maximumDown2 += columnLower[iColumn] * value;
			else
			  ++infiniteLower2;
		      } else if (value<0.0) {
			if (columnUpper[iColumn] < large)
			  maximumDown2 += columnUpper[iColumn] * value;
			else
			  ++infiniteLower2;
			if (columnLower[iColumn] > -large)
			  maximumUp2 += columnLower[iColumn] * value;
			else
			  ++infiniteUpper2;
		      }
		    }
		    // Build in a margin of error
		    maximumUp2 += 1.0e-8*fabs(maximumUp2);
		    maximumDown2 -= 1.0e-8*fabs(maximumDown2);
		    if (maximumUp2 < lower && maximumUp2 > lower -relaxedTolerance)
		      maximumUp2=lower;
		    if (maximumDown2 > upper && maximumDown2 < upper +relaxedTolerance)
		      maximumDown2=upper;
		    assert (infiniteLower==infiniteLower2);
		    assert (infiniteUpper==infiniteUpper2);
		    assert (fabs(maximumDown-maximumDown2)<DEBUG_TOLERANCE*(1.0e6+fabs(maximumDown)));
		    assert (fabs(maximumUp-maximumUp2)<DEBUG_TOLERANCE*(1.0e6+fabs(maximumUp)));
		  } // END DEBUG
#endif
		}
	      }
	      if (upper <large) {
		if (!infiniteLower) {
		  assert(nowLower >- large2);
		  newBound = nowLower +
		    (upper - maximumDown) / value;
		  // relax if original was large
		  if (fabs(maximumDown)>1.0e8)
		    newBound += 1.0e-12*fabs(maximumDown);
		} else if (infiniteLower==1&&nowLower<=-large) {
		  newBound =   (upper - maximumDown) / value;
		  // relax if original was large
		  if (fabs(maximumDown)>1.0e8)
		    newBound += 1.0e-12*fabs(maximumDown);
		} else {
		  newBound = COIN_DBL_MAX;
		}
		if (newBound < nowUpper - 1.0e-12&&newBound<large) {
		  // Tighten the upper bound
		  columnUpper[iColumn] = newBound;
		  markRow[iRow]=1;
		  numberChanged++;
		  // Mark rows as possible
		  CoinBigIndex kcs = mcstrt[iColumn];
		  CoinBigIndex kce = kcs + hincol[iColumn];
		  CoinBigIndex k;
		  for (k=kcs; k<kce; ++k) {
		    int row = hrow[k];
		    if (markRow[row]==-2) {
		      // on list for next time
		      markRow[row]=-1;
		    }
		  }
		  // check infeasible (relaxed)
		  if (newBound - nowLower <
		      -relaxedTolerance) {
		    numberInfeasible++;
		  }
		  // adjust
		  double now;
		  if (nowUpper>=large) {
		    now=0.0;
		    infiniteUpper--;
		  } else {
		    now = nowUpper;
		  }
		  maximumUp += (newBound-now) * value;
		  nowUpper = newBound;
#if FREE_DEBUG >1
		  if (fabs((newBound-now)*value)>1.0e8) {
		    // recompute
		    infiniteUpper = 0;
		    maximumUp = 0.0;
		    CoinBigIndex j2;
		    // Compute possible lower and upper ranges
		    for (j2 = rStart; j2 < rEnd; ++j2) {
		      double value=element[j2];
		      int iColumn = column[j2];
		      if (value > 0.0) {
			if (columnUpper[iColumn] < large)
			  maximumUp += columnUpper[iColumn] * value;
			else
			  ++infiniteUpper;
		      } else if (value<0.0) {
			if (columnLower[iColumn] > -large)
			  maximumUp += columnLower[iColumn] * value;
			else
			  ++infiniteUpper;
		      }
		    }
		    // Build in a margin of error
		    maximumUp += 1.0e-8*fabs(maximumUp);
		    if (maximumUp < lower && maximumUp > lower -relaxedTolerance)
		      maximumUp=lower;
		  }
#endif
#if FREE_DEBUG
		  { // DEBUG
		    int infiniteUpper2 = 0;
		    int infiniteLower2 = 0;
		    double maximumUp2 = 0.0;
		    double maximumDown2 = 0.0;
		    CoinBigIndex j2;
		    // Compute possible lower and upper ranges
		    for (j2 = rStart; j2 < rEnd; ++j2) {
		      double value=element[j2];
		      int iColumn = column[j2];
		      if (value > 0.0) {
			if (columnUpper[iColumn] < large)
			  maximumUp2 += columnUpper[iColumn] * value;
			else
			  ++infiniteUpper2;
			if (columnLower[iColumn] > -large)
			  maximumDown2 += columnLower[iColumn] * value;
			else
			  ++infiniteLower2;
		      } else if (value<0.0) {
			if (columnUpper[iColumn] < large)
			  maximumDown2 += columnUpper[iColumn] * value;
			else
			  ++infiniteLower2;
			if (columnLower[iColumn] > -large)
			  maximumUp2 += columnLower[iColumn] * value;
			else
			  ++infiniteUpper2;
		      }
		    }
		    // Build in a margin of error
		    maximumUp2 += 1.0e-8*fabs(maximumUp2);
		    maximumDown2 -= 1.0e-8*fabs(maximumDown2);
		    if (maximumUp2 < lower && maximumUp2 > lower -relaxedTolerance)
		      maximumUp2=lower;
		    if (maximumDown2 > upper && maximumDown2 < upper +relaxedTolerance)
		      maximumDown2=upper;
		    assert (infiniteLower==infiniteLower2);
		    assert (infiniteUpper==infiniteUpper2);
		    assert (fabs(maximumDown-maximumDown2)<DEBUG_TOLERANCE*(1.0e6+fabs(maximumDown)));
		    assert (fabs(maximumUp-maximumUp2)<DEBUG_TOLERANCE*(1.0e6+fabs(maximumUp)));
		  } // END DEBUG
#endif
		}
	      }
	    } else {
	      // negative value
	      if (lower>-large) {
		if (!infiniteUpper) {
		  assert(nowLower < large2);
		  newBound = nowLower +
		    (lower - maximumUp) / value;
		  // relax if original was large
		  if (fabs(maximumUp)>1.0e8)
		    newBound += 1.0e-12*fabs(maximumUp);
		} else if (infiniteUpper==1&&nowLower<=-large) {
		  newBound = (lower -maximumUp) / value;
		  // relax if original was large
		  if (fabs(maximumUp)>1.0e8)
		    newBound += 1.0e-12*fabs(maximumUp);
		} else {
		  newBound = COIN_DBL_MAX;
		}
		if (newBound < nowUpper - 1.0e-12&&newBound<large) {
		  // Tighten the upper bound
		  columnUpper[iColumn] = newBound;
		  markRow[iRow]=1;
		  numberChanged++;
		  // Mark rows as possible
		  CoinBigIndex kcs = mcstrt[iColumn];
		  CoinBigIndex kce = kcs + hincol[iColumn];
		  CoinBigIndex k;
		  for (k=kcs; k<kce; ++k) {
		    int row = hrow[k];
		    if (markRow[row]==-2) {
		      // on list for next time
		      markRow[row]=-1;
		    }
		  }
		  // check infeasible (relaxed)
		  if (newBound - nowLower <
		      -relaxedTolerance) {
		    numberInfeasible++;
		  }
		  // adjust
		  double now;
		  if (nowUpper>=large) {
		    now=0.0;
		    infiniteLower--;
		  } else {
		    now = nowUpper;
		  }
		  maximumDown += (newBound-now) * value;
		  nowUpper = newBound;
#if FREE_DEBUG >1
		  if (fabs((newBound-now)*value)>1.0e8) {
		    // recompute
		    infiniteLower = 0;
		    maximumDown = 0.0;
		    CoinBigIndex j2;
		    // Compute possible lower and upper ranges
		    for (j2 = rStart; j2 < rEnd; ++j2) {
		      double value=element[j2];
		      int iColumn = column[j2];
		      if (value > 0.0) {
			if (columnLower[iColumn] > -large)
			  maximumDown += columnLower[iColumn] * value;
			else
			  ++infiniteLower;
		      } else if (value<0.0) {
			if (columnUpper[iColumn] < large)
			  maximumDown += columnUpper[iColumn] * value;
			else
			  ++infiniteLower;
		      }
		    }
		    // Build in a margin of error
		    maximumDown -= 1.0e-8*fabs(maximumDown);
		    if (maximumDown > upper && maximumDown < upper +relaxedTolerance)
		      maximumDown=upper;
		  }
#endif
#if FREE_DEBUG
		  { // DEBUG
		    int infiniteUpper2 = 0;
		    int infiniteLower2 = 0;
		    double maximumUp2 = 0.0;
		    double maximumDown2 = 0.0;
		    CoinBigIndex j2;
		    // Compute possible lower and upper ranges
		    for (j2 = rStart; j2 < rEnd; ++j2) {
		      double value=element[j2];
		      int iColumn = column[j2];
		      if (value > 0.0) {
			if (columnUpper[iColumn] < large)
			  maximumUp2 += columnUpper[iColumn] * value;
			else
			  ++infiniteUpper2;
			if (columnLower[iColumn] > -large)
			  maximumDown2 += columnLower[iColumn] * value;
			else
			  ++infiniteLower2;
		      } else if (value<0.0) {
			if (columnUpper[iColumn] < large)
			  maximumDown2 += columnUpper[iColumn] * value;
			else
			  ++infiniteLower2;
			if (columnLower[iColumn] > -large)
			  maximumUp2 += columnLower[iColumn] * value;
			else
			  ++infiniteUpper2;
		      }
		    }
		    // Build in a margin of error
		    maximumUp2 += 1.0e-8*fabs(maximumUp2);
		    maximumDown2 -= 1.0e-8*fabs(maximumDown2);
		    if (maximumUp2 < lower && maximumUp2 > lower -relaxedTolerance)
		      maximumUp2=lower;
		    if (maximumDown2 > upper && maximumDown2 < upper +relaxedTolerance)
		      maximumDown2=upper;
		    assert (infiniteLower==infiniteLower2);
		    assert (infiniteUpper==infiniteUpper2);
		    assert (fabs(maximumDown-maximumDown2)<DEBUG_TOLERANCE*(1.0e6+fabs(maximumDown)));
		    assert (fabs(maximumUp-maximumUp2)<DEBUG_TOLERANCE*(1.0e6+fabs(maximumUp)));
		  } // END DEBUG
#endif
		}
	      }
	      if (upper <large) {
		if (!infiniteLower) {
		  assert(nowUpper < large2);
		  newBound = nowUpper +
		    (upper - maximumDown) / value;
		  // relax if original was large
		  if (fabs(maximumDown)>1.0e8)
		    newBound -= 1.0e-12*fabs(maximumDown);
		} else if (infiniteLower==1&&nowUpper>=large) {
		  newBound =   (upper - maximumDown) / value;
		  // relax if original was large
		  if (fabs(maximumDown)>1.0e8)
		    newBound -= 1.0e-12*fabs(maximumDown);
		} else {
		  newBound = -COIN_DBL_MAX;
		}
		if (newBound > nowLower + 1.0e-12&&newBound>-large) {
		  // Tighten the lower bound
		  columnLower[iColumn] = newBound;
		  markRow[iRow]=1;
		  numberChanged++;
		  // Mark rows as possible
		  CoinBigIndex kcs = mcstrt[iColumn];
		  CoinBigIndex kce = kcs + hincol[iColumn];
		  CoinBigIndex k;
		  for (k=kcs; k<kce; ++k) {
		    int row = hrow[k];
		    if (markRow[row]==-2) {
		      // on list for next time
		      markRow[row]=-1;
		    }
		  }
		  // check infeasible (relaxed)
		  if (nowUpper - newBound <
		      -relaxedTolerance) {
		    numberInfeasible++;
		  }
		  // adjust
		  double now;
		  if (nowLower<=-large) {
		    now=0.0;
		    infiniteUpper--;
		  } else {
		    now = nowLower;
		  }
		  maximumUp += (newBound-now) * value;
		  nowLower = newBound;
#if FREE_DEBUG >1
		  if (fabs((newBound-now)*value)>1.0e8) {
		    // recompute
		    infiniteUpper = 0;
		    maximumUp = 0.0;
		    CoinBigIndex j2;
		    // Compute possible lower and upper ranges
		    for (j2 = rStart; j2 < rEnd; ++j2) {
		      double value=element[j2];
		      int iColumn = column[j2];
		      if (value > 0.0) {
			if (columnUpper[iColumn] < large)
			  maximumUp += columnUpper[iColumn] * value;
			else
			  ++infiniteUpper;
		      } else if (value<0.0) {
			if (columnLower[iColumn] > -large)
			  maximumUp += columnLower[iColumn] * value;
			else
			  ++infiniteUpper;
		      }
		    }
		    // Build in a margin of error
		    maximumUp += 1.0e-8*fabs(maximumUp);
		    if (maximumUp < lower && maximumUp > lower -relaxedTolerance)
		      maximumUp=lower;
		  }
#endif
#if FREE_DEBUG
		  { // DEBUG
		    int infiniteUpper2 = 0;
		    int infiniteLower2 = 0;
		    double maximumUp2 = 0.0;
		    double maximumDown2 = 0.0;
		    CoinBigIndex j2;
		    // Compute possible lower and upper ranges
		    for (j2 = rStart; j2 < rEnd; ++j2) {
		      double value=element[j2];
		      int iColumn = column[j2];
		      if (value > 0.0) {
			if (columnUpper[iColumn] < large)
			  maximumUp2 += columnUpper[iColumn] * value;
			else
			  ++infiniteUpper2;
			if (columnLower[iColumn] > -large)
			  maximumDown2 += columnLower[iColumn] * value;
			else
			  ++infiniteLower2;
		      } else if (value<0.0) {
			if (columnUpper[iColumn] < large)
			  maximumDown2 += columnUpper[iColumn] * value;
			else
			  ++infiniteLower2;
			if (columnLower[iColumn] > -large)
			  maximumUp2 += columnLower[iColumn] * value;
			else
			  ++infiniteUpper2;
		      }
		    }
		    // Build in a margin of error
		    maximumUp2 += 1.0e-8*fabs(maximumUp2);
		    maximumDown2 -= 1.0e-8*fabs(maximumDown2);
		    if (maximumUp2 < lower && maximumUp2 > lower -relaxedTolerance)
		      maximumUp2=lower;
		    if (maximumDown2 > upper && maximumDown2 < upper +relaxedTolerance)
		      maximumDown2=upper;
		    assert (infiniteLower==infiniteLower2);
		    assert (infiniteUpper==infiniteUpper2);
		    assert (fabs(maximumDown-maximumDown2)<DEBUG_TOLERANCE*(1.0e6+fabs(maximumDown)));
		    assert (fabs(maximumUp-maximumUp2)<DEBUG_TOLERANCE*(1.0e6+fabs(maximumUp)));
		  } // END DEBUG
#endif
		}
	      }
	    }
	  }
	}
      }
    }
    totalTightened += numberChanged;
    if (iPass==1)
      numberCheck=CoinMax(10,numberChanged>>5);
    if (numberInfeasible) break;
  }
  if (!numberInfeasible) {
    //#define NO_FORCING
#ifdef NO_FORCING
#define ZZ 1
#ifdef ZZ
    double * clo2 = CoinCopyOfArray(prob->clo_,numberColumns);
    double * cup2 = CoinCopyOfArray(prob->cup_,numberColumns);
#endif
    forcing_constraint_action::action * actions = NULL;
    int numActions=0;
    int maxActions=0;
    double * clo = prob->clo_;
    double * cup = prob->cup_;
    double *csol  = prob->sol_ ;
    int * fixed = prob->usefulColumnInt_;
    for (int i=0;i<numberColumns;i++) {
      if (cup[i]>clo[i]+tolerance)
	fixed[i]=0;
      else
	fixed[i]=-1;
    }
#endif
    for (iRow = 0; iRow < numberRows; iRow++) {

      if (markRow[iRow]<0) {

	// possible row
	int infiniteUpper = 0;
	int infiniteLower = 0;
	double maximumUp = 0.0;
	double maximumDown = 0.0;
	CoinBigIndex rStart = rowStart[iRow];
	CoinBigIndex rEnd = rowStart[iRow]+rowLength[iRow];
	CoinBigIndex j;
	// Compute possible lower and upper ranges

	for (j = rStart; j < rEnd; ++j) {
	  double value=element[j];
	  iColumn = column[j];
	  if (value > 0.0) {
	    if (columnUpper[iColumn] < large)
	      maximumUp += columnUpper[iColumn] * value;
	    else
	      ++infiniteUpper;
	    if (columnLower[iColumn] > -large)
	      maximumDown += columnLower[iColumn] * value;
	    else
	      ++infiniteLower;
	  } else if (value<0.0) {
	    if (columnUpper[iColumn] < large)
	      maximumDown += columnUpper[iColumn] * value;
	    else
	      ++infiniteLower;
	    if (columnLower[iColumn] > -large)
	      maximumUp += columnLower[iColumn] * value;
	    else
	      ++infiniteUpper;
	  }
	}
	// Build in a margin of error
	maximumUp += 1.0e-8*fabs(maximumUp);
	maximumDown -= 1.0e-8*fabs(maximumDown);
	double maxUp = maximumUp+infiniteUpper*1.0e31;
	double maxDown = maximumDown-infiniteLower*1.0e31;
	if (maxUp <= rowUpper[iRow] + tolerance &&
	    maxDown >= rowLower[iRow] - tolerance) {
#ifndef NO_FORCING
	  // Row is redundant
	  useless_rows[nuseless_rows++] = iRow;
	  prob->addRow(iRow);
#else
	  if (maxUp <= rowUpper[iRow] - tolerance &&
	      maxDown >= rowLower[iRow] + tolerance) {
	    // Row is redundant
	    useless_rows[nuseless_rows++] = iRow;
	    prob->addRow(iRow);
	  } else {
	    CoinBigIndex k;
	    double tol2 = 10.0*tolerance;
	    int nFree=0;
	    bool bad=false;
	    for ( k=rStart; k<rEnd; k++) {
	      int jcol = column[k];
	      if (columnUpper[jcol]-columnLower[jcol]>tol2)
		nFree++;
	      if (fixed[jcol]>0)
		bad=true;
	    }
	    if (nFree&&!bad) {
	      // the lower bound can just be reached, or
	      // the upper bound can just be reached;
	      // called a "forcing constraint" in the paper (p. 226)
	      const int lbound_tight = (maxUp < PRESOLVE_INF &&
					fabs(rowLower[iRow] - maxUp) < tolerance);
	      double *bounds = new double[rowLength[iRow]];
	      int *rowcols = new int[rowLength[iRow]];
	      int lk = rStart;	// load fix-to-down in front
	      int uk = rEnd;	// load fix-to-up in back
	      for ( k=rStart; k<rEnd; k++) {
		int jcol = column[k];
		if (!fixed[jcol])
		  fixed[jcol]=1;
		prob->addCol(jcol);
		double coeff = element[k];
		// one of the two contributed to maxup - set the other to that
		if (lbound_tight == (coeff > 0.0)) {
		  --uk;
		  bounds[uk-rStart] = clo[jcol];
		  rowcols[uk-rStart] = jcol;
		  if (csol != 0) {
		    csol[jcol] = columnUpper[jcol] ;
		  }
#ifdef ZZ
		  assert (columnLower[jcol]==clo[jcol]);
		  clo[jcol] = columnUpper[jcol];
#endif
		  columnLower[jcol] = columnUpper[jcol];
		} else {
		  bounds[lk-rStart] = cup[jcol];
		  rowcols[lk-rStart] = jcol;
		  ++lk;
		  if (csol != 0) {
		    csol[jcol] = columnLower[jcol] ;
		  }
#ifdef ZZ
		  assert (columnUpper[jcol]==cup[jcol]);
		  cup[jcol] = columnLower[jcol];
#endif
		  columnUpper[jcol] = columnLower[jcol];
		}
	      }
	      PRESOLVEASSERT(uk == lk);
	      if (numActions==maxActions) {
		maxActions = (11*maxActions)/10+10;
		forcing_constraint_action::action * temp =
		  new forcing_constraint_action::action [maxActions];
		for (int i=0;i<numActions;i++)
		  temp[i]=actions[i];
		delete [] actions;
		actions = temp;
	      }

	      forcing_constraint_action::action * f = &actions[numActions];
	      numActions++;
	      PRESOLVE_DETAIL_PRINT(printf("pre_impliedfree %dR E\n",iRow));
	      f->row = iRow;
	      f->nlo = lk-rStart;
	      f->nup = rEnd-uk;
	      f->rowcols = rowcols;
	      f->bounds = bounds;
	    } else if (!nFree&&!bad) {
	      // Row is redundant
	      useless_rows[nuseless_rows++] = iRow;
	      prob->addRow(iRow);
	    }
	  }
#endif
	}
      }
    }
#ifdef NO_FORCING
    if (numActions) {
#if	PRESOLVE_SUMMARY
      printf("NFORCED_FROM_IMPLIED:  %d\n", numActions);
#endif
      next = new forcing_constraint_action(numActions,
      				   CoinCopyOfArray(actions,numActions), next);
      deleteAction(actions,action*);
      actions=NULL;
    }
    assert (!actions);
#endif
    if (nuseless_rows) {
      next = useless_constraint_action::presolve(prob,
						 useless_rows, nuseless_rows,
						 next);
    }
#ifdef NO_FORCING
#ifdef ZZ
    // temp
    for (int i=0;i<numberColumns;i++) {
      clo[i]=clo2[i];
      cup[i]=cup2[i];
    }
#endif
    //delete [] clo2;
    //delete [] cup2;
    if (numActions) {
      //int * fixed = prob->usefulColumnInt_;
      // See if any columns fixed
      int nFixed=0;
      for (int i=0;i<numberColumns;i++) {
	//if (columnUpper[i]<columnLower[i]+tolerance&&
	//  cup[i]>clo[i]+tolerance) {
	if (fixed[i]>0) {
	  fixed[nFixed++]=i;
	  assert (columnUpper[i]>columnLower[i]-10.0*tolerance);
	}
      }
      //if (nFixed)
      //printf("could fix %d\n",nFixed);
      //next = remove_fixed_action::presolve(prob,fixed,nFixed,next) ;
    }
#endif
    if ((prob->presolveOptions_&16)!=0) {
      // may not unroll
      const unsigned char *integerType = prob->integerType_;
      double *csol  = prob->sol_ ;
      double * clo = prob->clo_;
      double * cup = prob->cup_;
      int * fixed = prob->usefulColumnInt_;
      int nFixed=0;
      int nChanged=0;
      for (int i=0;i<numberColumns;i++) {
	if (clo[i]==cup[i])
	  continue;
	double lower = columnLower[i];
	double upper = columnUpper[i];
	if (integerType[i]) {
	  //if (floor(upper+1.0e-4)<upper)
	  upper=floor(upper+1.0e-4);
	  //if (ceil(lower-1.0e-4)>lower)
	  lower=ceil(lower-1.0e-4);
	}
	if (upper-lower<1.0e-8) {
	  if (upper-lower<-tolerance)
	    numberInfeasible++;
	  if (CoinMin(fabs(upper),fabs(lower))<=1.0e-7)
	    upper=0.0;
	  fixed[nFixed++]=i;
	  //printf("fixing %d to %g\n",i,upper);
	  prob->addCol(i);
	  cup[i]=upper;
	  clo[i]=upper;
	  if (csol != 0)
	    csol[i] = upper;
	} else {
	  if (integerType[i]) {
	    if (upper<cup[i]) {
	      cup[i]=upper;
	      nChanged++;
	      prob->addCol(i);
	    }
	    if (lower>clo[i]) {
	      clo[i]=lower;
	      nChanged++;
	      prob->addCol(i);
	    }
	  }
	}
      }
#ifdef CLP_INVESTIGATE
      if (nFixed||nChanged)
	printf("%d fixed in impliedfree, %d changed\n",nFixed,nChanged);
#endif
      if (nFixed)
	next = remove_fixed_action::presolve(prob,fixed,nFixed,next) ;
    }
  }

  delete [] columnLower;
  delete [] columnUpper;
  return next;
}

} // end unnamed file-local namespace

// If there is a row with a singleton column such that no matter what
// the values of the other variables are, the constraint forces the singleton
// column to have a feasible value, then we can drop the column and row,
// since we just compute the value of the column from the row in postsolve.
// This seems vaguely similar to the case of a useless constraint, but it
// is different.  For example, if the singleton column is already free,
// then this operation will eliminate it, but the constraint is not useless
// (assuming the constraint is not trivial), since the variables do not imply an
// upper or lower bound.
//
// If the column is not singleton, we can still do something similar if the
// constraint is an equality constraint.  In that case, we substitute away
// the variable in the other constraints it appears in.  This introduces
// new coefficients, but the total number of coefficients never increases
// if the column has only two constraints, and may not increase much even
// if there are more.
//
// There is nothing to prevent us from substituting away a variable
// in an equality from the other constraints it appears in, but since
// that causes fill-in, it wouldn't make sense unless we could then
// drop the equality itself.  We can't do that if the bounds on the
// variable in equation aren't implied by the equality.
// Another way of thinking of this is that there is nothing special
// about an equality; just like one can't always drop an inequality constraint
// with a column singleton, one can't always drop an equality.
//
// It is possible for two singleton columns to be in the same row.
// In that case, the other one will become empty.  If its bounds and
// costs aren't just right, this signals an unbounded problem.
// We don't need to check that specially here.
//
// invariant:  loosely packed
const CoinPresolveAction *implied_free_action::presolve(CoinPresolveMatrix *prob,
						     const CoinPresolveAction *next,
						    int & fill_level)
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
  int *hrow	= prob->hrow_;
  const CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;
  const int ncols	= prob->ncols_;

  const double *clo	= prob->clo_;
  const double *cup	= prob->cup_;

  const double *rowels	= prob->rowels_;
  const int *hcol	= prob->hcol_;
  const CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow	= prob->hinrow_;
  int nrows	= prob->nrows_;

  /*const*/ double *rlo	= prob->rlo_;
  /*const*/ double *rup	= prob->rup_;

  double *cost		= prob->cost_;

  presolvehlink *rlink = prob->rlink_;
  presolvehlink *clink = prob->clink_;

  const unsigned char *integerType = prob->integerType_;
  bool stopSomeStuff = (prob->presolveOptions()&4)!=0;

  const double tol = prob->feasibilityTolerance_;
#if 1
  // This needs to be made faster
  int numberInfeasible;
  //printf("Imp pass %d\n",prob->pass_);
#ifdef COIN_LIGHTWEIGHT_PRESOLVE
  if (prob->pass_==1) {
#endif
    next = testRedundant(prob,next,numberInfeasible);
    if ((prob->presolveOptions_&16384)!=0)
      numberInfeasible=0;
    if (numberInfeasible) {
      // infeasible
      prob->status_|= 1;
      return (next);
    }
#ifdef COIN_LIGHTWEIGHT_PRESOLVE
  }
#endif
  if (prob->pass_>15&&(prob->presolveOptions_&0x10000)!=0) {
    fill_level=2;
    return next;
  }
#endif
  //  int nbounds = 0;

  action *actions	= new action [ncols];
# ifdef ZEROFAULT
  CoinZeroN(reinterpret_cast<char *>(actions),ncols*sizeof(action)) ;
# endif
  int nactions = 0;
  bool fixInfeasibility = (prob->presolveOptions_&16384)!=0;

  int *implied_free = prob->usefulColumnInt_; //new int[ncols];
  int * whichFree = implied_free+ncols;
  int numberFree=0;
  int i;

  // memory for min max
  int * infiniteDown = new int [nrows];
  int * infiniteUp = new int [nrows];
  double * maxDown = new double[nrows];
  double * maxUp = new double[nrows];

  // mark as not computed
  // -1 => not computed, -2 give up (singleton), -3 give up (other)
  for (i=0;i<nrows;i++) {
    if (hinrow[i]>1)
      infiniteUp[i]=-1;
    else
      infiniteUp[i]=-2;
  }
#ifdef USE_SMALL_LARGE
  double large=1.0e10;
#else
  double large=1.0e20;
#endif

  int numberLook = prob->numberColsToDo_;
  int iLook;
  int * look = prob->colsToDo_;
  //int * look2 = NULL;
  // if gone from 2 to 3 look at all
  // why does this loop not check for prohibited columns? -- lh, 040818 --
  // changed 040923
  if (fill_level<0) {
    look = prob->usefulColumnInt_+ncols; //new int[ncols];
    //look=look2;
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
  int maxLook = CoinAbs(fill_level);
  for (iLook=0;iLook<numberLook;iLook++) {
    int j=look[iLook];
    if (hincol[j]  <= maxLook&&hincol[j]) {
      CoinBigIndex kcs = mcstrt[j];
      CoinBigIndex kce = kcs + hincol[j];
      bool singletonColumn = (hincol[j]==1);
      bool possible = false;
      bool singleton = false;
      CoinBigIndex k;
      double largestElement=0.0;
      for (k=kcs; k<kce; ++k) {
	int row = hrow[k];
	double coeffj = colels[k];

	// if its row is an equality constraint...
	if (hinrow[row] > 1 ) {
	  if ( fabs(rlo[row] - rup[row]) < tol &&
	       fabs(coeffj) > ZTOLDP2) {
	    possible=true;
	  }
	  largestElement = CoinMax(largestElement,fabs(coeffj));
	} else {
	  singleton=true;
	}
      }
      if (possible&&!singleton) {
	double low=-COIN_DBL_MAX;
	double high=COIN_DBL_MAX;
	// get bound implied by all rows
	for (k=kcs; k<kce; ++k) {
	  int row = hrow[k];
	  double coeffj = colels[k];
	  if (fabs(coeffj) > ZTOLDP2) {
	    if (infiniteUp[row]==-1) {
	      // compute
	      CoinBigIndex krs = mrstrt[row];
	      CoinBigIndex kre = krs + hinrow[row];
	      int infiniteUpper = 0;
	      int infiniteLower = 0;
	      double maximumUp = 0.0;
	      double maximumDown = 0.0;
	      CoinBigIndex kk;
	      // Compute possible lower and upper ranges
	      for (kk = krs; kk < kre; ++kk) {
		double value=rowels[kk];
		int iColumn = hcol[kk];
		if (value > 0.0) {
		  if (cup[iColumn] < large)
		    maximumUp += cup[iColumn] * value;
		  else
		    ++infiniteUpper;
		  if (clo[iColumn] > -large)
		    maximumDown += clo[iColumn] * value;
		  else
		    ++infiniteLower;
		} else if (value<0.0) {
		  if (cup[iColumn] < large)
		    maximumDown += cup[iColumn] * value;
		  else
		    ++infiniteLower;
		  if (clo[iColumn] > -large)
		    maximumUp += clo[iColumn] * value;
		  else
		    ++infiniteUpper;
		}
	      }
	      double maxUpx = maximumUp+infiniteUpper*1.0e31;
	      double maxDownx = maximumDown-infiniteLower*1.0e31;
	      if (maxUpx <= rup[row] + tol &&
		  maxDownx >= rlo[row] - tol) {

		// Row is redundant
		infiniteUp[row]=-3;

	      } else if (maxUpx < rlo[row] -tol &&!fixInfeasibility) {
		/* there is an upper bound and it can't be reached */
		prob->status_|= 1;
		prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
						prob->messages())
		  <<row
		  <<rlo[row]
		  <<rup[row]
		  <<CoinMessageEol;
		infiniteUp[row]=-3;
		break;
	      } else if ( maxDownx > rup[row]+tol&&!fixInfeasibility) {
		/* there is a lower bound and it can't be reached */
		prob->status_|= 1;
		prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
						prob->messages())
		  <<row
		  <<rlo[row]
		  <<rup[row]
		  <<CoinMessageEol;
		infiniteUp[row]=-3;
		break;
	      } else {
		infiniteUp[row]=infiniteUpper;
		infiniteDown[row]=infiniteLower;
		maxUp[row]=maximumUp;
		maxDown[row]=maximumDown;
	      }
	    }
	    if (infiniteUp[row]>=0) {
	      double lower = rlo[row];
	      double upper = rup[row];
	      double value=coeffj;
	      double nowLower = clo[j];
	      double nowUpper = cup[j];
	      double newBound;
	      int infiniteUpper=infiniteUp[row];
	      int infiniteLower=infiniteDown[row];
	      double maximumUp = maxUp[row];
	      double maximumDown = maxDown[row];
	      if (value > 0.0) {
		// positive value
		if (lower>-large) {
		  if (!infiniteUpper) {
		    assert(nowUpper < large);
		    newBound = nowUpper +
		      (lower - maximumUp) / value;
		    // relax if original was large
		    if (fabs(maximumUp)>1.0e8&&!singletonColumn)
		      newBound -= 1.0e-12*fabs(maximumUp);
		  } else if (infiniteUpper==1&&nowUpper>large) {
		    newBound = (lower -maximumUp) / value;
		    // relax if original was large
		    if (fabs(maximumUp)>1.0e8&&!singletonColumn)
		      newBound -= 1.0e-12*fabs(maximumUp);
		  } else {
		    newBound = -COIN_DBL_MAX;
		  }
		  if (newBound<=-large)
		    newBound = -COIN_DBL_MAX;
		  if (newBound > nowLower + 1.0e-12) {
		    // Tighten the lower bound
		    // adjust
		    double now;
		    if (nowLower<-large) {
		      now=0.0;
		      infiniteLower--;
		    } else {
		      now = nowLower;
		    }
		    maximumDown += (newBound-now) * value;
		    nowLower = newBound;
		  }
		  low=CoinMax(low,newBound);
		}
		if (upper <large) {
		  if (!infiniteLower) {
		    assert(nowLower >- large);
		    newBound = nowLower +
		      (upper - maximumDown) / value;
		    // relax if original was large
		    if (fabs(maximumDown)>1.0e8&&!singletonColumn)
		      newBound += 1.0e-12*fabs(maximumDown);
		  } else if (infiniteLower==1&&nowLower<-large) {
		    newBound =   (upper - maximumDown) / value;
		    // relax if original was large
		    if (fabs(maximumDown)>1.0e8&&!singletonColumn)
		      newBound += 1.0e-12*fabs(maximumDown);
		  } else {
		    newBound = COIN_DBL_MAX;
		  }
		  if (newBound>=large)
		    newBound = COIN_DBL_MAX;
		  if (newBound < nowUpper - 1.0e-12) {
		    // Tighten the upper bound
		    // adjust
		    double now;
		    if (nowUpper>large) {
		      now=0.0;
		      infiniteUpper--;
		    } else {
		      now = nowUpper;
		    }
		    maximumUp += (newBound-now) * value;
		    nowUpper = newBound;
		  }
		  high=CoinMin(high,newBound);
		}
	      } else {
		// negative value
		if (lower>-large) {
		  if (!infiniteUpper) {
		    assert(nowLower >- large);
		    newBound = nowLower +
		      (lower - maximumUp) / value;
		    // relax if original was large
		    if (fabs(maximumUp)>1.0e8&&!singletonColumn)
		      newBound += 1.0e-12*fabs(maximumUp);
		  } else if (infiniteUpper==1&&nowLower<-large) {
		    newBound = (lower -maximumUp) / value;
		    // relax if original was large
		    if (fabs(maximumUp)>1.0e8&&!singletonColumn)
		      newBound += 1.0e-12*fabs(maximumUp);
		  } else {
		    newBound = COIN_DBL_MAX;
		  }
		  if (newBound>=large)
		    newBound = COIN_DBL_MAX;
		  if (newBound < nowUpper - 1.0e-12) {
		    // Tighten the upper bound
		    // adjust
		    double now;
		    if (nowUpper>large) {
		      now=0.0;
		      infiniteLower--;
		    } else {
		      now = nowUpper;
		    }
		    maximumDown += (newBound-now) * value;
		    nowUpper = newBound;
		  }
		  high=CoinMin(high,newBound);
		}
		if (upper <large) {
		  if (!infiniteLower) {
		    assert(nowUpper < large);
		    newBound = nowUpper +
		      (upper - maximumDown) / value;
		    // relax if original was large
		    if (fabs(maximumDown)>1.0e8&&!singletonColumn)
		      newBound -= 1.0e-12*fabs(maximumDown);
		  } else if (infiniteLower==1&&nowUpper>large) {
		    newBound =   (upper - maximumDown) / value;
		    // relax if original was large
		    if (fabs(maximumDown)>1.0e8&&!singletonColumn)
		      newBound -= 1.0e-12*fabs(maximumDown);
		  } else {
		    newBound = -COIN_DBL_MAX;
		  }
		  if (newBound<=-large)
		    newBound = -COIN_DBL_MAX;
		  if (newBound > nowLower + 1.0e-12) {
		    // Tighten the lower bound
		    // adjust
		    double now;
		    if (nowLower<-large) {
		      now=0.0;
		      infiniteUpper--;
		    } else {
		      now = nowLower;
		    }
		    maximumUp += (newBound-now) * value;
		    nowLower = newBound;
		  }
		  low = CoinMax(low,newBound);
		}
	      }
	    } else if (infiniteUp[row]==-3) {
	      // give up
	      high=COIN_DBL_MAX;
	      low=-COIN_DBL_MAX;
	      break;
	    }
	  }
	}
	if (clo[j] <= low && high <= cup[j]) {

	  // both column bounds implied by the constraints of the problem
	  // get row
	  // If more than one equality is present, how do I know the one I
	  // select here will be the one that actually implied tighter
	  // bounds? Seems like I should care.  -- lh, 040818 --
	  largestElement *= 0.1;
	  int krow=-1;
	  int ninrow=ncols+1;
	  double thisValue=0.0;
	  for (k=kcs; k<kce; ++k) {
	    int row = hrow[k];
	    double coeffj = colels[k];
	    if ( fabs(rlo[row] - rup[row]) < tol &&
		 fabs(coeffj) > largestElement) {
	      if (hinrow[row]<ninrow) {
		ninrow=hinrow[row];
		krow=row;
		thisValue=coeffj;
	      }
	    }
	  }
	  if (krow>=0) {
	    bool goodRow=true;
	    if (integerType[j]) {
	      // can only accept if good looking row
	      double scaleFactor = 1.0/thisValue;
	      double rhs = rlo[krow]*scaleFactor;
	      if (fabs(rhs-floor(rhs+0.5))<tol) {
		CoinBigIndex krs = mrstrt[krow];
		CoinBigIndex kre = krs + hinrow[krow];
		CoinBigIndex kk;
		bool allOnes=true;
		for (kk = krs; kk < kre; ++kk) {
		  double value=rowels[kk]*scaleFactor;
		  if (fabs(value)!=1.0)
		    allOnes=false;
		  int iColumn = hcol[kk];
		  if (!integerType[iColumn]||fabs(value-floor(value+0.5))>tol) {
		    goodRow=false;
		    break;
		  }
		}
		if (rlo[krow]==1.0&&hinrow[krow]>=5&&stopSomeStuff&&allOnes)
		  goodRow=false; // may spoil SOS
	      } else {
		goodRow=false;
	      }
	    }
	    if (goodRow) {
	      implied_free[numberFree] = krow;
	      whichFree[numberFree++] = j;
	      // And say row no good for further use
	      infiniteUp[krow]=-3;
	      //printf("column %d implied free by row %d hincol %d hinrow %d\n",
	      //     j,krow,hincol[j],hinrow[krow]);
	    }
	  }
	}
      }
    }
  }

  delete [] infiniteDown;
  delete [] infiniteUp;
  delete [] maxDown;
  delete [] maxUp;

  int isolated_row = -1;

  // first pick off the easy ones
  // note that this will only deal with columns that were originally
  // singleton; it will not deal with doubleton columns that become
  // singletons as a result of dropping rows.
  for (iLook=0;iLook<numberFree;iLook++) {
    int j=whichFree[iLook];
    if (hincol[j] == 1) {
      CoinBigIndex kcs = mcstrt[j];
      int row = hrow[kcs];
      double coeffj = colels[kcs];

      CoinBigIndex krs = mrstrt[row];
      CoinBigIndex kre = krs + hinrow[row];


      // isolated rows are weird
      {
	int n = 0;
	for (CoinBigIndex k=krs; k<kre; ++k)
	  n += hincol[hcol[k]];
	if (n==hinrow[row]) {
	  isolated_row = row;
	  break;
	}
      }

      const bool nonzero_cost = (cost[j] != 0.0&&fabs(rup[row]-rlo[row])<=tol);

      double *save_costs = nonzero_cost ? new double[hinrow[row]] : NULL;

      {
	action *s = &actions[nactions++];

	s->row = row;
	s->col = j;
	PRESOLVE_DETAIL_PRINT(printf("pre_impliedfree2 %dC %dR E\n",j,row));

	s->clo = clo[j];
	s->cup = cup[j];
	s->rlo = rlo[row];
	s->rup = rup[row];

	s->ninrow = hinrow[row];
	s->rowels = presolve_dupmajor(rowels,hcol,hinrow[row],krs) ;
	s->costs = save_costs;
      }

      if (nonzero_cost) {
	double rhs = rlo[row];
	double costj = cost[j];

#	if PRESOLVE_DEBUG
	printf("FREE COSTS:  %g  ", costj);
#	endif
	for (CoinBigIndex k=krs; k<kre; k++) {
	  int jcol = hcol[k];
	  save_costs[k-krs] = cost[jcol];

	  if (jcol != j) {
	    double coeff = rowels[k];

#	    if PRESOLVE_DEBUG
	    printf("%g %g   ", cost[jcol], coeff/coeffj);
#	    endif
	    /*
	     * Similar to eliminating doubleton:
	     *   cost1 x = cost1 (c - b y) / a = (c cost1)/a - (b cost1)/a
	     *   cost[icoly] += cost[icolx] * (-coeff2 / coeff1);
	     */
	    cost[jcol] += costj * (-coeff / coeffj);
	  }
	}
#	if PRESOLVE_DEBUG
	printf("\n");

	/* similar to doubleton */
	printf("BIAS??????? %g %g %g %g\n",
	       costj * rhs / coeffj,
	       costj, rhs, coeffj);
#	endif
	prob->change_bias(costj * rhs / coeffj);
	// ??
	cost[j] = 0.0;
      }

      /* remove the row from the columns in the row */
      for (CoinBigIndex k=krs; k<kre; k++) {
	int jcol=hcol[k];
	prob->addCol(jcol);
	presolve_delete_from_col(row,jcol,mcstrt,hincol,hrow,colels) ;
	if (hincol[jcol] == 0)
	{ PRESOLVE_REMOVE_LINK(prob->clink_,jcol) ; }
      }
      PRESOLVE_REMOVE_LINK(rlink, row);
      hinrow[row] = 0;

      // just to make things squeeky
      rlo[row] = 0.0;
      rup[row] = 0.0;

      PRESOLVE_REMOVE_LINK(clink, j);
      hincol[j] = 0;

      implied_free[iLook] = -1;
    }
  }

  //delete [] look2;
  if (nactions) {
#   if PRESOLVE_SUMMARY
    printf("NIMPLIED FREE:  %d\n", nactions);
#   endif
    action *actions1 = new action[nactions];
    CoinMemcpyN(actions, nactions, actions1);
    next = new implied_free_action(nactions, actions1, next);
  }
  delete [] actions;

  if (isolated_row != -1) {
    const CoinPresolveAction *nextX = isolated_constraint_action::presolve(prob,
						isolated_row, next);
    if (nextX)
      next = nextX; // may fail
  }
  // try more complex ones
  if (fill_level) {
    next = subst_constraint_action::presolve(prob, implied_free,
					     whichFree,numberFree,
					     next,fill_level);
  }
  //delete[]implied_free;

  if (prob->tuning_) {
    double thisTime=CoinCpuTime();
    int droppedRows = prob->countEmptyRows() - startEmptyRows ;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
    printf("CoinPresolveImpliedFree(64) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_);
  }
  return (next);
}



const char *implied_free_action::name() const
{
  return ("implied_free_action");
}

void implied_free_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

  double *elementByColumn	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *columnStart		= prob->mcstrt_;
  int *numberInColumn		= prob->hincol_;
  int *link		= prob->link_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *sol	= prob->sol_;

  double *rcosts	= prob->rcosts_;
  double *dcost		= prob->cost_;

  double *acts	= prob->acts_;
  double *rowduals = prob->rowduals_;

  const double maxmin	= prob->maxmin_;

# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
  char *cdone	= prob->cdone_;
  char *rdone	= prob->rdone_;
# endif

  CoinBigIndex &free_list = prob->free_list_;

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {

    int irow = f->row;
    int icol = f->col;

    int ninrow = f->ninrow;
    const double *rowels = f->rowels;
    const int *rowcols = reinterpret_cast<const int *>(rowels+ninrow) ;
    const double *save_costs = f->costs;

    // put back coefficients in the row
    // this includes recreating the singleton column
    {
      for (int k = 0; k<ninrow; k++) {
	int jcol = rowcols[k];
	double coeff = rowels[k];

	if (save_costs) {
	  rcosts[jcol] += maxmin*(save_costs[k]-dcost[jcol]);
	  dcost[jcol] = save_costs[k];
	}
	{
	  CoinBigIndex kk = free_list;
	  assert(kk >= 0 && kk < prob->bulk0_) ;
	  free_list = link[free_list];
	  link[kk] = columnStart[jcol];
	  columnStart[jcol] = kk;
	  elementByColumn[kk] = coeff;
	  hrow[kk] = irow;
	}

	if (jcol == icol) {
	  // initialize the singleton column
	  numberInColumn[jcol] = 1;
	  clo[icol] = f->clo;
	  cup[icol] = f->cup;

# 	  if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
	  cdone[icol] = IMPLIED_FREE;
#	  endif
	} else {
	  numberInColumn[jcol]++;
	}
      }
#     if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
      rdone[irow] = IMPLIED_FREE;
#     endif
#     if PRESOLVE_CONSISTENCY
      presolve_check_free_list(prob) ;
#     endif

      rlo[irow] = f->rlo;
      rup[irow] = f->rup;
    }
    //deleteAction( f->costs,double*); do on delete
    // coeff has now been initialized

    // compute solution
    {
      double act = 0.0;
      double coeff = 0.0;

      for (int k = 0; k<ninrow; k++)
	if (rowcols[k] == icol)
	  coeff = rowels[k];
	else {
	  int jcol = rowcols[k];
	  PRESOLVE_STMT(CoinBigIndex kk = presolve_find_row2(irow, columnStart[jcol], numberInColumn[jcol], hrow, link));
	  act += rowels[k] * sol[jcol];
	}

      PRESOLVEASSERT(fabs(coeff) > ZTOLDP);
      double thisCost = maxmin*dcost[icol];
      double loActivity,upActivity;
      if (coeff>0) {
	loActivity = (rlo[irow]-act)/coeff;
	upActivity = (rup[irow]-act)/coeff;
      } else {
	loActivity = (rup[irow]-act)/coeff;
	upActivity = (rlo[irow]-act)/coeff;
      }
      loActivity = CoinMax(loActivity,clo[icol]);
      upActivity = CoinMin(upActivity,cup[icol]);
      int where; //0 in basis, -1 at lb, +1 at ub
      const double tolCheck	= 0.1*prob->ztolzb_;
      if (loActivity<clo[icol]+tolCheck/fabs(coeff)&&thisCost>=0.0)
	where=-1;
      else if (upActivity>cup[icol]-tolCheck/fabs(coeff)&&thisCost<0.0)
	where=1;
      else
	where =0;
      // But we may need to put in basis to stay dual feasible
      double possibleDual = thisCost/coeff;
      if (where) {
	double worst=  prob->ztoldj_;
	for (int k = 0; k<ninrow; k++) {
	  int jcol = rowcols[k];
	  if (jcol!=icol) {
	    CoinPrePostsolveMatrix::Status status = prob->getColumnStatus(jcol);
	    // can only trust basic
	    if (status==CoinPrePostsolveMatrix::basic) {
	      if (fabs(rcosts[jcol])>worst)
		worst=fabs(rcosts[jcol]);
	    } else if (sol[jcol]<clo[jcol]+ZTOLDP) {
	      if (-rcosts[jcol]>worst)
		worst=-rcosts[jcol];
	    } else if (sol[jcol]>cup[jcol]-ZTOLDP) {
	      if (rcosts[jcol]>worst)
		worst=rcosts[jcol];
	    }
	  }
	}
	if (worst>prob->ztoldj_) {
	  // see if better if in basis
	  double worst2	= prob->ztoldj_;
	  for (int k = 0; k<ninrow; k++) {
	    int jcol = rowcols[k];
	    if (jcol!=icol) {
	      double coeff = rowels[k];
	      double newDj = rcosts[jcol]-possibleDual*coeff;
	      CoinPrePostsolveMatrix::Status status = prob->getColumnStatus(jcol);
	      // can only trust basic
	      if (status==CoinPrePostsolveMatrix::basic) {
		if (fabs(newDj)>worst2)
		  worst2=fabs(newDj);
	      } else if (sol[jcol]<clo[jcol]+ZTOLDP) {
		if (-newDj>worst2)
		  worst2=-newDj;
	      } else if (sol[jcol]>cup[jcol]-ZTOLDP) {
		if (newDj>worst2)
		  worst2=newDj;
	      }
	    }
	  }
	  if (worst2<worst)
	    where=0; // put in basis
	}
      }
      if (!where) {
	// choose rowdual to make this col basic
	rowduals[irow] = possibleDual;
	if ((rlo[irow] < rup[irow] && rowduals[irow] < 0.0)
	    || rlo[irow]< -1.0e20) {
	  //if (rlo[irow]<-1.0e20&&rowduals[irow]>ZTOLDP)
	  //printf("IMP %g %g %g\n",rlo[irow],rup[irow],rowduals[irow]);
	  sol[icol] = (rup[irow] - act) / coeff;
	  //assert (sol[icol]>=clo[icol]-1.0e-5&&sol[icol]<=cup[icol]+1.0e-5);
	  acts[irow] = rup[irow];
	  prob->setRowStatus(irow,CoinPrePostsolveMatrix::atUpperBound);
	} else {
	  sol[icol] = (rlo[irow] - act) / coeff;
	  //assert (sol[icol]>=clo[icol]-1.0e-5&&sol[icol]<=cup[icol]+1.0e-5);
	  acts[irow] = rlo[irow];
	  prob->setRowStatus(irow,CoinPrePostsolveMatrix::atLowerBound);
	}
	prob->setColumnStatus(icol,CoinPrePostsolveMatrix::basic);
	for (int k = 0; k<ninrow; k++) {
	  int jcol = rowcols[k];
	  double coeff = rowels[k];
	  rcosts[jcol] -= possibleDual*coeff;
	}
	rcosts[icol] = 0.0;
      } else {
	rowduals[irow] = 0.0;
	rcosts[icol] = thisCost;
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::basic);
	if (where<0) {
	  // to lb
	  prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atLowerBound);
	  sol[icol]=clo[icol];
	} else {
	  // to ub
	  prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atUpperBound);
	  sol[icol]=cup[icol];
	}
	acts[irow] = act + sol[icol]*coeff;
	//assert (acts[irow]>=rlo[irow]-1.0e-5&&acts[irow]<=rup[irow]+1.0e-5);
      }
#     if PRESOLVE_DEBUG
      {
	double *colels	= prob->colels_;
	int *hrow	= prob->hrow_;
	const CoinBigIndex *mcstrt	= prob->mcstrt_;
	int *hincol	= prob->hincol_;
	for (int j = 0; j<ninrow; j++) {
	  int jcol = rowcols[j];
	  CoinBigIndex k = mcstrt[jcol];
	  int nx = hincol[jcol];
	  double dj = dcost[jcol];
	  for (int i=0; i<nx; ++i) {
	    int row = hrow[k];
	    double coeff = colels[k];
	    k = link[k];
	    dj -= rowduals[row] * coeff;
	    //printf("col jcol row %d coeff %g dual %g new dj %g\n",
	    // row,coeff,rowduals[row],dj);
	  }
	  if (fabs(dj-rcosts[jcol])>1.0e-3)
	    printf("changed\n");
	}
      }
#     endif
    }
  }

  return ;
}


/*
  Why do we delete costs during postsolve() execution, but none of the other
  components of the action?
*/
implied_free_action::~implied_free_action()
{
  int i;
  for (i=0;i<nactions_;i++) {
    deleteAction(actions_[i].rowels,double *);
    deleteAction( actions_[i].costs,double *);
  }
  deleteAction(actions_,action *);
}
