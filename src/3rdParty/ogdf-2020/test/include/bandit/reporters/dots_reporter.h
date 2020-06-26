#ifndef BANDIT_DOTS_REPORTER_H
#define BANDIT_DOTS_REPORTER_H

#include <bandit/reporters/colored_reporter.h>
#include <bandit/reporters/test_run_summary.h>

namespace bandit {
  namespace detail {
    struct dots_reporter : public colored_reporter {
      dots_reporter(std::ostream& stm, const failure_formatter& failure_formatter,
          const colorizer& colorizer)
          : colored_reporter(stm, failure_formatter, colorizer) {}

      dots_reporter(const failure_formatter& failure_formatter, const colorizer& colorizer)
          : dots_reporter(std::cout, failure_formatter, colorizer) {}

      dots_reporter& operator=(const dots_reporter&) {
        return *this;
      }

      void test_run_complete() override {
        progress_reporter::test_run_complete();

        stm_ << std::endl;

        test_run_summary summary(specs_run_, specs_failed_, specs_succeeded_, specs_skipped_, failures_,
            test_run_errors_, colorizer_);
        summary.write(stm_);
        stm_.flush();
      }

      void test_run_error(const std::string& desc, const struct test_run_error& err) override {
        progress_reporter::test_run_error(desc, err);

        std::stringstream ss;
        ss << std::endl;
        ss << "Failed to run \"" << current_context_name() << "\": error \"" << err.what() << "\"" << std::endl;

        test_run_errors_.push_back(ss.str());
      }

      void it_succeeded(const std::string& desc) override {
        progress_reporter::it_succeeded(desc);
        stm_ << colorizer_.green() << "." << colorizer_.reset();
        stm_.flush();
      }

      void it_failed(const std::string& desc, const assertion_exception& ex) override {
        progress_reporter::it_failed(desc, ex);
        stm_ << colorizer_.red() << "F" << colorizer_.reset();
        stm_.flush();
      }

      void it_skip(const std::string& desc) override {
        progress_reporter::it_skip(desc);
        stm_ << colorizer_.yellow() << 'S' << colorizer_.reset();
        stm_.flush();
      }

      void it_unknown_error(const std::string& desc) override {
        progress_reporter::it_unknown_error(desc);
        stm_ << colorizer_.red() << "E" << colorizer_.reset();
        stm_.flush();
      }
    };
  }
}
#endif
