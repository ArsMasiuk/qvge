//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_OREXPRESSION_H
#define SNOWHOUSE_OREXPRESSION_H

#include "../../stringize.h"
#include "expression_fwd.h"

namespace snowhouse
{
  template<typename LeftExpression, typename RightExpression>
  struct OrExpression : Expression<OrExpression<LeftExpression, RightExpression> >
  {
    OrExpression(const LeftExpression& left, const RightExpression& right)
        : m_left(left), m_right(right)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return (m_left(actual) || m_right(actual));
    }

    LeftExpression m_left;
    RightExpression m_right;
  };

  template<typename LeftExpression, typename RightExpression>
  struct Stringizer<OrExpression<LeftExpression, RightExpression> >
  {
    static std::string ToString(const OrExpression<LeftExpression, RightExpression>& expression)
    {
      std::ostringstream builder;
      builder << snowhouse::Stringize(expression.m_left) << " or " << snowhouse::Stringize(expression.m_right);

      return builder.str();
    }
  };
}

#endif
