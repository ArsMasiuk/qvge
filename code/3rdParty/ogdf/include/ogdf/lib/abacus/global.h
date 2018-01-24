/*!\file
 * \author Matthias Elf
 * \brief global.
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

#pragma once

#include <ogdf/lib/abacus/abacusroot.h>
#include <ogdf/lib/abacus/hash.h>

namespace abacus {


//! Global data and functions.
/**
 * This class stores global data (e.g., a zero tolerance,
 * an output stream, a table with system parameters)
 * und functions operating with this data. For each application
 * there is usually one global object and almost every object
 * in this system has a pointer to an associated global object
 * or a pointer to an object of a class derived from AbacusGlobal
 * (e.g., Master).
 *
 * Like the class AbacusRoot, the class AbacusGlobal helps us to avoid
 * names with global scope.
 *
 * We assume that a set of parameters is associated with every application.
 * These parameters can either be given by ABACUS system classes or can
 * be user defined. The class AbacusGlobal contains a table for the storage
 * of the literal values of the parameters. Moreover, functions are
 * provided for assigning the literal valued parameters to specific types.
 */
class OGDF_EXPORT AbacusGlobal : public AbacusRoot {
public:

	//! The constructor.
	/**
	 * Initializes our filtered output and error stream with the standard output
	 * stream \a std::cout  and the standard error stream \a std::cerr.
	 *
	 * \param eps        The zero-tolerance used within all member functions of objects
	 *                   which have a pointer to this global object (default value 1.0e-4).
	 * \param machineEps The machine dependent zero tolerance (default value 1.0e-7).
	 * \param infinity   All values greater than \a infinity are regarded as "infinite big",
	 *                   all values less than \a -infinity are regarded as "infinite small"
	 *                   (default value 1.0e32).
	 *                   Please note that this value might be different from the value the
	 *                   LP-solver uses internally. You should make sure that the value used here is
	 *                   always greater than or equal to the value used by the solver.
	 */
	AbacusGlobal(
		double eps        = 1.0e-4,
		double machineEps = 1.0e-7,
		double infinity   = 1.0e32) 	:
		eps_(eps),
		machineEps_(machineEps),
		infinity_(infinity),
		paramTable_(100)
	{
	}

	//! The destructor.
	virtual ~AbacusGlobal() {};

	//! The output operator writes some of the data members to an ouput stream \a out.
	/**
	 * \param out The output stream.
	 * \param rhs The object being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend OGDF_EXPORT std::ostream &operator<<(std::ostream &out, const AbacusGlobal &rhs);

#if 0
	//! Writes \a nTab tabs to the output stream.
	/**
	 * This tabulator is not the normal tabulator but consists of four blanks.
	 *
	 * \return A reference to the global output stream.
	 *
	 * \param nTab The number of tabulators which should be written to the
	 *             global output stream. The default value is 0.
	 */
#if 0
	virtual AbaOStream& lout() const;
#endif

	//! Writes \a nTab tabs to the error stream.
	/**
	 * This tabulator is not the normal tabulator but consists of four blanks.
	 *
	 * \return A reference to the global output stream.
	 *
	 * \param nTab The number of tabulators which should be written to the
	 *             global error stream. The default value is 0.
	 */
#if 0
	virtual AbaOStream& fout() const;
#endif
#endif

	//! Returns the zero tolerance.
	double eps() const { return eps_; }

	//! Sets the zero tolerance to \a e.
	/**
	 * \param e The new value of the zero tolerance.
	 */
	void eps(double e) { eps_ = e; }

	//! Provides a machine dependent zero tolerance.
	/**
	 * The machine dependent zero tolerance is used, e.g., to test if
	 * a floating point value is 0.
	 * This value is usually less than \a eps(), which provides, e.g.,
	 * a safety tolerance if a constraint is violated.
	 *
	 * \return The machine dependent zero tolerance.
	 */
	double machineEps() const { return  machineEps_; }

	//! Sets the machine dependent zero tolerance to \a e.
	/**
	 * \param e The new value of the machine dependent zero tolerance.
	 */
	void machineEps(double e) { machineEps_ = e; }

	//! Provides a floating point value of "infinite" size.
	/**
	 * Especially, we assume that \a -infinity() is the lower and \a infinity()
	 * is the upper bound of an unbounded variable in the linear program.
	 *
	 * \return A very large floating point number. The default value of \a infinity() is 1.0e32.
	 */
	double infinity() const {
		return infinity_;
	}

