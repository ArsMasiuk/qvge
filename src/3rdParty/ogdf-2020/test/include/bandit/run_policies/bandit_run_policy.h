#ifndef BANDIT_BANDIT_RUN_POLICY_H
#define BANDIT_BANDIT_RUN_POLICY_H

#include <bandit/filter_chain.h>
#include <bandit/run_policies/run_policy.h>

namespace bandit {
  namespace detail {
    struct bandit_run_policy : public run_policy {
      bandit_run_policy(filter_chain_t filter_chain,
          bool break_on_failure, bool dry_run)
          : run_policy(),
            filter_chain_(filter_chain),
            break_on_failure_(break_on_failure), dry_run_(dry_run) {}

      bool should_run(const std::string& it_name, const contextstack_t& contexts) const override {
        if (dry_run_) {
          return false;
        }

        if (break_on_failure_ && has_encountered_failure()) {
          return false;
        }

        // Never run if a context has been marked as skip
        // using 'describe_skip'
        if (has_context_with_hard_skip(contexts)) {
          return false;
        }

        // Now go through the filter chain
        for (auto filter : filter_chain_) {
          bool match = context_matches_pattern(contexts, filter.pattern) || matches_pattern(it_name, filter.pattern);
          bool skip_applies = filter.skip && match;
          bool only_applies = !filter.skip && !match;
          if (skip_applies || only_applies) {
            return false;
          }
        }

        return true;
      }

    private:
      bool has_context_with_hard_skip(const contextstack_t& contexts) const {
        contextstack_t::const_iterator it;
        for (it = contexts.begin(); it != contexts.end(); it++) {
          if ((*it)->hard_skip()) {
            return true;
          }
        }

        return false;
      }

      bool context_matches_pattern(const contextstack_t& contexts, const std::string& pattern) const {
        for (auto context : contexts) {
          if (matches_pattern(context->name(), pattern)) {
            return true;
          }
        }

        return false;
      }

      bool matches_pattern(const std::string& name, const std::string& pattern) const {
        return name.find(pattern) != std::string::npos;
      }

    private:
      const filter_chain_t filter_chain_;
      bool break_on_failure_;
      bool dry_run_;
    };
  }
}
#endif
