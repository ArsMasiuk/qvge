#ifndef BANDIT_INFO_REPORTER_H
#define BANDIT_INFO_REPORTER_H

#include <iostream>
#include <stack>
#include <bandit/reporters/colored_reporter.h>

namespace bandit {
  namespace detail {
    struct info_reporter : public colored_reporter {
      struct context_info {
        context_info(const std::string& d) : desc(d), total(0), skipped(0), failed(0) {}

        void merge(const context_info& ci) {
          total += ci.total;
          skipped += ci.skipped;
          failed += ci.failed;
        }

        const std::string desc; // copy
        int total;
        int skipped;
        int failed;
      };

      info_reporter(std::ostream& stm, const failure_formatter& failure_formatter, const colorizer& colorizer)
          : colored_reporter(stm, failure_formatter, colorizer),
            indentation_(0), not_yet_shown_(0), context_stack_() {}

      info_reporter(const failure_formatter& failure_formatter, const colorizer& colorizer)
          : info_reporter(std::cout, failure_formatter, colorizer) {}

      info_reporter& operator=(const info_reporter&) {
        return *this;
      }

      void list_failures_and_errors() {
        if (specs_failed_ > 0) {
          stm_
              << colorizer_.red()
              << "List of failures:"
              << std::endl;
          for (const auto& failure : failures_) {
            stm_
                << colorizer_.white()
                << " (*) "
                << colorizer_.red()
                << failure << std::endl;
          }
        }
        if (test_run_errors_.size() > 0) {
          stm_
              << colorizer_.red()
              << "List of run errors:"
              << std::endl;
          for (const auto& error : test_run_errors_) {
            stm_
                << colorizer_.white()
                << " (*) "
                << colorizer_.red()
                << error << std::endl;
          }
        }
      }

      void summary() {
        stm_
            << colorizer_.white()
            << "Tests run: " << specs_run_
            << std::endl;
        if (specs_skipped_ > 0) {
          stm_
              << colorizer_.yellow()
              << "Skipped: " << specs_skipped_
              << std::endl;
        }
        if (specs_succeeded_ > 0) {
          stm_
              << colorizer_.green()
              << "Passed: " << specs_succeeded_
              << std::endl;
        }
        if (specs_failed_ > 0) {
          stm_
              << colorizer_.red()
              << "Failed: " << specs_failed_
              << std::endl;
        }
        if (test_run_errors_.size() > 0) {
          stm_
              << colorizer_.red()
              << "Errors: " << test_run_errors_.size()
              << std::endl;
        }
        stm_
            << colorizer_.reset()
            << std::endl;
      }

      void test_run_complete() override {
        progress_reporter::test_run_complete();
        stm_ << std::endl;
        list_failures_and_errors();
        summary();
        stm_.flush();
      }

      void test_run_error(const std::string& desc, const struct test_run_error& err) override {
        progress_reporter::test_run_error(desc, err);

        std::stringstream ss;
        ss << "Failed to run \"" << current_context_name() << "\": error \"" << err.what() << "\"" << std::endl;
        test_run_errors_.push_back(ss.str());
      }

      void context_starting(const std::string& desc) override {
        progress_reporter::context_starting(desc);
        context_stack_.emplace(desc);
        if (context_stack_.size() == 1) {
          output_context_start_message();
        } else {
          ++not_yet_shown_;
        }
      }

      void output_context_start_message() {
        stm_
            << indent()
            << colorizer_.blue()
            << "begin "
            << colorizer_.white()
            << context_stack_.top().desc
            << colorizer_.reset()
            << std::endl;
        ++indentation_;
        stm_.flush();
      }

      void output_not_yet_shown_context_start_messages() {
        std::stack<context_info> temp_stack;
        for (int i = 0; i < not_yet_shown_; ++i) {
          temp_stack.push(context_stack_.top());
          context_stack_.pop();
        }
        for (int i = 0; i < not_yet_shown_; ++i) {
          context_stack_.push(temp_stack.top());
          output_context_start_message();
          temp_stack.pop();
        }
        not_yet_shown_ = 0;
      }

      void context_ended(const std::string& desc) override {
        progress_reporter::context_ended(desc);
        if (context_stack_.size() == 1 || context_stack_.top().total > context_stack_.top().skipped) {
          output_context_end_message();
        }
        const context_info context = context_stack_.top(); // copy
        context_stack_.pop();
        if (!context_stack_.empty()) {
          context_stack_.top().merge(context);
        }
        if (not_yet_shown_ > 0) {
          --not_yet_shown_;
        }
      }

      void output_context_end_message() {
        const context_info& context = context_stack_.top();
        --indentation_;
        stm_
            << indent()
            << colorizer_.blue()
            << "end "
            << colorizer_.reset()
            << context.desc;
        if (context.total > 0) {
          stm_
              << colorizer_.white()
              << " " << context.total << " total";
        }
        if (context.skipped > 0) {
          stm_
              << colorizer_.yellow()
              << " " << context.skipped << " skipped";
        }
        if (context.failed > 0) {
          stm_
              << colorizer_.red()
              << " " << context.failed << " failed";
        }
        stm_ << colorizer_.reset() << std::endl;
      }

      void it_skip(const std::string& desc) override {
        progress_reporter::it_skip(desc);
        ++context_stack_.top().total;
        ++context_stack_.top().skipped;
      }

      void it_starting(const std::string& desc) override {
        if (context_stack_.size() > 1 && context_stack_.top().total == context_stack_.top().skipped) {
          output_not_yet_shown_context_start_messages();
        }

        progress_reporter::it_starting(desc);
        stm_
            << indent()
            << colorizer_.yellow()
            << "[ TEST ]"
            << colorizer_.reset()
            << " it " << desc;
        ++indentation_;
        stm_.flush();
      }

      void it_succeeded(const std::string& desc) override {
        progress_reporter::it_succeeded(desc);
        ++context_stack_.top().total;
        --indentation_;
        stm_
            << "\r" << indent()
            << colorizer_.green()
            << "[ PASS ]"
            << colorizer_.reset()
            << " it " << desc
            << std::endl;
        stm_.flush();
      }

      void it_failed(const std::string& desc, const assertion_exception& ex) override {
        ++specs_failed_;

        std::stringstream ss;
        ss << current_context_name() << " " << desc << ":" << std::endl
           << failure_formatter_.format(ex);
        failures_.push_back(ss.str());

        ++context_stack_.top().total;
        ++context_stack_.top().failed;
        --indentation_;
        stm_
            << "\r" << indent()
            << colorizer_.red()
            << "[ FAIL ]"
            << colorizer_.reset()
            << " it " << desc
            << std::endl;
        stm_.flush();
      }

      void it_unknown_error(const std::string& desc) override {
        ++specs_failed_;

        std::stringstream ss;
        ss << current_context_name() << " " << desc << ": Unknown exception" << std::endl;
        failures_.push_back(ss.str());

        ++context_stack_.top().total;
        ++context_stack_.top().failed;
        --indentation_;
        stm_
            << "\r" << indent()
            << colorizer_.red()
            << "-ERROR->"
            << colorizer_.reset()
            << " it " << desc
            << std::endl;
        stm_.flush();
      }

    private:
      std::string indent() {
        return std::string(2 * indentation_, ' ');
      }

      int indentation_;
      int not_yet_shown_; // number of elements in stack that are not yet shown
      std::stack<context_info> context_stack_;
    };
  }
}
#endif
