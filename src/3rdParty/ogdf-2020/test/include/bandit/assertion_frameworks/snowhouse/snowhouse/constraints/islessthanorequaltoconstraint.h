//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_ISLESSTHANOREQUALTOCONSTRAINT_H
#define SNOWHOUSE_ISLESSTHANOREQUALTOCONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  template<typename ExpectedType>
  struct IsLessThanOrEqualToConstraint : Expression<IsLessThanOrEqualToConstraint<ExpectedType> >
  {
    IsLessThanOrEqualToConstraint(const ExpectedType& expected)
        : m_expected(expected)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return (actual <= m_expected);
    }

    ExpectedType m_expected;
  };

  template<typename ExpectedType>
  inline IsLessThanOrEqualToConstraint<ExpectedType> IsLessThanOrEqualTo(const ExpectedType& expected)
  {
    return IsLessThanOrEqualToConstraint<ExpectedType>(expected);
  }

  inline IsLessThanOrEqualToConstraint<std::string> IsLessThanOrEqualTo(const char* expected)
  {
    return IsLessThanOrEqualToConstraint<std::string>(expected);
  }

  template<typename ExpectedType>
  struct Stringizer<IsLessThanOrEqualToConstraint<ExpectedType> >
  {
    static std::string ToString(const IsLessThanOrEqualToConstraint<ExpectedType>& constraint)
    {
      std::ostringstream builder;
      builder << "less than or equal to " << snowhouse::Stringize(constraint.m_expected);

      return builder.str();
    }
  };
}

#endif
