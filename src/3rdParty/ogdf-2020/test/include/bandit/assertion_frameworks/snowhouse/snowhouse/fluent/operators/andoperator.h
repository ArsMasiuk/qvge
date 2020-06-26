//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_ANDOPERATOR_H
#define SNOWHOUSE_ANDOPERATOR_H

#include "constraintoperator.h"

namespace snowhouse
{
  struct AndOperator : public ConstraintOperator
  {
    template<typename ConstraintListType, typename ActualType>
    void Evaluate(ConstraintListType& list, ResultStack& result, OperatorStack& operators, const ActualType& actual)
    {
      EvaluateOperatorsWithLessOrEqualPrecedence(*this, operators, result);

      operators.push(this);

      EvaluateConstraintList(list.m_tail, result, operators, actual);
    }

    void PerformOperation(ResultStack& result)
    {
      if (result.size() < 2)
      {
        throw InvalidExpressionException("The expression contains an and operator with too few operands");
      }

      bool right = result.top();
      result.pop();
      bool left = result.top();
      result.pop();

      result.push(left && right);
    }

    int Precedence() const
    {
      return 3;
    }
  };

  template<>
  struct Stringizer<AndOperator>
  {
    static std::string ToString(const AndOperator&)
    {
      return "and";
    }
  };
}
#endif
