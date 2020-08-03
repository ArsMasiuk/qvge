/** \file
 * \brief Definition of exception classes
 *
 * \author Carsten Gutwenger, Markus Chimani
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/basic/basic.h>
#include <ogdf/basic/Logger.h>

namespace ogdf {

//! @name Throwing exceptions
//! @{

//! Flushes some output streams
#define OGDF_FLUSH_OUTPUTS \
	std::cout << std::flush, ::ogdf::Logger::sfout() << std::flush

/**
 * Replacement for \c throw.
 * This macro is used to throw an exception and pass the file name
 * and line number of the location in the source file (in Debug mode only).
 * @param CLASS is the name of the exception class.
 * @param PARAM is an additional parameter (like the error code) required
 *        by the exception calls.
 * @ingroup macros
 */
#define OGDF_THROW_PARAM(CLASS, PARAM) \
	OGDF_FLUSH_OUTPUTS, throw CLASS(PARAM)

/**
 * Replacement for \c throw.
 * This macro is used to throw an exception and pass the file name
 * and line number of the location in the source file (in Debug mode only).
 * @param CLASS is the name of the exception class.
 * @ingroup macros
 */
#define OGDF_THROW(CLASS) \
	OGDF_FLUSH_OUTPUTS, throw CLASS()

#ifdef OGDF_DEBUG
# undef OGDF_THROW_PARAM
# define OGDF_THROW_PARAM(CLASS, PARAM) \
	OGDF_FLUSH_OUTPUTS, throw CLASS(PARAM, __FILE__, __LINE__)
# undef OGDF_THROW
# define OGDF_THROW(CLASS) \
	OGDF_FLUSH_OUTPUTS, throw CLASS(__FILE__, __LINE__)
#endif

//! @}

//! Code for an internal failure condition
/**
 * @ingroup exceptions
 *
 * \see AlgorithmFailureException
 */
enum class AlgorithmFailureCode {
	Unknown,
	IllegalParameter, //!< function parameter is illegal
	NoFlow,           //!< min-cost flow could not find a legal flow
	Sort,             //!< sequence not sorted
	Label,            //!< labelling failed
	ExternalFace,     //!< external face not correct
	ForbiddenCrossing,//!< crossing forbidden but necessary
	TimelimitExceeded,//!< it took too long
	NoSolutionFound,  //!< couldn't solve the problem
	IndexOutOfBounds, //!< index out of bounds

	// The following codes are used by Abacus (think about changing them to
	// more error describing codes)
	PrimalBound,
	DualBound,
	NotInteger,
	Buffer,
	AddVar,
	Sorter,
	Phase,
	Active,
	NoSolution,
	MakeFeasible,
	Guarantee,
	BranchingVariable,
	Strategy,
	CloseHalf,
	StandardPool,
	Variable,
	LpIf,
	Lp,
	Bstack,
	LpStatus,
	BranchingRule,
	FixSet,
	LpSub,
	String,
	Constraint,
	Pool,
	Global,
	FsVarStat,
	LpVarStat,
	OsiIf,
	ConBranchRule,
	Timer,
	Array,
	Csense,
	BPrioQueue,
	FixCand,
	BHeap,
	Poolslot,
	SparVec,
	Convar,
	Ostream,
	Hash,
	Paramaster,
	InfeasCon,

	STOP              // INSERT NEW CODES BEFORE afcSTOP!
};

//! Code for the library which was intended to get used, but its use is not supported.
/**
 * @ingroup exceptions
 * \see LibraryNotSupportedException
 */
enum class LibraryNotSupportedCode {
	Unknown,
	Coin,                          //!< COIN not supported
	Abacus,                        //!< ABACUS not supported
	FunctionNotImplemented,        //!< the used library doesn't support that function
	MissingCallbackImplementation, //
	STOP                           // INSERT NEW CODES BEFORE nscSTOP!
};

//! Base class of all ogdf exceptions.
/**
 * @ingroup exceptions
 */
class OGDF_EXPORT Exception {

private:

