// Copyright (C) 2017 Stephan Beyer
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_ISEMPTYCONSTRAINT_H
#define SNOWHOUSE_ISEMPTYCONSTRAINT_H

#include "expressions/expression.h"

namespace snowhouse
{
  struct IsEmptyConstraint : Expression<IsEmptyConstraint>
  {
    // The ignored default argument is a workaround to make this class
    // compatible to ConstraintAdapterType
    IsEmptyConstraint(int = 0)
    {
    }

    template<typename ActualType>
    bool operator()(const ActualType& actual) const
    {
      return actual.empty();
    }
  };

  inline IsEmptyConstraint IsEmpty()
  {
    return IsEmptyConstraint();
  }

  template<>
  struct Stringizer<IsEmptyConstraint>
  {
    static std::string ToString(const IsEmptyConstraint&)
    {
      return "empty";
    }
  };
}

#endif
