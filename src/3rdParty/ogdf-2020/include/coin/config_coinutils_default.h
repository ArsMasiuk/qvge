
/***************************************************************************/
/*           HERE DEFINE THE PROJECT SPECIFIC PUBLIC MACROS                */
/*    These are only in effect in a setting that doesn't use configure     */
/***************************************************************************/

/* Version number of project */
#define COINUTILS_VERSION        "2.8"

/* Major Version number of project */
#define COINUTILS_VERSION_MAJOR      2

/* Minor Version number of project */
#define COINUTILS_VERSION_MINOR      8

/* Release Version number of project */
#define COINUTILS_VERSION_RELEASE 9999

/* Use 64-bit integer type provided by Microsoft */
#ifdef _MSC_VER
# define COIN_INT64_T __int64
# define COIN_UINT64_T unsigned __int64
  /* Define to integer type capturing pointer */
# define COIN_INTPTR_T intptr_t
#else
# define COIN_INT64_T long long
# define COIN_UINT64_T unsigned long long
# define COIN_INTPTR_T int*
#endif
