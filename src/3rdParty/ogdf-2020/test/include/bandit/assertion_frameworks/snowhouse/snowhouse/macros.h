#ifndef SNOWHOUSE_MACROS_H
#define SNOWHOUSE_MACROS_H
// clang-format off

#define SNOWHOUSE_MAJOR 3
#define SNOWHOUSE_MINOR 0
#define SNOWHOUSE_PATCH 0

#define SNOWHOUSE_TOSTRING(x) #x
#define SNOWHOUSE_MACROTOSTRING(x) SNOWHOUSE_TOSTRING(x)
#define SNOWHOUSE_VERSION \
  SNOWHOUSE_MACROTOSTRING(SNOWHOUSE_MAJOR) "." \
  SNOWHOUSE_MACROTOSTRING(SNOWHOUSE_MINOR) "." \
  SNOWHOUSE_MACROTOSTRING(SNOWHOUSE_PATCH)

#if __cplusplus > 199711L
// Visual Studio (including 2013) does not support the noexcept keyword
# ifdef _MSC_VER
#  define _ALLOW_KEYWORD_MACROS
#  define noexcept
# endif
#endif

#if __cplusplus > 199711L || (defined(_MSC_VER) && _MSC_VER >= 1600)
# include <cstddef>
# define SNOWHOUSE_HAS_NULLPTR
#endif

#endif
