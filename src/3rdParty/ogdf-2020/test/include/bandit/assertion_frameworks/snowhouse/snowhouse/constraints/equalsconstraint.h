//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_EQUALSCONSTRAINT_H
#define SNOWHOUSE_EQUALSCONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  template<typename ExpectedType>
  struct EqualsConstraint : Expression<EqualsConstraint<ExpectedType> >
  {
    EqualsConstraint(const ExpectedType& expected)
        : m_expected(expected)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return (m_expected == actual);
    }

    ExpectedType m_expected;
  };

  template<typename ExpectedType>
  inline EqualsConstraint<ExpectedType> Equals(const ExpectedType& expected)
  {
    return EqualsConstraint<ExpectedType>(expected);
  }

  inline EqualsConstraint<std::string> Equals(const char* expected)
  {
    return EqualsConstraint<std::string>(expected);
  }

  inline EqualsConstraint<bool> IsFalse()
  {
    return EqualsConstraint<bool>(false);
  }

  inline EqualsConstraint<bool> IsTrue()
  {
    return EqualsConstraint<bool>(true);
  }

#ifdef SNOWHOUSE_HAS_NULLPTR
  inline EqualsConstraint<std::nullptr_t> IsNull()
  {
    return EqualsConstraint<std::nullptr_t>(nullptr);
  }
#endif

  template<>
  struct Stringizer<EqualsConstraint<bool> >
  {
    static std::string ToString(const EqualsConstraint<bool>& constraint)
    {
      return constraint.m_expected ? "true" : "false";
    }
  };

  template<typename ExpectedType>
  struct Stringizer<EqualsConstraint<ExpectedType> >
  {
    static std::string ToString(const EqualsConstraint<ExpectedType>& constraint)
    {
      std::ostringstream builder;
      builder << "equal to " << snowhouse::Stringize(constraint.m_expected);

      return builder.str();
    }
  };
}

#endif
