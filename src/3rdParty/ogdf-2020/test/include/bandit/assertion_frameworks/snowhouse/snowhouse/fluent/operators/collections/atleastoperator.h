//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_ATLEASTOPERATOR_H
#define SNOWHOUSE_ATLEASTOPERATOR_H

#include "collectionoperator.h"
#include "collectionconstraintevaluator.h"

namespace snowhouse
{
  struct AtLeastOperator : public CollectionOperator
  {
    explicit AtLeastOperator(unsigned int expected)
        : m_expected(expected)
    {
    }

    template<typename ConstraintListType, typename ActualType>
    void Evaluate(ConstraintListType& list, ResultStack& result, OperatorStack& operators, const ActualType& actual)
    {
      unsigned int passed_elements = CollectionConstraintEvaluator<ConstraintListType, ActualType>::Evaluate(*this, list, result, operators, actual);

      result.push(passed_elements >= m_expected);
    }

    unsigned int m_expected;
  };

  template<>
  struct Stringizer<AtLeastOperator>
  {
    static std::string ToString(const AtLeastOperator& op)
    {
      std::ostringstream stm;
      stm << "at least " << op.m_expected;
      return stm.str();
    }
  };
}

#endif
