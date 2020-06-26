#ifndef BANDIT_ASSERTION_EXCEPTION_H
#define BANDIT_ASSERTION_EXCEPTION_H

#include <stdexcept>
#include <string>

namespace bandit {
  namespace detail {
    struct assertion_exception : public std::runtime_error {
      assertion_exception(const std::string& message,
          const std::string& filename, const unsigned int linenumber)
          : std::runtime_error(message), file_name_(filename), line_number_(linenumber) {}

      assertion_exception(const std::string& message)
          : std::runtime_error(message), line_number_(0) {}

      // To make gcc < 4.7 happy.
      assertion_exception(const assertion_exception&) = default;

#ifndef _MSC_VER
      assertion_exception(assertion_exception&&) = default;
#else
      assertion_exception(assertion_exception&& other)
          : std::runtime_error(other), file_name_(), line_number_(other.line_number_) {
        std::swap(file_name_, other.file_name_);
      }
#endif

      virtual ~assertion_exception() noexcept {}

      const std::string& file_name() const {
        return file_name_;
      }

      unsigned int line_number() const {
        return line_number_;
      }

    private:
      std::string file_name_;
      unsigned int line_number_;
    };
  }
}
#endif
