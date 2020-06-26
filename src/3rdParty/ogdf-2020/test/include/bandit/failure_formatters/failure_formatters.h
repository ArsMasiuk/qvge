#ifndef BANDIT_FAILURE_FORMATTERS
#define BANDIT_FAILURE_FORMATTERS

#include "failure_formatter.h"
#include "default_failure_formatter.h"
#include "visual_studio_failure_formatter.h"

namespace bandit {
  namespace detail {
    inline failure_formatter& registered_failure_formatter() {
      static default_failure_formatter formatter;
      return formatter;
    }
  }
}
#endif
