//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_HASLENGTHCONSTRAINT_H
#define SNOWHOUSE_HASLENGTHCONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  template<typename ExpectedType>
  struct HasLengthConstraint : Expression<HasLengthConstraint<ExpectedType> >
  {
    HasLengthConstraint(const ExpectedType& expected)
        : m_expected(expected)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      typedef typename ActualType::size_type SizeType;
      SizeType expectedSize = static_cast<SizeType>(m_expected);
      return (actual.size() == expectedSize);
    }

    ExpectedType m_expected;
  };

  template<typename ExpectedType>
  inline HasLengthConstraint<ExpectedType> HasLength(const ExpectedType& expected)
  {
    return HasLengthConstraint<ExpectedType>(expected);
  }

  inline HasLengthConstraint<std::string> HasLength(const char* expected)
  {
    return HasLengthConstraint<std::string>(expected);
  }

  template<typename ExpectedType>
  struct Stringizer<HasLengthConstraint<ExpectedType> >
  {
    static std::string ToString(const HasLengthConstraint<ExpectedType>& constraint)
    {
      std::ostringstream builder;
      builder << "of length " << snowhouse::Stringize(constraint.m_expected);

      return builder.str();
    }
  };
}

#endif
