#ifndef BANDIT_ADAPTERS_SNOWHOUSE_H
#define BANDIT_ADAPTERS_SNOWHOUSE_H

#include <bandit/adapters/adapter.h>
#include <bandit/assertion_exception.h>
#include <bandit/assertion_frameworks/snowhouse/snowhouse/snowhouse.h>

namespace bandit {
  namespace adapters {
    struct snowhouse_adapter : public assertion_adapter {
      void adapt_exceptions(detail::voidfunc_t func) override {
        try {
          func();
        } catch (const snowhouse::AssertionException& ex) {
          throw bandit::detail::assertion_exception(ex.GetMessage(), ex.GetFilename(), ex.GetLineNumber());
        }
      }
    };
  }
}
#endif
