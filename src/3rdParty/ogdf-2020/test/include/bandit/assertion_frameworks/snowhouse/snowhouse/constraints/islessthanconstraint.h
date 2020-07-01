//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_ISLESSTHANCONSTRAINT_H
#define SNOWHOUSE_ISLESSTHANCONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  template<typename ExpectedType>
  struct IsLessThanConstraint : Expression<IsLessThanConstraint<ExpectedType> >
  {
    IsLessThanConstraint(const ExpectedType& expected)
        : m_expected(expected)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return (actual < m_expected);
    }

    ExpectedType m_expected;
  };

  template<typename ExpectedType>
  inline IsLessThanConstraint<ExpectedType> IsLessThan(const ExpectedType& expected)
  {
    return IsLessThanConstraint<ExpectedType>(expected);
  }

  inline IsLessThanConstraint<std::string> IsLessThan(const char* expected)
  {
    return IsLessThanConstraint<std::string>(expected);
  }

  template<typename ExpectedType>
  struct Stringizer<IsLessThanConstraint<ExpectedType> >
  {
    static std::string ToString(const IsLessThanConstraint<ExpectedType>& constraint)
    {
      std::ostringstream builder;
      builder << "less than " << snowhouse::Stringize(constraint.m_expected);

      return builder.str();
    }
  };
}
#endif
