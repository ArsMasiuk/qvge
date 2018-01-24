//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_ISGREATERTHANOREQUALTOCONSTRAINT_H
#define SNOWHOUSE_ISGREATERTHANOREQUALTOCONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  template<typename ExpectedType>
  struct IsGreaterThanOrEqualToConstraint : Expression<IsGreaterThanOrEqualToConstraint<ExpectedType> >
  {
    IsGreaterThanOrEqualToConstraint(const ExpectedType& expected)
        : m_expected(expected)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return (actual >= m_expected);
    }

    ExpectedType m_expected;
  };

  template<typename ExpectedType>
  inline IsGreaterThanOrEqualToConstraint<ExpectedType> IsGreaterThanOrEqualTo(const ExpectedType& expected)
  {
    return IsGreaterThanOrEqualToConstraint<ExpectedType>(expected);
  }

  inline IsGreaterThanOrEqualToConstraint<std::string> IsGreaterThanOrEqualTo(const char* expected)
  {
    return IsGreaterThanOrEqualToConstraint<std::string>(expected);
  }

  template<typename ExpectedType>
  struct Stringizer<IsGreaterThanOrEqualToConstraint<ExpectedType> >
  {
    static std::string ToString(const IsGreaterThanOrEqualToConstraint<ExpectedType>& constraint)
    {
      std::ostringstream builder;
      builder << "greater than or equal to " << snowhouse::Stringize(constraint.m_expected);

      return builder.str();
    }
  };
}

#endif
