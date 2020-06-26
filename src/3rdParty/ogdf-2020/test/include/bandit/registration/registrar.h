#ifndef BANDIT_REGISTRAR_H
#define BANDIT_REGISTRAR_H

#include <bandit/registration/spec_registry.h>

namespace bandit {
  namespace detail {
    struct spec_registrar {
      spec_registrar(bandit::detail::voidfunc_t func) {
        bandit::detail::specs().push_back(func);
      }
    };
  }
}

#define go_bandit \
  static bandit::detail::spec_registrar bandit_registrar

#define SPEC_BEGIN(name) \
  go_bandit([]{
#define SPEC_END \
  });
#endif
