//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_ASSERTIONEXCEPTION_H
#define SNOWHOUSE_ASSERTIONEXCEPTION_H

#include <exception>
#include <string>

#include "macros.h"

namespace snowhouse
{
  struct AssertionException : public std::exception
  {
    explicit AssertionException(const std::string& message)
        : m_message(message), m_fileName(""), m_line(0)
    {
    }

    AssertionException(const std::string& message, const std::string& fileName, unsigned int line)
        : m_message(message), m_fileName(fileName), m_line(line)
    {
    }

#if __cplusplus > 199711L
    AssertionException(const AssertionException&) = default;
#endif

#if __cplusplus > 199711L
    virtual ~AssertionException() noexcept
#else
    virtual ~AssertionException() throw()
#endif
    {
    }

    std::string GetMessage() const
    {
      return m_message;
    }

    std::string GetFilename() const
    {
      return m_fileName;
    }

    unsigned int GetLineNumber() const
    {
      return m_line;
    }

  private:
    std::string m_message;
    std::string m_fileName;
    unsigned int m_line;
  };
}

#endif
