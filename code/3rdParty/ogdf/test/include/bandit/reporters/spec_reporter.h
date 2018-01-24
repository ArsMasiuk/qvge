#ifndef BANDIT_SPEC_REPORTER_H
#define BANDIT_SPEC_REPORTER_H

#include <bandit/reporters/colored_reporter.h>
#include <bandit/reporters/test_run_summary.h>

namespace bandit {
  namespace detail {
    struct spec_reporter : public colored_reporter {
      spec_reporter(std::ostream& stm, const failure_formatter& failure_formatter,
          const colorizer& colorizer)
          : colored_reporter(stm, failure_formatter, colorizer), indentation_(0) {}

      spec_reporter(const failure_formatter& failure_formatter, const colorizer& colorizer)
          : spec_reporter(std::cout, failure_formatter, colorizer) {}

      spec_reporter& operator=(const spec_reporter&) {
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

      void context_starting(const std::string& desc) override {
        progress_reporter::context_starting(desc);

        stm_ << indent();
        stm_ << "describe " << desc << std::endl;
        increase_indent();
        stm_.flush();
      }

      void context_ended(const std::string& desc) override {
        progress_reporter::context_ended(desc);
        decrease_indent();
      }

      void it_starting(const std::string& desc) override {
        progress_reporter::it_starting(desc);
        stm_ << indent() << "- it " << desc << " ... ";
        stm_.flush();
      }

      void it_succeeded(const std::string& desc) override {
        progress_reporter::it_succeeded(desc);
        stm_ << colorizer_.green();
        stm_ << "OK";
        stm_ << colorizer_.reset();
        stm_ << std::endl;
        stm_.flush();
      }

      void it_failed(const std::string& desc, const assertion_exception& ex) override {
        progress_reporter::it_failed(desc, ex);
        stm_ << colorizer_.red();
        stm_ << "FAILED";
        stm_ << colorizer_.reset();
        stm_ << std::endl;
        stm_.flush();
      }

      void it_unknown_error(const std::string& desc) override {
        progress_reporter::it_unknown_error(desc);
        stm_ << colorizer_.red();
        stm_ << "ERROR";
        stm_ << colorizer_.reset();
        stm_ << std::endl;
        stm_.flush();
      }

      void it_skip(const std::string& desc) override {
        progress_reporter::it_skip(desc);
        stm_ << indent() << "- it " << desc << " ... SKIPPED" << std::endl;
        stm_.flush();
      }

    private:
      void increase_indent() {
        indentation_++;
      }

      void decrease_indent() {
        indentation_--;
      }

      std::string indent() {
        return std::string(indentation_, '\t');
      }

    private:
      int indentation_;
    };
  }
}
#endif
