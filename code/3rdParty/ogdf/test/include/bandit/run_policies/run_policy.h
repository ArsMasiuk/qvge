#ifndef BANDIT_RUN_POLICY_H
#define BANDIT_RUN_POLICY_H

#include <memory>
#include <stdexcept>
#include <bandit/context.h>

namespace bandit {
  namespace detail {
    struct run_policy {
      run_policy()
          : encountered_failure_(false) {}
      run_policy(const run_policy& other) = default;

#ifndef _MSC_VER
      run_policy(run_policy&&) = default;
#else
      run_policy(run_policy&& other)
          : encountered_failure_(other.encountered_failure_) {}
#endif

      virtual ~run_policy() {}

      virtual bool should_run(const std::string& it_name, const contextstack_t& contexts) const = 0;

      virtual void encountered_failure() {
        encountered_failure_ = true;
      }

      virtual bool has_encountered_failure() const {
        return encountered_failure_;
      }

    private:
      bool encountered_failure_;
    };

    typedef std::unique_ptr<run_policy> run_policy_ptr;

    struct policy_runner {
      // A function is required to initialize a static run_policy variable in a header file
      // and this struct aims at encapsulating this function
      static void register_run_policy(run_policy* policy) {
        if (policy == nullptr) {
          throw std::runtime_error("Invalid null policy passed to "
                                   "bandit::detail::register_run_policy");
        }
        get_run_policy_address() = policy;
      }

      static run_policy& registered_run_policy() {
        auto policy = get_run_policy_address();
        if (policy == nullptr) {
          throw std::runtime_error("No policy set. Please call "
                                   "bandit::detail::register_run_policy with a non-null reporter");
        }
        return *policy;
      }

    private:
      static run_policy*& get_run_policy_address() {
        static run_policy* policy = nullptr;
        return policy;
      }
    };

    inline void register_run_policy(run_policy* policy) {
      policy_runner::register_run_policy(policy);
    }

    inline run_policy& registered_run_policy() {
      return policy_runner::registered_run_policy();
    }
  }
}
#endif
