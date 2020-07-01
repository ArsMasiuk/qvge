/** \file
 * \brief Basic configuration file
 *
 * \author Carsten Gutwenger
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

#include <ogdf/basic/internal/version.h>
#include <ogdf/basic/internal/config_autogen.h>
#include <iostream>
#include <string>

#if defined(OGDF_DEBUG) && defined(NDEBUG)
# error "Contradicting configuration: Macros OGDF_DEBUG and NDEBUG are defined."
#endif

namespace ogdf {

// generally used <string> members
using std::string;
using std::to_string;

// detection of the system

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(_AIX) || defined(__APPLE__)
#define OGDF_SYSTEM_UNIX
#endif

#if defined(__WIN32__) || defined(_WIN32) || defined(__NT__)
#define OGDF_SYSTEM_WINDOWS
#endif

// Note: Apple OS X machines will be both OGDF_SYSTEM_UNIX and OGDF_SYSTEM_OSX
#if defined(__APPLE__)
#define OGDF_SYSTEM_OSX
#endif

// C++11 standard

#if __cplusplus < 201103

#if defined(_MSC_VER)
#if _MSC_VER < 1700
#error "Compiling OGDF requires Visual C++ 11 (Visual Studio 2012) or higher!"
#endif

#elif defined(__GNUC__)
#ifndef __GXX_EXPERIMENTAL_CXX0X__
#error "No C++11 support activated for g++ (compile with -std=c++0x or -std=c++11)!"
#endif

#else
#error "Compiling OGDF requires a C++11 compliant compiler!"
#endif

#endif

#ifdef __has_cpp_attribute
# define OGDF_HAS_CPP_ATTRIBUTE(x) (__has_cpp_attribute(x) && __cplusplus >= __has_cpp_attribute(x))
#else
# define OGDF_HAS_CPP_ATTRIBUTE(x) 0
#endif

//! @name Important when compiling OGDF as DLL
//! @ingroup macros
//! @{

/**
 * Specifies that a function or class is exported by the OGDF DLL.
 * It is set according to the definition of OGDF_INSTALL (OGDF is build as DLL) and OGDF_DLL (OGDF is used as DLL).
 * If none of these are defined (OGDF is build or used as static library), the define expands to nothing.
 */
#define OGDF_EXPORT

#ifdef OGDF_SYSTEM_WINDOWS
# ifdef OGDF_DLL
#  undef OGDF_EXPORT
#  ifdef OGDF_INSTALL
#   define OGDF_EXPORT __declspec(dllexport)
#  else
#   define OGDF_EXPORT __declspec(dllimport)
#  endif
# endif
#endif

//! @}
//! @name Deprecation
//! @{

//! Mark a class / member / function as deprecated
//! @ingroup macros
#define OGDF_DEPRECATED(reason)

#if OGDF_HAS_CPP_ATTRIBUTE(deprecated)
# undef OGDF_DEPRECATED
# define OGDF_DEPRECATED(reason) [[deprecated(reason)]]
#elif defined(_MSC_VER)
# undef OGDF_DEPRECATED
# define OGDF_DEPRECATED(reason) __declspec(deprecated(reason))
#elif defined(__GNUC__)
# undef OGDF_DEPRECATED
# define OGDF_DEPRECATED(reason) __attribute__((deprecated(reason)))
#endif

//! @}

//! @name Optimization
//! @{

/**
 * Specify the likely branch in a condition.
 * Usage: \code if (OGDF_LIKELY(i >= 0)) { likely branch } else { unlikely branch } \endcode
 * @ingroup macros
 */
#define OGDF_LIKELY(x)    (x)

/**
 * Specify the unlikely branch in a condition.
 * Usage: \code if (OGDF_UNLIKELY(set.empty())) { unlikely branch } else { likely branch } \endcode
 * @ingroup macros
 */
#define OGDF_UNLIKELY(x)  (x)

//! Specify the minimum alignment (in bytes) of a type to be \p b. This is used in type declarations.
//! @ingroup macros
#define OGDF_DECL_ALIGN(b)

#ifdef _MSC_VER // Visual C++ compiler
# undef OGDF_DECL_ALIGN
# define OGDF_DECL_ALIGN(b) __declspec(align(b))
#elif defined(__GNUC__) // GNU gcc compiler (also Intel compiler)
# undef OGDF_LIKELY
# define OGDF_LIKELY(x)    __builtin_expect((x),1)
# undef OGDF_UNLIKELY
# define OGDF_UNLIKELY(x)  __builtin_expect((x),0)
# undef OGDF_DECL_ALIGN
# define OGDF_DECL_ALIGN(b) __attribute__ ((aligned(b)))
#endif

//! @}

//! An attribute to mark cases (in switch) that fall through to the next case
#define OGDF_CASE_FALLTHROUGH
#if OGDF_HAS_CPP_ATTRIBUTE(fallthrough)
# undef OGDF_CASE_FALLTHROUGH
# define OGDF_CASE_FALLTHROUGH [[fallthrough]]
#elif defined(__GNUC__) && __GNUC__ >= 7
# undef OGDF_CASE_FALLTHROUGH
# define OGDF_CASE_FALLTHROUGH __attribute__((fallthrough))
#endif

// compiler adaptions

#ifdef _MSC_VER

#ifdef OGDF_DLL
// disable useless warnings
// missing dll-interface

