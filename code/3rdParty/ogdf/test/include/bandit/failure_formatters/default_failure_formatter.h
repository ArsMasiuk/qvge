#ifndef BANDIT_DEFAULT_FAILURE_FORMATTER_H
#define BANDIT_DEFAULT_FAILURE_FORMATTER_H

#include <sstream>
#include <bandit/failure_formatters/failure_formatter.h>

namespace bandit {
  namespace detail {
    struct default_failure_formatter : public failure_formatter {
      std::string format(const assertion_exception& err) const override {
        std::stringstream ss;
        if (err.file_name().size()) {
          ss << err.file_name();

          if (err.line_number()) {
            ss << ":" << err.line_number();
          }

          ss << ": ";
        }

        ss << err.what();

        return ss.str();
      }
    };
  }
}
#endif
