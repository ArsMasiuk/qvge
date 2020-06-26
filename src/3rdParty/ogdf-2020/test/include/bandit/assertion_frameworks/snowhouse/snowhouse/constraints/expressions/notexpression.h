//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_NOTEXPRESSION_H
#define SNOWHOUSE_NOTEXPRESSION_H

#include "../../stringize.h"
#include "expression_fwd.h"

namespace snowhouse
{
  template<typename ExpressionType>
  struct NotExpression : Expression<NotExpression<ExpressionType> >
  {
    explicit NotExpression(const ExpressionType& expression)
        : m_expression(expression)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return !m_expression(actual);
    }

    ExpressionType m_expression;
  };

  template<typename ExpressionType>
  struct Stringizer<NotExpression<ExpressionType> >
  {
    static std::string ToString(const NotExpression<ExpressionType>& expression)
    {
      std::ostringstream builder;
      builder << "not " << snowhouse::Stringize(expression.m_expression);

      return builder.str();
    }
  };
}

#endif
