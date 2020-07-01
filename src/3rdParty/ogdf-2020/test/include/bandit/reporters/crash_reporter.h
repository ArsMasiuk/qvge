#ifndef BANDIT_CRASH_REPORTER_H
#define BANDIT_CRASH_REPORTER_H

#include <iostream>
#include <vector>
#include <bandit/reporters/progress_reporter.h>

namespace bandit {
  namespace detail {
    struct crash_reporter : public progress_reporter {
      crash_reporter(std::ostream& stm, const failure_formatter& failure_formatter)
          : progress_reporter(failure_formatter), stm_(stm) {}

      crash_reporter(const failure_formatter& failure_formatter)
          : crash_reporter(std::cout, failure_formatter) {}

      crash_reporter& operator=(const crash_reporter&) {
        return *this;
      }

      void test_run_complete() override {
        progress_reporter::test_run_complete();
        for (auto failure : failures_) {
          stm_ << std::endl
               << "# FAILED " << failure;
        }
        for (auto error : test_run_errors_) {
          stm_ << std::endl
               << "# ERROR " << error;
        }
        stm_.flush();
      }

      void test_run_error(const std::string& desc, const struct test_run_error& err) override {
        progress_reporter::test_run_error(desc, err);
        std::stringstream ss;
        ss << current_context_name() << ": " << desc << ": " << err.what() << std::endl;
        test_run_errors_.push_back(ss.str());
      }

      void it_skip(const std::string& desc) override {
        progress_reporter::it_skip(desc);
      }

      void it_starting(const std::string& desc) override {
        progress_reporter::it_starting(desc);
        for (auto context : contexts_) {
          stm_ << context << " | ";
        }
        stm_ << desc << std::endl;
      }

      void it_succeeded(const std::string& desc) override {
        progress_reporter::it_succeeded(desc);
      }

      void it_failed(const std::string& desc, const assertion_exception& ex) override {
        ++specs_failed_;

        std::stringstream ss;
        ss << current_context_name() << " " << desc << ":" << std::endl
           << failure_formatter_.format(ex);
        failures_.push_back(ss.str());

        stm_ << "FAILED" << std::endl;
      }

      void it_unknown_error(const std::string& desc) override {
        ++specs_failed_;

        std::stringstream ss;
        ss << current_context_name() << " " << desc << ": Unknown exception" << std::endl;
        failures_.push_back(ss.str());

        stm_ << "UNKNOWN EXCEPTION" << std::endl;
      }

    private:
      std::ostream& stm_;
    };
  }
}
#endif
