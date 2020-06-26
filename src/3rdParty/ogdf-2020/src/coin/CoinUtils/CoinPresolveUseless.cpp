/* $Id: CoinPresolveUseless.cpp 1448 2011-06-19 15:34:41Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>
#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinHelperFunctions.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

// WHAT HAPPENS IF COLS ARE DROPPED AS A RESULT??
// should be like do_tighten.
// not really - one could fix costed variables to appropriate bound.
// ok, don't bother about it.  If it is costed, it will be checked
// when it is eliminated as an empty col; if it is costed in the
// wrong direction, the problem is unbounded, otherwise it is pegged
// at its bound.  no special action need be taken here.
const CoinPresolveAction *useless_constraint_action::presolve(CoinPresolveMatrix * prob,
								  const int *useless_rows,
								  int nuseless_rows,
				       const CoinPresolveAction *next)
{
  // may be modified by useless constraint
  double *colels	= prob->colels_;

  // may be modified by useless constraint
        int *hrow	= prob->hrow_;

  const CoinBigIndex *mcstrt	= prob->mcstrt_;

  // may be modified by useless constraint
        int *hincol	= prob->hincol_;

	//  double *clo	= prob->clo_;
	//  double *cup	= prob->cup_;

  const double *rowels	= prob->rowels_;
  const int *hcol	= prob->hcol_;
  const CoinBigIndex *mrstrt	= prob->mrstrt_;

  // may be written by useless constraint
        int *hinrow	= prob->hinrow_;
	//  const int nrows	= prob->nrows_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  action *actions	= new action [nuseless_rows];

# if PRESOLVE_DEBUG
  std::cout << "Entering useless_constraint_action::presolve." << std::endl ;
  presolve_check_sol(prob) ;
# endif
# if PRESOLVE_SUMMARY
  printf("NUSELESS ROWS:  %d\n", nuseless_rows);
# endif

  for (int i=0; i<nuseless_rows; ++i) {
    int irow = useless_rows[i];
    CoinBigIndex krs = mrstrt[irow];
    CoinBigIndex kre = krs + hinrow[irow];
    PRESOLVE_DETAIL_PRINT(printf("pre_useless %dR E\n",irow));

    action *f = &actions[i];

    f->row = irow;
    f->ninrow = hinrow[irow];
    f->rlo = rlo[irow];
    f->rup = rup[irow];
    f->rowcols = CoinCopyOfArray(&hcol[krs], hinrow[irow]);
    f->rowels  = CoinCopyOfArray(&rowels[krs], hinrow[irow]);

    for (CoinBigIndex k=krs; k<kre; k++)
    { presolve_delete_from_col(irow,hcol[k],mcstrt,hincol,hrow,colels) ;
      if (hincol[hcol[k]] == 0)
      { PRESOLVE_REMOVE_LINK(prob->clink_,hcol[k]) ; } }
    hinrow[irow] = 0;
    PRESOLVE_REMOVE_LINK(prob->rlink_,irow) ;

    // just to make things squeeky
    rlo[irow] = 0.0;
    rup[irow] = 0.0;
  }

# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
  std::cout << "Leaving useless_constraint_action::presolve." << std::endl ;
# endif

  next = new useless_constraint_action(nuseless_rows, actions, next);

  return (next);
}
// Put constructors here
useless_constraint_action::useless_constraint_action(int nactions,
                                                     const action *actions,
                                                     const CoinPresolveAction *next)
  :   CoinPresolveAction(next),
      nactions_(nactions),
      actions_(actions)
{}
useless_constraint_action::~useless_constraint_action()
{
  for (int i=0;i<nactions_;i++) {
    deleteAction(actions_[i].rowcols, int *);
    deleteAction(actions_[i].rowels, double *);
  }
  deleteAction(actions_, action *);
}

const char *useless_constraint_action::name() const
{
  return ("useless_constraint_action");
}

void useless_constraint_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *link		= prob->link_;
  int *hincol		= prob->hincol_;

  //  double *rowduals	= prob->rowduals_;
  double *rowacts	= prob->acts_;
  const double *sol	= prob->sol_;


  CoinBigIndex &free_list		= prob->free_list_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {

    int irow	= f->row;
    int ninrow	= f->ninrow;
    const int *rowcols	= f->rowcols;
    const double *rowels = f->rowels;
    double rowact = 0.0;

    rup[irow] = f->rup;
    rlo[irow] = f->rlo;

    for (CoinBigIndex k=0; k<ninrow; k++) {
      int jcol = rowcols[k];
      //      CoinBigIndex kk = mcstrt[jcol];

      // append deleted row element to each col
      {
	CoinBigIndex kk = free_list;
	assert(kk >= 0 && kk < prob->bulk0_) ;
	free_list = link[free_list];
	hrow[kk] = irow;
	colels[kk] = rowels[k];
	link[kk] = mcstrt[jcol];
	mcstrt[jcol] = kk;
      }

      rowact += rowels[k] * sol[jcol];
      hincol[jcol]++;
    }
#   if PRESOLVE_CONSISTENCY
    presolve_check_free_list(prob) ;
#   endif

    // I don't know if this is always true
    PRESOLVEASSERT(prob->getRowStatus(irow)==CoinPrePostsolveMatrix::basic);
    // rcosts are unaffected since rowdual is 0

    rowacts[irow] = rowact;
    // leave until desctructor
    //deleteAction(rowcols,int *);
    //deleteAction(rowels,double *);
  }

  //deleteAction(actions_,action *);

# if PRESOLVE_CONSISTENCY
  presolve_check_threads(prob) ;
# endif

}
