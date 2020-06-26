//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_CONSTRAINTADAPTER_H
#define SNOWHOUSE_CONSTRAINTADAPTER_H

#include "../stringize.h"
#include "constraintlist.h"

namespace snowhouse
{
  template<typename ConstraintType>
  struct ConstraintAdapter
  {
    explicit ConstraintAdapter(const ConstraintType& constraint)
        : m_constraint(constraint)
    {
    }

    template<typename ConstraintListType, typename ActualType>
    void Evaluate(ConstraintListType& list, ResultStack& result, OperatorStack& operators, const ActualType& actual)
    {
      result.push(m_constraint(actual));
      EvaluateConstraintList(list.m_tail, result, operators, actual);
    }

    ConstraintType m_constraint;
  };

  template<typename ConstraintType>
  struct Stringizer<ConstraintAdapter<ConstraintType> >
  {
    static std::string ToString(const ConstraintAdapter<ConstraintType>& constraintAdapter)
    {
      return snowhouse::Stringize(constraintAdapter.m_constraint);
    }
  };
}

#endif