// warning C4251: 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
#pragma warning(disable : 4251)
// warning C4275: non-DLL-interface classkey 'identifier' used as base for DLL-interface classkey 'identifier'
#pragma warning(disable : 4275)
#endif

// warning C4355: 'this' : used in base member initializer list
#pragma warning (disable : 4355)

#endif

//! Provides information about how OGDF has been configured.
/**
* @ingroup system
*/
class OGDF_EXPORT Configuration {
public:
	//! Specifies the operating system for which OGDF has been configured/built.
	enum class System {
		Unknown, //!< not known (inproper configuration)
		Windows, //!< Windows
		Unix,    //!< Unix/Linux
		OSX,     //!< Apple OSX
		STOP
	};

	//! Specifies the LP-solver used by OGDF.
	enum class LPSolver {
		None,     //!< no LP-solver available
		Clp,      //!< COIN-OR LP-solver (Clp)
		Symphony, //!< Symphony
		CPLEX,    //!< CPLEX (commercial)
		Gurobi,   //!< Gurobi (commercial)
		STOP
	};

	//! Specifies the memory-manager used by OGDF.
	enum class MemoryManager {
		PoolTS, //!< thread-safe pool allocator
		PoolNTS, //!< non-thread-safe pool allocator
		Malloc, //!< malloc/free allocator
		STOP
	};

	//! Returns the operating system for which OGDF has been configured.
	static constexpr System whichSystem() {
#ifdef OGDF_SYSTEM_WINDOWS
		return System::Windows;
#elif defined(OGDF_SYSTEM_OSX)
		return System::OSX;
#elif defined(OGDF_SYSTEM_UNIX)
		return System::Unix;
#else
		return System::Unknown
#endif
	}

	//! Returns whether OGDF has been configured with LP-solver support.
	/**
	 * Since COIN and ABACUS are required and shipped, this function
	 * always returns true.
	 */
	OGDF_DEPRECATED("OGDF always has LP solver support since 2015.05")
	static constexpr bool haveLPSolver() {
		return true;
	}

	//! Returns the LP-solver used by OGDF.
	static constexpr LPSolver whichLPSolver() {
#if defined(COIN_OSI_CLP)
		return LPSolver::Clp;
#elif defined(COIN_OSI_SYM)
		return LPSolver::Symphony;
#elif defined(COIN_OSI_CPX)
		return LPSolver::CPLEX;
#elif defined(COIN_OSI_GRB)
		return LPSolver::Gurobi;
#else
# error "OGDF is compiled without LP solver. Check your build configuration."
#endif
	}

	//! Returns whether OGDF has been configured with COIN support.
	/**
	 * COIN is used as LP solver by some OGDF algorithms.
	 * In former versions, OGDF could be configured without COIN support,
	 * so this functionality was not available.
	 * Now this function always returns true.
	 */
	OGDF_DEPRECATED("OGDF always has COIN-OR since 2015.05")
	static constexpr bool haveCoin() {
		return true;
	}

	//! Returns whether OGDF has been configured with ABACUS support.
	/**
	 * ABACUS is used as branch-and-cut-solver by some OGDF algorithms.
	 * In former versions, OGDF could be configured without ABACUS support,
	 * so this functionality was not available.
	 * Now this function always returns true.
	 */
	OGDF_DEPRECATED("OGDF always has ABACUS since 2015.05")
	static constexpr bool haveAbacus() {
		return true;
	}

	/**
	 * Returns the memory manager used by OGDF.
	 *
	 * The memory manager is configured using the build configuration.
	 * Depending on that, the following macros are set:
	 * - OGDF_MEMORY_POOL_TS: buffered-pool allocator per thread pool (thread-safe)
	 * - OGDF_MEMORY_POOL_NTS: pool allocator (not thread-safe)
	 * - OGDF_MEMORY_MALLOC_TS: just using malloc/free (thread-safe)
	 */
	static constexpr MemoryManager whichMemoryManager() {
#if defined(OGDF_MEMORY_POOL_TS)
		return MemoryManager::PoolTS;
#elif defined(OGDF_MEMORY_POOL_NTS)
		return MemoryManager::PoolNTS;
#elif defined(OGDF_MEMORY_MALLOC_TS)
		return MemoryManager::Malloc;
#else
# error "OGDF is compiled without memory manager. Check your build configuration."
#endif
	}

	//! Converts \p sys to a (readable) string.
	static const string &toString(System sys);

	//! Converts \p lps to a (readable) string.
	static const string &toString(LPSolver lps);

	//! Converts \p mm to a (readable) string.
	static const string &toString(MemoryManager mm);
};


//! Output operator for Configuration::System (uses Configuration::toString(Configuration::System)).
inline std::ostream &operator<<(std::ostream &os, Configuration::System sys)
{
	os << Configuration::toString(sys);
	return os;
}

//! Output operator for Configuration::LPSolver (uses Configuration::toString(Configuration::LPSolver)).
inline std::ostream &operator<<(std::ostream &os, Configuration::LPSolver lps)
{
	os << Configuration::toString(lps);
	return os;
}

//! Output operator for Configuration::MemoryManager (uses Configuration::toString(Configuration::MemoryManager)).
inline std::ostream &operator<<(std::ostream &os, Configuration::MemoryManager mm)
{
	os << Configuration::toString(mm);
	return os;
}

}
