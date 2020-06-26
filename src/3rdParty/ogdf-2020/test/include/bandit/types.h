#ifndef BANDIT_TYPES_H
#define BANDIT_TYPES_H

#include <functional>

namespace bandit {
  namespace detail {
    typedef std::function<void()> voidfunc_t;
  }
}
#endif
