//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_CONSTRAINTLIST_H
#define SNOWHOUSE_CONSTRAINTLIST_H

#include <stack>

namespace snowhouse
{
  struct ConstraintOperator;
  typedef std::stack<bool> ResultStack;
  typedef std::stack<ConstraintOperator*> OperatorStack;

  template<typename HT, typename TT>
  struct ConstraintList
  {
    typedef HT HeadType;
    typedef TT TailType;

    ConstraintList(const HeadType& head, const TailType& tail)
        : m_head(head), m_tail(tail)
    {
    }

    HeadType m_head;
    TailType m_tail;
  };

  struct Nil
  {
    Nil()
    {
    }

    Nil(const Nil&)
    {
    }
  };

  // ---- These structs defines the resulting types of list concatenation operations
  template<typename L1, typename L2>
  struct type_concat
  {
    typedef ConstraintList<typename L1::HeadType, typename type_concat<typename L1::TailType, L2>::t> t;
  };

  template<typename L2>
  struct type_concat<Nil, L2>
  {
    typedef L2 t;
  };

  template<typename L3>
  inline L3 tr_concat(const Nil&, const Nil&)
  {
    return Nil();
  }

  // ---- These structs define the concatenation operations.

  template<typename LeftList, typename RightList, typename ResultList>
  struct ListConcat
  {
    static ResultList Concatenate(const LeftList& left, const RightList& right)
    {
      return ResultList(left.m_head, ListConcat<typename LeftList::TailType, RightList, typename type_concat<typename LeftList::TailType, RightList>::t>::Concatenate(left.m_tail, right));
    }
  };

  // Concatenating an empty list with a second list yields the second list
  template<typename RightList, typename ResultList>
  struct ListConcat<Nil, RightList, ResultList>
  {
    static ResultList Concatenate(const Nil&, const RightList& right)
    {
      return right;
    }
  };

  // Concatenating two empty lists yields an empty list
  template<typename ResultList>
  struct ListConcat<Nil, Nil, ResultList>
  {
    static ResultList Concatenate(const Nil&, const Nil&)
    {
      return Nil();
    }
  };

  // ---- The concatenation operation

  template<typename L1, typename L2>
  inline typename type_concat<L1, L2>::t Concatenate(const L1& list1, const L2& list2)
  {
    return ListConcat<L1, L2, typename type_concat<L1, L2>::t>::Concatenate(list1, list2);
  }
}

#endif
