//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_STARTSWITHCONSTRAINT_H
#define SNOWHOUSE_STARTSWITHCONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  template<typename ExpectedType>
  struct StartsWithConstraint : Expression<StartsWithConstraint<ExpectedType> >
  {
    StartsWithConstraint(const ExpectedType& expected)
        : m_expected(expected)
    {
    }

    bool operator()(const std::string& actual) const
    {
      return actual.find(m_expected) == 0;
    }

    ExpectedType m_expected;
  };

  template<typename ExpectedType>
  inline StartsWithConstraint<ExpectedType> StartsWith(const ExpectedType& expected)
  {
    return StartsWithConstraint<ExpectedType>(expected);
  }

  inline StartsWithConstraint<std::string> StartsWith(const char* expected)
  {
    return StartsWithConstraint<std::string>(expected);
  }

  template<typename ExpectedType>
  struct Stringizer<StartsWithConstraint<ExpectedType> >
  {
    static std::string ToString(const StartsWithConstraint<ExpectedType>& constraint)
    {
      std::ostringstream builder;
      builder << "starts with " << snowhouse::Stringize(constraint.m_expected);

      return builder.str();
    }
  };
}

#endif
