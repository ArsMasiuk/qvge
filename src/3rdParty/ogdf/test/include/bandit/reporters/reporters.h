#ifndef BANDIT_REPORTERS_H
#define BANDIT_REPORTERS_H

#include <bandit/reporters/dots_reporter.h>
#include <bandit/reporters/single_line_reporter.h>
#include <bandit/reporters/xunit_reporter.h>
#include <bandit/reporters/info_reporter.h>
#include <bandit/reporters/spec_reporter.h>
#include <bandit/reporters/crash_reporter.h>

namespace bandit {
  namespace detail {
    struct listener;

    struct reporter {
      // A function is required to initialize a static listener variable in a header file
      // and this struct aims at encapsulating this function
      static void register_listener(listener* reporter) {
        if (reporter == nullptr) {
          throw std::runtime_error("Invalid null reporter passed to "
                                   "bandit::detail::register_listener");
        }
        get_reporter_address() = reporter;
      }
      static listener& registered_listener() {
        auto reporter = get_reporter_address();
        if (reporter == nullptr) {
          throw std::runtime_error("No reporter set. Please call "
                                   "bandit::detail::register_listener with a non-null reporter");
        }
        return *reporter;
      }

    private:
      static listener*& get_reporter_address() {
        static listener* reporter_ = nullptr;
        return reporter_;
      }
    };

    inline void register_listener(listener* reporter) {
      reporter::register_listener(reporter);
    }
    inline listener& registered_listener() {
      return reporter::registered_listener();
    }
  }
}
#endif
