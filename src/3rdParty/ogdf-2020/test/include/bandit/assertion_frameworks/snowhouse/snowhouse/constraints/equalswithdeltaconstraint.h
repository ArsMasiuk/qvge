//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_EQUALSWITHDELTACONSTRAINT_H
#define SNOWHOUSE_EQUALSWITHDELTACONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  template<typename ExpectedType, typename DeltaType>
  struct EqualsWithDeltaConstraint : Expression<EqualsWithDeltaConstraint<ExpectedType, DeltaType> >
  {
    EqualsWithDeltaConstraint(const ExpectedType& expected, const DeltaType& delta)
        : m_expected(expected), m_delta(delta)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return ((m_expected <= (actual + m_delta)) && (m_expected >= (actual - m_delta)));
    }

    ExpectedType m_expected;
    DeltaType m_delta;
  };

  template<typename ExpectedType, typename DeltaType>
  inline EqualsWithDeltaConstraint<ExpectedType, DeltaType> EqualsWithDelta(const ExpectedType& expected, const DeltaType& delta)
  {
    return EqualsWithDeltaConstraint<ExpectedType, DeltaType>(expected, delta);
  }

  template<typename ExpectedType, typename DeltaType>
  struct Stringizer<EqualsWithDeltaConstraint<ExpectedType, DeltaType> >
  {
    static std::string ToString(const EqualsWithDeltaConstraint<ExpectedType, DeltaType>& constraint)
    {
      std::ostringstream builder;
      builder << "equal to " << snowhouse::Stringize(constraint.m_expected) << " (+/- " << snowhouse::Stringize(constraint.m_delta) << ")";

      return builder.str();
    }
  };
}

#endif
