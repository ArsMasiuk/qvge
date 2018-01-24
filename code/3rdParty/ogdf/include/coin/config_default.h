
/* include the COIN-OR-wide system specific configure header */
#include "configall_system.h"

/* include the public project specific macros */
#include "config_coinutils_default.h"

/***************************************************************************/
/*             HERE DEFINE THE PROJECT SPECIFIC MACROS                     */
/*    These are only in effect in a setting that doesn't use configure     */
/***************************************************************************/

/* If defined, debug sanity checks are performed during runtime */
/* #define COIN_DEBUG 1 */

/* Define to 1 if the Cbc package is used */
/* #define COIN_HAS_CBC 1 */

/* Define to 1 if the Cgl package is used */
/* #undef COIN_HAS_CGL */

/* Define to 1 if the Clp package is used */
#define COIN_HAS_CLP 1

/* Define to 1 if the CoinUtils package is used */
#define COIN_HAS_COINUTILS 1

/* Define to 1 if the Osi package is used */
#define COIN_HAS_OSI 1

/* Define to 1 if the Osi package is used */
/* #define COIN_HAS_OSITESTS 1 */

/* Define to 1 if the Vol package is used */
/* #define COIN_HAS_VOL 1 */

/* Define to 1 if the Cplex package is used */
/* #define COIN_HAS_CPX 1 */

/* Define to 1 if the Dylp package is used */
/* #undef COIN_HAS_DYLP */

/* Define to 1 if the FortMP package is used */
/* #undef COIN_HAS_FMP */

/* Define to 1 if the Glpk package is used */
/* #undef COIN_HAS_GLPK */

/* Define to 1 if the Mosek package is used */
/* #undef COIN_HAS_MSK */

/* Define to 1 if the Osl package is used */
/* #undef COIN_HAS_OSL */

/* Define to 1 if the Soplex package is used */
/* #undef COIN_HAS_SPX */

/* Define to 1 if the Sym package is used */
/* #undef COIN_HAS_SYM */

/* Define to 1 if the Xpress package is used */
/* #undef COIN_HAS_XPR */
