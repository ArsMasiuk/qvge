#ifndef BANDIT_REPORTERS_SINGLE_LINE_REPORTER_H
#define BANDIT_REPORTERS_SINGLE_LINE_REPORTER_H

#include <bandit/reporters/colored_reporter.h>
#include <bandit/reporters/test_run_summary.h>

namespace bandit {
  namespace detail {
    struct single_line_reporter : public colored_reporter {
      single_line_reporter(std::ostream& stm, const failure_formatter& failure_formatter,
          const colorizer& colorizer)
          : colored_reporter(stm, failure_formatter, colorizer) {}

      single_line_reporter(const failure_formatter& failure_formatter,
          const colorizer& colorizer)
          : single_line_reporter(std::cout, failure_formatter, colorizer) {}

      single_line_reporter& operator=(const single_line_reporter&) {
        return *this;
      }

      void test_run_complete() override {
        progress_reporter::test_run_complete();

        stm_ << std::endl;

        test_run_summary summary(specs_run_, specs_failed_, specs_succeeded_, specs_skipped_, failures_,
            test_run_errors_, colorizer_);
        summary.write(stm_);
      }

      void test_run_error(const std::string& desc, const struct test_run_error& err) override {
        progress_reporter::test_run_error(desc, err);

        std::stringstream ss;
        ss << std::endl;
        ss << "Failed to run \"" << current_context_name() << "\": error \"" << err.what() << "\"" << std::endl;

        test_run_errors_.push_back(ss.str());
      }

      void it_starting(const std::string& desc) override {
        print_status_line();
        progress_reporter::it_starting(desc);
      }

      void it_succeeded(const std::string& desc) override {
        progress_reporter::it_succeeded(desc);
        print_status_line();
      }

      void it_failed(const std::string& desc, const assertion_exception& ex) override {
        progress_reporter::it_failed(desc, ex);
        print_status_line();
      }

      void it_unknown_error(const std::string& desc) override {
        progress_reporter::it_unknown_error(desc);
        print_status_line();
      }

    private:
      void print_status_line() {
        stm_ << '\r';
        stm_ << "Executed " << specs_run_ << " tests.";

        if (specs_failed_) {
          stm_
              << " " << specs_succeeded_ << " succeeded. "
              << colorizer_.red() << specs_failed_ << " failed."
              << colorizer_.reset();
        }
        stm_.flush();
      }
    };
  }
}
#endif
