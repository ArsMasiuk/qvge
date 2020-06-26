#ifndef BANDIT_FAILURE_FORMATTER_H
#define BANDIT_FAILURE_FORMATTER_H

#include <memory>
#include <bandit/assertion_exception.h>

namespace bandit {
  namespace detail {
    struct failure_formatter {
      virtual std::string format(const assertion_exception&) const = 0;
      virtual ~failure_formatter() = default;
    };

    typedef std::unique_ptr<failure_formatter> failure_formatter_ptr;
  }
}
#endif
