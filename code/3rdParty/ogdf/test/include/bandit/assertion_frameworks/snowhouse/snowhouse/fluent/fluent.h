//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_FLUENT_H
#define SNOWHOUSE_FLUENT_H

#include "expressionbuilder.h"

namespace snowhouse
{
  inline ExpressionBuilder<Nil> Is()
  {
    return ExpressionBuilder<Nil>(Nil());
  }

  inline ExpressionBuilder<Nil> Has()
  {
    return ExpressionBuilder<Nil>(Nil());
  }
}

#endif
