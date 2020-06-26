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

#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/lp.h>
#include <ogdf/lib/abacus/sparvec.h>
#include <ogdf/lib/abacus/row.h>
#include <ogdf/lib/abacus/column.h>
#include <ogdf/lib/abacus/lpvarstat.h>
#include <ogdf/lib/abacus/slackstat.h>

namespace abacus {


//LP::LP(Master*master)
//
//	:
//	master_(master),
//	optStat_(Unoptimized),
//	xValStatus_(Missing),
//	barXValStatus_(Missing),
//	yValStatus_(Missing),
//	recoStatus_(Missing),
//	slackStatus_(Missing),
//	basisStatus_(Missing),
//	nOpt_(0)
//{}
//
//LP::~LP()
//
//{}

//void LP::initialize(OptSense sense,
//						int nRow,
//						int maxRow,
//						int nCol,
//						int maxCol,
//						Array<double> &obj,
//						Array<double> &lBound,
//						Array<double> &uBound,
//						Array<Row*> &rows)
//
//{
//	_initialize(sense,nRow,maxRow,nCol,maxCol,obj,lBound,uBound,rows);
//}

//void LP::initialize(OptSense sense,
//						int nRow,
//						int maxRow,
//						int nCol,
//						int maxCol,
//						Array<double> &obj,
//						Array<double> &lBound,
//						Array<double> &uBound,
//						Array<Row*> &rows,
//						Array<LPVARSTAT::STATUS> &lpVarStat,
//						Array<SlackStat::STATUS> &slackStat)
//
//{
//	_initialize(sense,nRow,maxRow,nCol,maxCol,obj,lBound,uBound,rows);
//	LP::loadBasis(lpVarStat,slackStat);
//}
//
//void LP::loadBasis(Array<LPVARSTAT::STATUS> &lpVarStat,
//					   Array<SlackStat::STATUS> &slackStat)
//
//{
//	_loadBasis(lpVarStat,slackStat);
//}

LP::OPTSTAT LP::optimize(METHOD method)
{
	if(nCol()==0){
		Logger::ifout() << "LP::optimize(): cannot optimize (number of columns is 0)\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Lp);
	}

	++nOpt_;

	switch(method){
	case Primal:
		optStat_= _primalSimplex();
		break;
	case Dual:
		optStat_= _dualSimplex();
		break;
	case BarrierAndCrossover:
		optStat_= _barrier(true);
		break;
	case BarrierNoCrossover:
		optStat_= _barrier(false);
		break;
	case Approximate:
		optStat_= _approx();
		break;
	}

	return optStat_;
}


//void LP::remRows(ArrayBuffer<int> &ind)
//{
//	initPostOpt();
//
//	_remRows(ind);
//}


//void LP::initPostOpt()
//{
//	optStat_= Unoptimized;
//	xValStatus_= barXValStatus_= recoStatus_= Missing;
//	slackStatus_= yValStatus_= basisStatus_= Missing;
//}


void LP::addRows(ArrayBuffer<Row*> &newRows)
{
	if(nRow()+newRows.size()> maxRow())
		rowRealloc(nRow()+newRows.size());

	initPostOpt();
	_addRows(newRows);

}


//void LP::rowRealloc(int newSize)
//{
//	_rowRealloc(newSize);
//}


//void LP::remCols(ArrayBuffer<int> &cols)
//{
//	initPostOpt();
//	_remCols(cols);
//}


void LP::addCols(ArrayBuffer<Column*> &newCols)
{

	if(nCol()+newCols.size()> maxCol())
		colRealloc(nCol()+newCols.size());

	initPostOpt();
	_addCols(newCols);
}


//void LP::colRealloc(int newSize)
//{
//	_colRealloc(newSize);
//}


//void LP::changeRhs(Array<double> &newRhs)
//{
//	initPostOpt();
//
//	_changeRhs(newRhs);
//}


void LP::changeLBound(int i,double newLb)
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif

	initPostOpt();

	_changeLBound(i,newLb);
}


void LP::changeUBound(int i,double newUb)
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif

	initPostOpt();

	_changeUBound(i,newUb);
}


int LP::pivotSlackVariableIn(ArrayBuffer<int> &rows)
{
	initPostOpt();

	return _pivotSlackVariableIn(rows);
}


//int LP::getInfeas(int&infeasRow,int&infeasCol,double*bInvRow)
//{
//	return _getInfeas(infeasRow,infeasCol,bInvRow);
//}


void LP::colsNnz(int nRow, Array<Row*> &rows, Array<int> &nnz)
{
	Row *row;
	int i, r;

	nnz.fill(0);

	for (r = 0; r < nRow; r++){
		row = rows[r];
		int rowNnz = row->nnz();

		for (i = 0; i < rowNnz; i++)
			nnz[row->support(i)]++;
	}
}


void LP::rows2cols(
	int nRow,
	Array<Row*> &rows,
	Array<SparVec*> &cols)
{
	Row*row;
	int i,r;

	for(r= 0;r<nRow;r++){
		row= rows[r];

		const int rowNnz= row->nnz();

		for(i= 0;i<rowNnz;i++)
			cols[row->support(i)]->insert(r,row->coeff(i));
	}
}


