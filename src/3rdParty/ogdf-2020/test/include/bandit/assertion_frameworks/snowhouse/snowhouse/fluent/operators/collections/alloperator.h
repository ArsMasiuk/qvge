//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_ALLOPERATOR_H
#define SNOWHOUSE_ALLOPERATOR_H

#include "collectionoperator.h"
#include "collectionconstraintevaluator.h"

namespace snowhouse
{
  struct AllOperator : public CollectionOperator
  {
    template<typename ConstraintListType, typename ActualType>
    void Evaluate(ConstraintListType& list, ResultStack& result, OperatorStack& operators, const ActualType& actual)
    {
      unsigned int passed_elements = CollectionConstraintEvaluator<ConstraintListType, ActualType>::Evaluate(*this, list, result, operators, actual);

      result.push(passed_elements == actual.size());
    }
  };

  template<>
  struct Stringizer<AllOperator>
  {
    static std::string ToString(const AllOperator&)
    {
      return "all";
    }
  };
}

#endif
