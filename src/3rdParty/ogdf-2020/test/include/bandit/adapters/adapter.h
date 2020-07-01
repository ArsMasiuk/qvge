#ifndef BANDIT_ADAPTER_H
#define BANDIT_ADAPTER_H

#include <bandit/types.h>

namespace bandit {
  namespace adapters {
    struct assertion_adapter {
      virtual void adapt_exceptions(detail::voidfunc_t) = 0;
    };
  }
}
#endif
