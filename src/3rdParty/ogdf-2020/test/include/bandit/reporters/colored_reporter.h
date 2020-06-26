#ifndef BANDIT_COLORED_REPORTER_H
#define BANDIT_COLORED_REPORTER_H

#include <ostream>
#include <bandit/reporters/colorizer.h>
#include <bandit/reporters/progress_reporter.h>

namespace bandit {
  namespace detail {
    struct colored_reporter : public progress_reporter {
      colored_reporter(std::ostream& stm,
          const failure_formatter& failure_formatter,
          const colorizer& colorizer)
          : progress_reporter(failure_formatter), stm_(stm), colorizer_(colorizer) {}

      ~colored_reporter() override {
        stm_ << colorizer_.reset();
      }

    protected:
      std::ostream& stm_;
      const detail::colorizer& colorizer_;
    };
  }
}
#endif
