//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_INVALIDEXPRESSIONEXCEPTION_H
#define SNOWHOUSE_INVALIDEXPRESSIONEXCEPTION_H

#include <string>

namespace snowhouse
{
  struct InvalidExpressionException
  {
    explicit InvalidExpressionException(const std::string& message)
        : m_message(message)
    {
    }

    const std::string& Message() const
    {
      return m_message;
    }

    std::string m_message;
  };
}

#endif
