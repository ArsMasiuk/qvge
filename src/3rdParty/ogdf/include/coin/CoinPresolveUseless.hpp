/* $Id: CoinPresolveUseless.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveUseless_H
#define CoinPresolveUseless_H
#define	USELESS		20

class useless_constraint_action : public CoinPresolveAction {
  struct action {
    double rlo;
    double rup;
    const int *rowcols;
    const double *rowels;
    int row;
    int ninrow;
  };

  const int nactions_;
  const action *const actions_;

  useless_constraint_action(int nactions,
                            const action *actions,
                            const CoinPresolveAction *next);

 public:
  const char *name() const;

  // These rows are asserted to be useless,
  // that is, given a solution the row activity
  // must be in range.
  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					 const int *useless_rows,
					 int nuseless_rows,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  ~useless_constraint_action();

};


#endif