void LP::rowRangeCheck(int r)const
{
	if(r < 0 || nRow() <= r) {
		int _r = nRow()-1;
		Logger::ifout() << "LP::rowRangeCheck(" << r << "): range of rows\n0 ... " << _r << " violated.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Lp);
	}
}


void LP::colRangeCheck(int i)const
{
	if(i < 0 || nCol() <= i) {
		int _c = nCol()-1;
		Logger::ifout() << "LP::colRangeCheck(" << i << "): range of columns\n0 ... " << _c << " violated.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Lp);
	}
}


std::ostream&operator<<(std::ostream&out, const LP&rhs)
{
	// LP: \a operator<<: local variables
	char sign;
	const double eps= rhs.master_->machineEps();

	// output the objective function
	/* The objective function is written in the form
	*   {\tt min 3.1 x0 + 4 x2}. Variables with coefficient 0 are not output.
	*   We also suppress the output of a \f$+\f$ before the first coefficient
	*   and the output of coefficients with value \f$1.0\f$.
	*/
	out<<rhs.sense()<<' ';

	int j = 0;
	for(int i = 0; i < rhs.nCol(); i++){
		double c = rhs.obj(i);
		if(c < -eps || c > eps) {
			if( c < 0.0){
				sign = '-';
				c = -c;
			}
			else sign = '+';

			if(j > 0 || sign == '-')
				out << sign << ' ';
			//suppress output of spaces when coefficients are zero
			//if(!(c<1.0-eps||1.0+eps<c))
			out << c << " x" << i << ' ';
			j++;
		}
		if( j && (j % 10 == 0) ) {
			out << std::endl;
			j = 1;
		}
	}
	out << std::endl;

	out << "s.t." << std::endl;

	// output the constraints
	/* The constraints of the LP are output row by row.
	*/
	Row row(rhs.master_,rhs.nCol());

	for(int i = 0; i < rhs.nRow(); i++) {
		rhs.row(i,row);
		out << "(" << i << "): " << row << std::endl;
	}

	// output the bounds
	/* The bounds are written in the form {\tt 0 <= x0 <= 2.3}. *//*:55*/

	out << "Bounds" << std::endl;
	for(int i = 0; i < rhs.nCol(); i++)
		out << rhs.lBound(i) << " <= x" << i << " <= " << rhs.uBound(i) << std::endl;

	out << "End" << std::endl;
	// output the solution of the linear program
	/* Finally the status of optimization of the LP is output, together
	*   with the value of the optimal solution if it is available.
	*/
	switch(rhs.optStat_){
	case LP::Unoptimized:
		out << "No solution available";
		break;
	case LP::Error:
		out << "Optimization failed";
		break;
	case LP::Optimal:
		out << "Optimum value: "<<rhs.value();
		break;
	case LP::Feasible:
		out << "Primal feasible value: "<<rhs.value();
		break;
	case LP::Infeasible:
		out << "Problem primal infeasible";
		break;
	case LP::Unbounded:
		out << "Problem unbounded";
		break;
	default:
		Logger::ifout() << "operator<<(AbaOStream&, const LP&): Unknown LP::Status!\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpStatus);
	}
	out << std::endl;

	return out;
}


int LP::writeBasisMatrix(const char*fileName)
{
	if(optStat_ != Optimal || slackStatus_ == Missing || basisStatus_ == Missing)
		return 1;

	// open the file for writing the basis
	std::ofstream file(fileName);
	if(!file) return 0;

	// mark the basic variables

	// mark the basic structural variables

	Array<bool> basicCol(nCol());
	Array<int> basisIndexCol(nCol());
	int nBasic= 0;

	for(int i = 0; i < nCol(); i++)
		if(lpVarStat(i) == LPVARSTAT::Basic) {
			basicCol[i] = true;
			basisIndexCol[i] = nBasic;
			nBasic++;
		}
		else
			basicCol[i] = false;

	//! mark the basic slack variables

	Array<int> basisIndexRow(nRow());
	for(int i = 0; i < nRow(); i++) {
		if(slackStat(i) == SlackStat::Basic) {
			basisIndexRow[i] = nBasic;
			nBasic++;
		}
	}

	// check the number of the basic variables

	if(nBasic != nRow()) {
		int _nR = nRow();
		Logger::ifout() << "number of basic variables " << nBasic << " != number of rows " << _nR << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Lp);
	}

	// write the basis row by row
	file << nRow() << std::endl;

	Row sparseRow(master_,nCol());

	for(int i = 0; i < nRow(); i++) {
		row(i,sparseRow);
		int nBasicInRow = 0;
		const int sparseRowNnz = sparseRow.nnz();

		for(int j = 0; j < sparseRowNnz; j++)
			if(basicCol[sparseRow.support(j)])
				nBasicInRow++;
		if(slackStat(i) == SlackStat::Basic)
			nBasicInRow++;

		file << i << ' ' << nBasicInRow << ' ';
		for(int j = 0; j < sparseRowNnz; j++) {
			if(basicCol[sparseRow.support(j)]){
				file << basisIndexCol[sparseRow.support(j)] << ' ';
				file << sparseRow.coeff(j) << ' ';
			}
		}
		if(slackStat(i) == SlackStat::Basic)
			file << basisIndexRow[i] << " 1";
		file << std::endl;
	}

	return 0;
}
}