	//! Sets the "infinite value" to \a x.
	/**
	 * Please note that this value might be different from the value the LP-solver
	 * uses internally.
	 * You should make sure that the value used here is always greater than or
	 * equal to the value used by the solver.
	 *
	 * \param x The new value representing "infinity".
	 */
	void infinity(double x) { infinity_ = x; }


	//! Returns true if \a x is regarded as "infinite" large, false otherwise.
	/**
	 * \param x The value compared with "infinity".
	 */
	bool isInfinity(double x) const {
		return ( x >= infinity_ );
	}

	//! Returns true if \a x is regarded as infinite small, false otherwise.
	/**
	 * \param x The value compared with "minus infinity".
	 */
	bool isMinusInfinity(double x) const {
		return ( x <= -infinity_ );
	}

	//! Returns whether the absolute difference between \a x and \a y is less than the machine dependent zero tolerance.
	/**
	 * \param x The first value being compared.
	 * \param y The second value being compared.
	 */
	bool equal(double x, double y) const {
		return ( fabs(x-y) < machineEps_ );
	}


	//! Returns whether the value \a x differs at most by the machine dependent zero tolerance from an integer value.
	bool isInteger(double x) const {
		return isInteger(x, machineEps_);
	}

	//! Returns whether the value \a x differs at most by \a eps from an integer value.
	bool isInteger(double x, double eps) const;

	//! Opens the parameter file \a fileName, reads all parameters, and inserts them in the parameter table.
	/**
	 * A parameter file may have at most 1024 characters per line.
	 *
	 * \param fileName The name of the parameter file.
	 */
	void readParameters(const string &fileName);

	//! Inserts parameter \a name with value \a value into the parameter table.
	/**
	 * If the parameter is already in the table, the value is overwritten.
	 *
	 * \param name  The name of the parameter.
	 * \param value The value of the parameter.
	 */
	void insertParameter(const char *name, const char *value);

	//! \brief Searches for parameter \a name in the parameter table and returns its value in \a param.
	/**
	 * This function is overloaded for different types of the argument \a parameter.
	 * See also the functions \a assignParameter and \a findParameter with
	 * enhanced functionality.
	 *
	 * \return 0 If the parameter is found, 1 otherwise.
	 *
	 * \param name  The name of the parameter.
	 * \param param The variable \a parameter receives the value of the parameter, if the
	 *              function returns 1, otherwise it is undefined.
	 */
	int getParameter(const char *name, int          &param) const;
	int getParameter(const char *name, unsigned int &param) const;
	int getParameter(const char *name, double       &param) const;
	int getParameter(const char *name, string       &param) const;
	int getParameter(const char *name, bool         &param) const;
	int getParameter(const char *name, char         &param) const;

