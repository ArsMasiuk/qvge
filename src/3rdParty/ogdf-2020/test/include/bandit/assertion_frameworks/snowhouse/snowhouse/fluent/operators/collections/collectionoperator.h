//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_COLLECTIONOPERATOR_H
#define SNOWHOUSE_COLLECTIONOPERATOR_H

#include "../constraintoperator.h"

namespace snowhouse
{
  struct CollectionOperator : public ConstraintOperator
  {
    void PerformOperation(ResultStack&)
    {
    }

    int Precedence() const
    {
      return 1;
    }
  };
}

#endif
