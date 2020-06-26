//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_CONTRAINTOPERATOR_H
#define SNOWHOUSE_CONTRAINTOPERATOR_H

#include "../../stringize.h"
#include "../constraintlist.h"
#include "invalidexpressionexception.h"

namespace snowhouse
{
  struct ConstraintOperator
  {
#if __cplusplus <= 199711L
    virtual ~ConstraintOperator()
    {
    }
#endif

    virtual void PerformOperation(ResultStack& result) = 0;
    virtual int Precedence() const = 0;

    template<typename ConstraintListType, typename ActualType>
    static bool EvaluateElementAgainstRestOfExpression(ConstraintListType& list, const ActualType& actual)
    {
      ResultStack innerResult;
      OperatorStack innerOperators;

      EvaluateConstraintList(list.m_tail, innerResult, innerOperators, actual);
      EvaluateAllOperatorsOnStack(innerOperators, innerResult);

      if (innerResult.empty())
      {
        throw InvalidExpressionException("The expression after \"" + snowhouse::Stringize(list.m_head) + "\" operator does not yield any result");
      }

      return innerResult.top();
    }

    static void EvaluateOperatorsWithLessOrEqualPrecedence(const ConstraintOperator& op, OperatorStack& operators, ResultStack& result)
    {
      while (!operators.empty())
      {
        ConstraintOperator* op_from_stack = operators.top();

        if (op_from_stack->Precedence() > op.Precedence())
        {
          break;
        }

        op_from_stack->PerformOperation(result);
        operators.pop();
      }
    }

    static void EvaluateAllOperatorsOnStack(OperatorStack& operators, ResultStack& result)
    {
      while (!operators.empty())
      {
        ConstraintOperator* op = operators.top();
        op->PerformOperation(result);
        operators.pop();
      }
    }
  };
}

#endif
