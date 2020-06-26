#ifndef BANDIT_SPEC_REGISTRY_H
#define BANDIT_SPEC_REGISTRY_H

#include <list>
#include <bandit/types.h>

namespace bandit {
  namespace detail {
    typedef std::list<voidfunc_t> spec_registry;

    inline detail::spec_registry& specs() {
      static detail::spec_registry registry;
      return registry;
    }
  }
}
#endif
