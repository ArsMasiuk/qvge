//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_NOTOPERATOR_H
#define SNOWHOUSE_NOTOPERATOR_H

#include "constraintoperator.h"

namespace snowhouse
{
  struct NotOperator : public ConstraintOperator
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
      if (result.size() < 1)
      {
        throw InvalidExpressionException("The expression contains a not operator without any operand");
      }

      bool right = result.top();
      result.pop();

      result.push(!right);
    }

    int Precedence() const
    {
      return 2;
    }
  };

  template<>
  struct Stringizer<NotOperator>
  {
    static std::string ToString(const NotOperator&)
    {
      return "not";
    }
  };
}

#endif
