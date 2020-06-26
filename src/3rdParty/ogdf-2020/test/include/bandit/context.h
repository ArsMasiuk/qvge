#ifndef BANDIT_CONTEXT_H
#define BANDIT_CONTEXT_H

#include <algorithm>
#include <deque>
#include <list>
#include <string>
#include <bandit/types.h>
#include <bandit/test_run_error.h>

namespace bandit {
  namespace detail {
    struct context {
      virtual ~context() {}
      virtual const std::string& name() = 0;
      virtual void execution_is_starting() = 0;
      virtual void register_before_each(voidfunc_t func) = 0;
      virtual void register_after_each(voidfunc_t func) = 0;
      virtual void run_before_eaches() = 0;
      virtual void run_after_eaches() = 0;
      virtual bool hard_skip() = 0;
    };

    struct bandit_context : public context {
      bandit_context(const std::string& desc, bool hard_skip_a)
          : desc_(desc), hard_skip_(hard_skip_a), is_executing_(false) {}

      const std::string& name() override {
        return desc_;
      }

      void execution_is_starting() override {
        is_executing_ = true;
      }

      void register_before_each(voidfunc_t func) override {
        if (is_executing_) {
          throw test_run_error("before_each was called after 'describe' or 'it'");
        }

        before_eaches_.push_back(func);
      }

      void register_after_each(voidfunc_t func) override {
        if (is_executing_) {
          throw test_run_error("after_each was called after 'describe' or 'it'");
        }

        after_eaches_.push_back(func);
      }

      void run_before_eaches() override {
        run_all(before_eaches_);
      }

      void run_after_eaches() override {
        run_all(after_eaches_);
      }

      bool hard_skip() override {
        return hard_skip_;
      }

    private:
      void run_all(const std::list<voidfunc_t>& funcs) {
        for (auto f : funcs) {
          f();
        }
      }

      std::string desc_;
      bool hard_skip_;
      bool is_executing_;
      std::list<voidfunc_t> before_eaches_;
      std::list<voidfunc_t> after_eaches_;
    };

    typedef std::deque<context*> contextstack_t;

    inline contextstack_t& context_stack() {
      static contextstack_t contexts;
      return contexts;
    }
  }
}
#endif
