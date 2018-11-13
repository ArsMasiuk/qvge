#ifndef BANDIT_TEST_RUN_SUMMARY_H
#define BANDIT_TEST_RUN_SUMMARY_H

#include <algorithm>
#include <list>
#include <iostream>
#include <bandit/reporters/colorizer.h>

namespace bandit {
  namespace detail {
    struct test_run_summary {
      test_run_summary(int specs_run, int specs_failed, int specs_succeeded, int specs_skipped,
          const std::list<std::string>& failures, const std::list<std::string>& test_run_errors,
          const detail::colorizer& colorizer)
          : specs_run_(specs_run), specs_succeeded_(specs_succeeded), specs_failed_(specs_failed),
            specs_skipped_(specs_skipped), failures_(failures), test_run_errors_(test_run_errors),
            colorizer_(colorizer) {}

      test_run_summary& operator=(const test_run_summary&) {
        return *this;
      }

      void write(std::ostream& stm) {
        if (specs_run_ == 0 && test_run_errors_.size() == 0) {
          stm << colorizer_.red();
          stm << "Could not find any tests.";
          stm << colorizer_.reset();
          stm << std::endl;
          return;
        }

        if (specs_failed_ == 0 && test_run_errors_.size() == 0) {
          stm << colorizer_.green();
          stm << "Success!";
          stm << colorizer_.reset();
          stm << std::endl;
        }

        if (test_run_errors_.size() > 0) {
          for (const auto& error : test_run_errors_) {
            stm << error << std::endl;
          }
        }

        if (specs_failed_ > 0) {
          stm << colorizer_.red();
          stm << "There were failures!";
          stm << colorizer_.reset() << std::endl;
          for (const auto& failure : failures_) {
            stm << failure << std::endl;
          }
        }

        stm << "Test run complete. " << specs_run_ << " tests run. " << specs_succeeded_ << " succeeded.";

        if (specs_skipped_ > 0) {
          stm << " " << specs_skipped_ << " skipped.";
        }

        if (specs_failed_ > 0) {
          stm << " " << specs_failed_ << " failed.";
        }

        if (test_run_errors_.size() > 0) {
          stm << " " << test_run_errors_.size() << " test run errors.";
        }

        stm << std::endl;
      }

    private:
      int specs_run_;
      int specs_succeeded_;
      int specs_failed_;
      int specs_skipped_;
      std::list<std::string> failures_;
      std::list<std::string> test_run_errors_;
      const detail::colorizer& colorizer_;
    };
  }
}
#endif