	//! \brief Searches for parameter \a name in the parameter table and returns its value in \a param.
	/**
	 * If no parameter \a name is found and no default value
	 * of the parameter is given, the program terminates with an error messages.
	 * The program terminates also with an error message if the value of a
	 * parameter is not within a specified feasible region.
	 * Depending on the type of the parameter, a feasible region can be an
	 * interval (specified by \a minVal and \a maxVal) or can be given by
	 * a set of feasible settings (given by a number \a nFeasible and a pointer
	 * \a feasible to the feasible values.
	 *
	 * This function is overloaded in two ways. First, this function is defined
	 * for different types of  the argument \a parameter,  second, for each such
	 * type we have both versions, with and without a default value of
	 * the parameter.
	 *
	 * \param param  The variable \a parameter receives the value of the parameter.
	 * \param name   The name of the parameter.
	 * \param minVal The value of the parameter is considered as infeasible if it
	 *               is less than \a minVal.
	 * \param maxVal The value of the parameter is considered as infeasible if it
	 *               is larger than \a maxVal.
	 */
	void assignParameter (
		int &param,
		const char *name,
		int minVal,
		int maxVal) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	void assignParameter (
		unsigned &param,
		const char *name,
		unsigned minVal,
		unsigned maxVal) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	void assignParameter (
		double &param,
		const char *name,
		double minVal,
		double maxVal) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	void assignParameter(bool &param, const char *name) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	/**
	 * \param param     The variable \a parameter receives the value of the parameter.
	 * \param name      The name of the parameter.
	 * \param nFeasible The number of feasible settings. If \a nFeasible is equal to
	 *                  zero, then all values are allowed. 0 is the default value.
	 * \param feasible  If \a nFeasible is greater zero, the this are the settings
	 *                  for the parameter to be considered as feasible. Must be an
	 *                  array of \a nFeasible strings.
	 */
	void assignParameter(
		string &param,
		const char *name,
		unsigned nFeasible = 0,
		const char *feasible[] = nullptr) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	/**
	 * \param param    The variable \a param receives the value of the parameter.
	 * \param name     The name of the parameter.
	 * \param feasible A string consisting of all feasible characters.
	 *                 If \a feasible is zero, then all characters are allowed.
	 */
	void assignParameter(char &param, const char *name,
		const char *feasible=nullptr) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	/**
	 * \param param  The variable \a parameter receives the value of the parameter.
	 * \param name   The name of the parameter.
	 * \param minVal The  value of the parameter is considered as infeasible
	 *               if it is less than \a minVal.
	 * \param maxVal The  value of the parameter is considered as infeasible
	 *               if it is larger than \a maxVal.
	 * \param defVal The default value that is used when the paramter is
	 *               not found in the parameter table.
	 */
	void assignParameter(
		int &param,
		const char *name,
		int minVal,
		int maxVal,
		int defVal) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	void assignParameter(
		unsigned &param,
		const char *name,
		unsigned minVal,
		unsigned maxVal,
		unsigned defVal) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	void assignParameter(
		double &param,
		const char *name,
		double minVal,
		double maxVal,
		double defVal) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	void assignParameter(bool &param, const char *name, bool defVal) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	/**
	 * \param param     The variable \a parameter receives the value of the parameter.
	 * \param name      The name of the parameter.
	 * \param nFeasible The number of feasible settings. If \a nFeasible is
	 *                  equal to zero, then all settings are allowed.
	 * \param feasible  The settings for the parameter to be considered as
	 *                  feasible. Must be an array of \a nFeasible strings.
	 * \param defVal    The default value that is used when the paramter is
	 *                  not found in the parameter table.
	 */
	void assignParameter(
		string &param,
		const char *name,
		unsigned nFeasible,
		const char *feasible[],
		const char *defVal) const;

	//! See AbacusGlobal::assignParameter(int&,const char*,int,int) for a description.
	/**
	 * \param param    The variable \a param receives the value of the parameter.
	 * \param name     The name of the parameter.
	 * \param feasible A string containing all feasible settings. If \a feasible is zero, then all settings are allowed.
	 * \param defVal   The default value that is used when the paramter is not found in the parameter table.
	 */
	void assignParameter(char &param, const char *name,
		const char *feasible, char defVal) const;

	//! Searches for parameter \a name in the parameter table.
	/**
	 * If no parameter \a name is found the function throws an exception.
	 * The function also throws an exception if the value of a
	 * parameter is not within a given list of feasible settings.
	 * This function is overloaded and can be used for different types of
	 * parameters such as integer valued, char valued and string parameters.
	 *
	 * \param name      The name of the parameter.
	 * \param nFeasible The number of feasible settings.
	 * \param feasible  The settings for the parameter to be considered as
	 *                  feasible. Must be an array of \a nFeasible strings.
	 *
	 * \return The index of the matched feasible setting.
	 */
	int findParameter(const char *name, unsigned nFeasible, const int *feasible) const;

	//! See AbacusGlobal::findParameter(const char *name, unsigned nFeasible, const int *feasible) for description.
	int findParameter(const char *name,
		unsigned nFeasible, const char *feasible[]) const;

	//! See AbacusGlobal::findParameter(const char *name, unsigned nFeasible, const int *feasible) for description.
	int findParameter(const char *name,const char *feasible) const;

private:

	double eps_; //!< A zero tolerance.

	//! The machine dependent zero tolerance, which is used to , e.g., to test if a floating point value is 0.
	/**
	 * This value is usually less than \a eps_, which represents, e.g.,
	 * a safety tolerance if a constraint is violated.
	 */
	double machineEps_;

	double infinity_; //!< An "infinite" big number.

	AbaHash<string, string> paramTable_;

	AbacusGlobal(const AbacusGlobal &rhs);
	const AbacusGlobal &operator=(const AbacusGlobal &rhs);
};

}
