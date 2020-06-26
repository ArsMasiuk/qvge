//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_FULFILLSCONSTRAINT_H
#define SNOWHOUSE_FULFILLSCONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  template<typename MatcherType>
  struct FulfillsConstraint : Expression<FulfillsConstraint<MatcherType> >
  {
    FulfillsConstraint(const MatcherType& matcher)
        : m_matcher(matcher)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return m_matcher.Matches(actual);
    }

    MatcherType m_matcher;
  };

  template<typename MatcherType>
  inline FulfillsConstraint<MatcherType> Fulfills(const MatcherType& matcher)
  {
    return FulfillsConstraint<MatcherType>(matcher);
  }

  template<typename MatcherType>
  struct Stringizer<FulfillsConstraint<MatcherType> >
  {
    static std::string ToString(const FulfillsConstraint<MatcherType>& constraint)
    {
      std::ostringstream builder;
      builder << snowhouse::Stringize(constraint.m_matcher);

      return builder.str();
    }
  };
}

#endif