	const char *m_file; //!< Source file where exception occurred.
	int         m_line; //!< Line number where exception occurred.

public:
	//! Constructs an exception.
	/**
	 * @param file is the name of the source file where exception was thrown.
	 * @param line is the line number in the source file where the exception was thrown.
	 */
	explicit Exception(const char *file = nullptr, int line = -1) :
		m_file(file),
		m_line(line)
		{ }

	//! Returns the name of the source file where exception was thrown.
	/**
	 * Returns a null pointer if the name of the source file is unknown.
	 */
	const char *file() const { return m_file; }

	//! Returns the line number where the exception was thrown.
	/**
	 * Returns -1 if the line number is unknown.
	 */
	int line() const { return m_line; }
};


//! %Exception thrown when result of cast is 0.
/**
* @ingroup exceptions
*/
class OGDF_EXPORT DynamicCastFailedException : public Exception {

public:
	//! Constructs a dynamic cast failed exception.
	explicit DynamicCastFailedException(const char *file = nullptr, int line = -1) : Exception(file, line) {}
};


//! %Exception thrown when not enough memory is available to execute an algorithm.
/**
* @ingroup exceptions
*/
class OGDF_EXPORT InsufficientMemoryException : public Exception {

public:
	//! Constructs an insufficient memory exception.
	explicit InsufficientMemoryException(const char *file = nullptr, int line = -1) : Exception(file, line) {}
};


//! %Exception thrown when a required standard comparer has not been specialized.
/**
 * @ingroup exceptions
 *
 * The default implementation of StdComparer<E> throws this exception, since it
 * provides no meaningful implementation of comparer methods. You need to specialize
 * this class for the types you want to use with sorting and searching methods (like
 * quicksort and binary search).
 */
class OGDF_EXPORT NoStdComparerException : public Exception {

public:
	//! Constructs a no standard comparer available exception.
	explicit NoStdComparerException(const char *file = nullptr, int line = -1) : Exception(file, line) {}
};


//! %Exception thrown when a data type is not supported by a generic function.
/**
* @ingroup exceptions
*/
class OGDF_EXPORT TypeNotSupportedException : public Exception {

public:
	//! Constructs a type-not-supported exception.
	explicit TypeNotSupportedException(const char *file = nullptr, int line = -1) : Exception(file, line) {}
};

//! %Exception thrown when an algorithm realizes an internal bug that prevents it from continuing.
/**
* @ingroup exceptions
*/
class OGDF_EXPORT AlgorithmFailureException : public Exception
{
public:

	//! Constructs an algorithm failure exception.
	explicit AlgorithmFailureException(AlgorithmFailureCode code,
		const char *file = nullptr,
		int line = -1) :
	Exception(file, line),
	m_exceptionCode(code)
	{}

	//! Constructs an algorithm failure exception.
	explicit AlgorithmFailureException(
		const char *file = nullptr,
		int line = -1) :
	Exception(file, line),
	m_exceptionCode(AlgorithmFailureCode::Unknown)
	{}

	//! Returns the error code of the exception.
	AlgorithmFailureCode exceptionCode() const { return m_exceptionCode; }

private:
	AlgorithmFailureCode m_exceptionCode; //!< The error code specifying the exception.
};

//! %Exception thrown when an external library shall be used which is not supported.
/**
* @ingroup exceptions
*/
class OGDF_EXPORT LibraryNotSupportedException : public Exception {
	public:
	//! Constructs a library not supported exception.
		explicit LibraryNotSupportedException(LibraryNotSupportedCode code,
			const char *file = nullptr,
			int line = -1) :
		Exception(file, line),
		m_exceptionCode(code)
		{}

	//! Constructs a library not supported exception.
		explicit LibraryNotSupportedException(
			const char *file = nullptr,
			int line = -1) :
		Exception(file, line),
		m_exceptionCode(LibraryNotSupportedCode::Unknown)
		{}

	//! Returns the error code of the exception.
	LibraryNotSupportedCode exceptionCode() const { return m_exceptionCode; }

private:
	LibraryNotSupportedCode m_exceptionCode; //!< The error code specifying the exception.
};

}
