#ifndef BANDIT_ADAPTERS_H
#define BANDIT_ADAPTERS_H

#include <bandit/adapters/snowhouse.h>

namespace bandit {
  namespace detail {
    inline bandit::adapters::assertion_adapter& registered_adapter() {
      static bandit::adapters::snowhouse_adapter adapter;
      return adapter;
    }
  }
}
#endif
