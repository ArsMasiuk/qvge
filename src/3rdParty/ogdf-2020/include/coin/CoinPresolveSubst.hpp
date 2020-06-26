/* $Id: CoinPresolveSubst.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveSubst_H
#define CoinPresolveSubst_H
#define	SUBST_ROW	21

#include "CoinPresolveMatrix.hpp"

class subst_constraint_action : public CoinPresolveAction {
private:
  subst_constraint_action();
  subst_constraint_action(const subst_constraint_action& rhs);
  subst_constraint_action& operator=(const subst_constraint_action& rhs);

  struct action {
    double *rlos;
    double *rups;

    double *coeffxs;
    int *rows;

    int *ninrowxs;
    /*const*/ int *rowcolsxs;
    /*const*/ double *rowelsxs;

    const double *costsx;
    int col;
    int rowy;

    int nincol;
  };

  const int nactions_;
  // actions_ is owned by the class and must be deleted at destruction
  const action *const actions_;

  subst_constraint_action(int nactions,
			  action *actions,
			  const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions) {}

 public:
  const char *name() const override;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					    const int *implied_free,
					    const int * which,
					    int numberFree,
					    const CoinPresolveAction *next,
					    int & fill_level);
  static const CoinPresolveAction *presolveX(CoinPresolveMatrix * prob,
				  const CoinPresolveAction *next,
				  int fillLevel);

  void postsolve(CoinPostsolveMatrix *prob) const override;

  ~subst_constraint_action();
};





/*static*/ void implied_bounds(const double *els,
			   const double *clo, const double *cup,
			   const int *hcol,
			   CoinBigIndex krs, CoinBigIndex kre,
			   double *maxupp, double *maxdownp,
			   int jcol,
			   double rlo, double rup,
			   double *iclb, double *icub);
#endif
