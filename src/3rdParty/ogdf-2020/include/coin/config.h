//----------------------
// CLP
//

/* Version number of project */
#define CLP_VERSION "1.14.7"

/* Major Version number of project */
#define CLP_VERSION_MAJOR 1

/* Minor Version number of project */
#define CLP_VERSION_MINOR 14

/* Release Version number of project */
#define CLP_VERSION_RELEASE 7

/* Define to the debug sanity check level (0 is no test) */
#define COIN_CLP_CHECKLEVEL 0

/* Define to the debug verbosity level (0 is no output) */
#define COIN_CLP_VERBOSITY 0


//----------------------
// CoinUtils
//

/* Default maximum pooled allocation size */
#define COINUTILS_MEMPOOL_MAXPOOLED -1

/* Define to 1 CoinUtils should override global new/delete */
/* #undef COINUTILS_MEMPOOL_OVERRIDE_NEW */

/* Define to 1 if the thread aware version of CoinUtils should be compiled */
/* #undef COINUTILS_PTHREADS */

/* Version number of project */
#define COINUTILS_VERSION "2.8.7"

/* Major Version number of project */
#define COINUTILS_VERSION_MAJOR 2

/* Minor Version number of project */
#define COINUTILS_VERSION_MINOR 8

/* Release Version number of project */
#define COINUTILS_VERSION_RELEASE 7

/* Define to the debug sanity check level (0 is no test) */
#define COIN_COINUTILS_CHECKLEVEL 0

/* Define to the debug verbosity level (0 is no output) */
#define COIN_COINUTILS_VERBOSITY 0


//----------------------
// Osi
//

/* Define to the debug sanity check level (0 is no test) */
#define COIN_OSI_CHECKLEVEL 0

/* Define to the debug verbosity level (0 is no output) */
#define COIN_OSI_VERBOSITY 0


//----------------------
// Available Packages
//

/* Define to 1 if the Cgl package is available */
/* #undef COIN_HAS_CGL */

/* Define to 1 if the Clp package is available */
#define COIN_HAS_CLP 1

/* Define to 1 if the Osi package is available */
#define COIN_HAS_OSI 1

/* Define to 1 if the OsiClp package is available */
#define COIN_HAS_OSICLP 1

/* Define to 1 if the OsiCpx package is available */
/* #undef COIN_HAS_OSICPX */

/* Define to 1 if the OsiDyLP package is available */
/* #undef COIN_HAS_OSIDYLP */

/* Define to 1 if the OsiGlpk package is available */
/* #undef COIN_HAS_OSIGLPK */

/* Define to 1 if the OsiMsk package is available */
/* #undef COIN_HAS_OSIMSK */

/* Define to 1 if the OsiVol package is available */
/* #undef COIN_HAS_OSIVOL */

/* Define to 1 if the OsiXpr package is available */
/* #undef COIN_HAS_OSIXPR */

/* Define to 1 if the Cpx package is available */
/* #undef COIN_HAS_CPX */

/* Define to 1 if the DyLP package is available */
/* #undef COIN_HAS_DYLP */

/* Define to 1 if the Glpk package is available */
/* #undef COIN_HAS_GLPK */

/* Define to 1 if the Msk package is available */
/* #undef COIN_HAS_MSK */

/* Define to 1 if the Sample package is available */
/* #undef COIN_HAS_SAMPLE */

/* Define to 1 if the Vol package is available */
/* #undef COIN_HAS_VOL */

/* Define to 1 if the Xpr package is available */
/* #undef COIN_HAS_XPR */

/* If defined, the LAPACK Library is available. */
/* #define COIN_HAS_LAPACK 1 */

/* Define to 1 if zlib is available */
/* #define COIN_HAS_ZLIB 1 */

/* If defined, the BLAS Library is available. */
/* #define COIN_HAS_BLAS 1 */

/* Define to 1 if bzlib is available */
/* #define COIN_HAS_BZLIB 1 */

/* Define to 1 if the Glpk package is available */
/* #undef COIN_HAS_GLPK */


//----------------------
// Compiler Adaptions
//

/* Define to 1 if you have the <cfloat> header file. */
#define HAVE_CFLOAT 1

/* Define to 1 if you have the <cmath> header file. */
#define HAVE_CMATH 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to be the name of C-function for Inf check */
#define COIN_C_FINITE std::isfinite

/* Define to be the name of C-function for NaN check */
#define COIN_C_ISNAN std::isnan

/* Define to 64bit integer type */
#define COIN_INT64_T int64_t

/* Define to integer type capturing pointer */
#define COIN_INTPTR_T intptr_t

/* Define to 64bit unsigned integer type */
#define COIN_UINT64_T int64_t

/* Define to a macro mangling the given C identifier (in lower and upper
   case), which must not contain underscores, for linking with Fortran. */
#define F77_FUNC(name,NAME) name ## _

/* As F77_FUNC, but for C identifiers containing underscores. */
#define F77_FUNC_(name,NAME) name ## _
