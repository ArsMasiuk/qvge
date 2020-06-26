#ifndef BANDIT_GRAMMAR_H
#define BANDIT_GRAMMAR_H

#include <bandit/adapters/adapters.h>
#include <bandit/reporters/reporters.h>
#include <bandit/run_policies/run_policy.h>

namespace bandit {
  inline void describe(const std::string& desc, detail::voidfunc_t func,
      detail::listener& listener, detail::contextstack_t& context_stack,
      bool hard_skip = false) {
    listener.context_starting(desc);

    context_stack.back()->execution_is_starting();

    detail::bandit_context ctxt(desc, hard_skip);

    context_stack.push_back(&ctxt);

    try {
      func();
    } catch (const bandit::detail::test_run_error& error) {
      listener.test_run_error(desc, error);
    }

    context_stack.pop_back();

    listener.context_ended(desc);
  }

  inline void describe(const std::string& desc, detail::voidfunc_t func, bool hard_skip = false) {
    describe(desc, func, detail::registered_listener(), detail::context_stack(), hard_skip);
  }

  inline void describe_skip(const std::string& desc, detail::voidfunc_t func,
      detail::listener& listener, detail::contextstack_t& context_stack) {
    describe(desc, func, listener, context_stack, true);
  }

  inline void describe_skip(const std::string& desc, detail::voidfunc_t func) {
    describe_skip(desc, func, detail::registered_listener(),
        detail::context_stack());
  }

  inline void xdescribe(const std::string& desc, detail::voidfunc_t func,
      detail::listener& listener = detail::registered_listener(),
      detail::contextstack_t& context_stack = detail::context_stack()) {
    describe_skip(desc, func, listener, context_stack);
  }

  inline void before_each(detail::voidfunc_t func,
      detail::contextstack_t& context_stack) {
    context_stack.back()->register_before_each(func);
  }

  inline void before_each(detail::voidfunc_t func) {
    before_each(func, detail::context_stack());
  }

  inline void after_each(detail::voidfunc_t func,
      detail::contextstack_t& context_stack) {
    context_stack.back()->register_after_each(func);
  }

  inline void after_each(detail::voidfunc_t func) {
    after_each(func, detail::context_stack());
  }

  inline void it_skip(const std::string& desc, detail::voidfunc_t, detail::listener& listener) {
    listener.it_skip(desc);
  }

  inline void it_skip(const std::string& desc, detail::voidfunc_t func) {
    it_skip(desc, func, detail::registered_listener());
  }

  inline void xit(const std::string& desc, detail::voidfunc_t func,
      detail::listener& listener = detail::registered_listener()) {
    it_skip(desc, func, listener);
  }

  inline void it(const std::string& desc, detail::voidfunc_t func, detail::listener& listener,
      detail::contextstack_t& context_stack,
      bandit::adapters::assertion_adapter& assertion_adapter,
      detail::run_policy& run_policy,
      bool hard_skip = false) {
    if (hard_skip || !run_policy.should_run(desc, context_stack)) {
      it_skip(desc, func, listener);
      return;
    }

    listener.it_starting(desc);

    context_stack.back()->execution_is_starting();

    auto try_with_adapter = [&](detail::voidfunc_t do_it) {
      try {
        assertion_adapter.adapt_exceptions([&] { do_it(); });
      } catch (const bandit::detail::assertion_exception& ex) {
        listener.it_failed(desc, ex);
        run_policy.encountered_failure();
      } catch (const std::exception& ex) {
        std::string err = std::string("exception: ") + ex.what();
        listener.it_failed(desc, bandit::detail::assertion_exception(err));
        run_policy.encountered_failure();
      } catch (...) {
        listener.it_unknown_error(desc);
        run_policy.encountered_failure();
      }
    };

    bool success = false;
    try_with_adapter([&] {
      for (auto context : context_stack) {
        context->run_before_eaches();
      }

      func();
      success = true;
    });

    try_with_adapter([&] {
      for (auto context : context_stack) {
        context->run_after_eaches();
      }

      if (success) {
        listener.it_succeeded(desc);
      }
    });
  }

  inline void it(const std::string& desc, detail::voidfunc_t func, bool hard_skip = false) {
    it(desc, func, detail::registered_listener(), detail::context_stack(),
        detail::registered_adapter(), detail::registered_run_policy(), hard_skip);
  }
}
#endif
