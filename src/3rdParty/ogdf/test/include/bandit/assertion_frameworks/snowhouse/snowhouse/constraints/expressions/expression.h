//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_EXPRESSION_H
#define SNOWHOUSE_EXPRESSION_H

#include "notexpression.h"
#include "andexpression.h"
#include "orexpression.h"

namespace snowhouse
{
  template<typename T>
  struct Expression
  {
    NotExpression<T> operator!() const
    {
      return NotExpression<T>(static_cast<const T&>(*this));
    }

    template<typename Right>
    AndExpression<T, Right> operator&&(const Right& right) const
    {
      return AndExpression<T, Right>(static_cast<const T&>(*this), right);
    }

    template<typename Right>
    OrExpression<T, Right> operator||(const Right& right) const
    {
      return OrExpression<T, Right>(static_cast<const T&>(*this), right);
    }
  };
}

#endif
