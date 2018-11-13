#ifndef BANDIT_BANDIT_H
#define BANDIT_BANDIT_H

// Visual Studio versions before 2015 do not support the noexcept keyword
#ifdef _MSC_VER
#  if (_MSC_VER < 1900)
#    define _ALLOW_KEYWORD_MACROS
#    define noexcept
#  endif
#endif

#include <bandit/grammar.h>
#include <bandit/runner.h>

#endif
