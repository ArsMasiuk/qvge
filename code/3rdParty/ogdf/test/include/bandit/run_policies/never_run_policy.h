#ifndef BANDIT_NEVER_RUN_POLICY_H
#define BANDIT_NEVER_RUN_POLICY_H

#include <bandit/run_policies/run_policy.h>

namespace bandit {
  namespace detail {
    struct never_run_policy : public run_policy {
      bool should_run(const std::string&, const contextstack_t&) const override {
        return false;
      }
    };
  }
}
#endif
