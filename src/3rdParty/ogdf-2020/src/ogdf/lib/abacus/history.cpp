/*!\file
 * \author Matthias Elf
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#include <ogdf/lib/abacus/history.h>
#include <iomanip>

using std::setw;

namespace abacus {


std::ostream& operator<<(std::ostream& out, const History &rhs)
{
	int64_t   min;       //!< total minutes
	int64_t   rmin;      //!< total minutes modulo 60
	int64_t   rsec;      //!< total seconds modulo 60

	const double eps      = rhs.master_->eps();
	const double infinity = rhs.master_->infinity();
	const bool   max      = rhs.master_->optSense()->max();

	// History: constants for formatting output
	const int w1 = 10;   //!< width of first column
	const int w2 = 13;   //!< width of second column
	const int w3 = 13;   //!< width of third column
	const int w4 = 12;   //!< width of forth column
	const int w5 = 12;   //!< width of fifth column
	const int w6 = 12;   //!< width of sixth column

	out << "Solution History" << std::endl << std::endl;

	if (rhs.n_) {

		// History: local variables of the output operator
		double    guarantee=0.0; //!< guarantee of a solution in the history
		double    quality=0.0;   //!< quality of a solution in the history
		bool      opt;       //!< \a true if problem has been solved to optimality
		bool      qAvailable;   //!< \a true if quality can be given at this point of history
		bool      gAvailable;   //!< \a true if guarantee can be given at this point of history
		bool      feasible;  //!< \a true if feasible solution available at this point of history
		int       last;      //!< index of last entry in history table
		double    ub;        //!< upper bound
		double    lb;        //!< lower bound
		double    optVal=0.0;    //!< value of the optimum solution

		// headline of history table
		out << setw(w1) << "Solution";

		if (rhs.master_->optSense()->max()) {
			out << setw(w2) << "Feas. Sol.";
			out << setw(w3) << "Upper Bound";
		}
		else {
			out << setw(w2) << "Lower Bound";
			out << setw(w3) << "Feas. Sol.";
		}

		out << setw(w4) << "Guarantee";
		out << setw(w5) << "Quality";
		out << setw(w6) << "Time";
		out << std::endl;

		// has the optimum solution been proved?
		/* If the optimum solution has been proved we can also output the
		*  quality of every feasible solution in the history table.
		*/
		last = rhs.n_ - 1;

		if (fabs(rhs.primalBound_[last] - rhs.dualBound_[last]) < eps) {
			opt    = true;
			optVal = rhs.primalBound_[last];
		}
		else opt = false;

		// output the history table
		for(int i = 0; i <= last; i++) {

			// determine guarantee and quality

			// determine the upper and the lower bound
			/* In a maximization problem the lower bounds are the primal feasible solutions
			*  und the upper bounds the dual feasible solutions, in a minimization
			*  problem this is vice versa.
			*/
			if (max) {
				ub = rhs.dualBound_[i];
				lb = rhs.primalBound_[i];
			}
			else {
				lb = rhs.dualBound_[i];
				ub = rhs.primalBound_[i];
			}

			// determine if a feasible solutions is currently available
			/* A feasible solution is available if the lower bound is not minus
			*  infinity for maximization problems, and the upper bound not infinity
			*  for minimization problems, respectively.
			*/
			if (max)
				feasible = (lb == -infinity ? false : true);
			else
				feasible = (ub == infinity ? false : true);

			// determine the guarantee
			/* A guarantee can only be given if a feasible solution
			*  is available, i.e., the lower bound is not minus infinity for minimization
			*  problems, or the upper bound is not infinity for maximization problems,
			*  respectively, and the lower bound is either nonzero or equals the upper
			*  bound.
			*/
			if (feasible) {
				if (max)
					gAvailable = (ub == infinity ? false : true);
				else
					gAvailable = (lb == -infinity ? false : true);

				if (gAvailable) {
					if (fabs(lb) > eps)          guarantee = fabs((ub - lb)/lb*100.0);
					else if(fabs(ub - lb) < eps) guarantee = 0.0;
					else gAvailable = false;
				}
			}
			else gAvailable = false;

			// determine the quality
			/* The quality can be only determined if the optimum solution has been
			*  found and at the current point of the history a feasible solution is
			*  available. Moreover, the nominator in the expression for the guarantee
			*  computation must either be nonzero, or the denominator must be zero.
			*/
			if (opt && feasible) {
				if (max) {
					if (fabs(lb) > eps) {
						qAvailable = true;
						quality = fabs((optVal - lb)/lb*100.0);
					}
					else if (fabs(optVal - lb) < eps) {
						qAvailable = true;
						quality = 0.0;
					}
					else qAvailable = false;
				}
				else {
					if (fabs(optVal) > eps) {
						qAvailable = true;
						quality = fabs((ub - optVal)/optVal*100.0);
					}
					else if (fabs(ub - optVal) < eps) {
						qAvailable = true;
						quality= 0.0;
					}
					else qAvailable = false;
				}

			}
			else qAvailable = false;

			out << setw(w1) << i;

			out << setw(w2) << lb;
			out << setw(w3) << ub;

			if (gAvailable) out << setw(w4-1) << guarantee << "%";
			else         out << setw(w4)   << "---";
			if (qAvailable) out << setw(w5-1) << quality << "%";
			else         out << setw(w5)   << "---";

			// output the history time
			/* The time is recorded in seconds and is transformed to the
			*  format {\tt hh:mm:ss}. Up to 999 hours the output will be nice.
			*/
			rsec = rhs.time_[i]%60;
			min  = rhs.time_[i]/60;
			rmin = min%60;

			out << setw(w6-9) << "";  //!< to get the correct width
			out << setw(3) << min/60 << ":";
			if(rmin < 10) out << '0';
			out << rmin << ':';
			if(rsec < 10) out << '0';
			out << rsec;

			out << std::endl;
		}
	}
	else
		out << "no solution history available" << std::endl;
	return out;
}


void History::update()
{
	if (n_ == size()) realloc();

	dualBound_[n_]   = master_->dualBound();
	primalBound_[n_] = master_->primalBound();
	time_[n_]        = master_->totalTime()->seconds();
	++n_;
}


void History::realloc()
{
	primalBound_.grow(100);
	dualBound_.grow(100);
	time_.grow(100);
}
}
