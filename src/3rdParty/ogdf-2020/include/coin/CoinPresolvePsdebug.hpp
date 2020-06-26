/* $Id: CoinPresolvePsdebug.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolvePsdebug_H
#define CoinPresolvePsdebug_H

/*! \defgroup PresolveDebugFunctions Presolve Debug Functions

  These functions implement consistency checks on data structures
  involved in presolve and postsolve and on the components of the lp solution.

  To use these functions, include CoinPresolvePsdebug.hpp in your file and
  define the compile-time constants PRESOLVE_SUMMARY, PRESOLVE_DEBUG, and
  PRESOLVE_CONSISTENCY (either in individual files or in Coin/Makefile).
  A value is needed (<i>i.e.</i>, PRESOLVE_DEBUG=1) but not at present used
  to control debug level. Be sure that the definition occurs before any
  CoinPresolve*.hpp file is processed.
*/

//@{

/*! \relates CoinPresolveMatrix
    \brief Check column-major and/or row-major matrices for duplicate
	   entries in the major vectors.

  By default, scans both the column- and row-major matrices. Set doCol (doRow)
  to false to suppress one or the other.
*/
void presolve_no_dups(const CoinPresolveMatrix *preObj,
		      bool doCol = true, bool doRow = true) ;

/*! \relates CoinPresolveMatrix
    \brief Check the links which track storage order for major vectors in
    the bulk storage area.

  By default, scans only the column-major matrix. Set doCol = false to
  suppress the scan. Set doRow = false to scan the row-major links. But be
  warned, the row-major links are not maintained with the same zeal as the
  column-major links.
*/
void presolve_links_ok(const CoinPresolveMatrix *preObj,
		       bool doCol = true, bool doRow = false) ;

/*! \relates CoinPresolveMatrix
    \brief Check for explicit zeros in the column- and/or row-major matrices.

  By default, scans both the column- and row-major matrices. Set doCol (doRow)
  to false to suppress one or the other.
*/
void presolve_no_zeros(const CoinPresolveMatrix *preObj,
		       bool doCol = true, bool doRow = true) ;

/*! \relates CoinPresolveMatrix
    \brief Checks for equivalence of the column- and row-major matrices.

  Normally the routine will test for coefficient presence and value. Set
  \p chkvals to false to suppress the check for equal value.
*/
void presolve_consistent(const CoinPresolveMatrix *preObj,
			 bool chkvals = true) ;

/*! \relates CoinPostsolveMatrix
    \brief Checks that column threads agree with column lengths
*/
void presolve_check_threads(const CoinPostsolveMatrix *obj) ;

/*! \relates CoinPostsolveMatrix
    \brief Checks the free list

    Scans the thread of free locations in the bulk store and checks that all
    entries are reasonable (0 <= index < bulk0_). If chkElemCnt is true, it
    Also checks that the total number of entries in the matrix plus the
    locations on the free list total to the size of the bulk store. Postsolve
    routines do not maintain an accurate element count, but this is useful
    for checking a newly constructed postsolve matrix.
*/
void presolve_check_free_list(const CoinPostsolveMatrix *obj,
			      bool chkElemCnt = false) ;

/*! \relates CoinPostsolveMatrix
    \brief Check stored reduced costs for accuracy and consistency with
	   variable status.

  The routine will check the value of the reduced costs for architectural
  variables (CoinPrePostsolveMatrix::rcosts_). It performs an accuracy check
  by recalculating the reduced cost from scratch. It will also check the
  value for consistency with the status information in
  CoinPrePostsolveMatrix::colstat_.
*/
void presolve_check_reduced_costs(const CoinPostsolveMatrix *obj) ;

/*! \relates CoinPostsolveMatrix
    \brief Check the dual variables for consistency with row activity.

  The routine checks that the value of the dual variable is consistent
  with the state of the constraint (loose, tight at lower bound, or tight at
  upper bound).
*/
void presolve_check_duals(const CoinPostsolveMatrix *postObj) ;

/*! \relates CoinPresolveMatrix
    \brief Check primal solution and architectural variable status.

    The architectural variables can be checked for bogus values, feasibility,
    and valid status. The row activity is checked for bogus values, accuracy,
    and feasibility.  By default, row activity is not checked (presolve is
    sloppy about maintaining it). See the definitions in
    CoinPresolvePsdebug.cpp for more information.
*/
void presolve_check_sol(const CoinPresolveMatrix *preObj,
			int chkColSol = 2, int chkRowAct = 1,
			int chkStatus = 1) ;

/*! \relates CoinPostsolveMatrix
    \brief Check primal solution and architectural variable status.

    The architectural variables can be checked for bogus values, feasibility,
    and valid status. The row activity is checked for bogus values, accuracy,
    and feasibility. See the definitions in CoinPresolvePsdebug.cpp for more
    information.
*/
void presolve_check_sol(const CoinPostsolveMatrix *postObj,
			int chkColSol = 2, int chkRowAct = 2,
			int chkStatus = 1) ;

/*! \relates CoinPresolveMatrix
    \brief Check for the proper number of basic variables.
*/
void presolve_check_nbasic(const CoinPresolveMatrix *preObj) ;

/*! \relates CoinPostsolveMatrix
    \brief Check for the proper number of basic variables.
*/
void presolve_check_nbasic(const CoinPostsolveMatrix *postObj) ;

//@}

#endif
