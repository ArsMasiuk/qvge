//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_EXPRESSIONBUILDER_H
#define SNOWHOUSE_EXPRESSIONBUILDER_H

#include "../constraints/constraints.h"
#include "constraintadapter.h"
#include "operators/andoperator.h"
#include "operators/notoperator.h"
#include "operators/oroperator.h"
#include "operators/collections/alloperator.h"
#include "operators/collections/noneoperator.h"
#include "operators/collections/atleastoperator.h"
#include "operators/collections/exactlyoperator.h"
#include "operators/collections/atmostoperator.h"

namespace snowhouse
{
  // ---- Evaluation of list of constraints

  template<typename ConstraintListType, typename ActualType>
  inline void EvaluateConstraintList(ConstraintListType& constraint_list, ResultStack& result, OperatorStack& operators, const ActualType& actual)
  {
    constraint_list.m_head.Evaluate(constraint_list, result, operators, actual);
  }

  template<typename ActualType>
  inline void EvaluateConstraintList(Nil&, ResultStack&, OperatorStack&, const ActualType&)
  {
  }

  template<typename ConstraintListType>
  struct ExpressionBuilder
  {
    explicit ExpressionBuilder(const ConstraintListType& list)
        : m_constraint_list(list)
    {
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EqualsConstraint<ExpectedType> >, Nil> >::t>
    EqualTo(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<EqualsConstraint<ExpectedType> > ConstraintAdapterType;
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;

      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());

      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename ExpectedType, typename DeltaType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EqualsWithDeltaConstraint<ExpectedType, DeltaType> >, Nil> >::t>
    EqualToWithDelta(const ExpectedType& expected, const DeltaType& delta)
    {
      typedef ConstraintAdapter<EqualsWithDeltaConstraint<ExpectedType, DeltaType> > ConstraintAdapterType;
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;

      ConstraintAdapterType constraint(EqualsWithDeltaConstraint<ExpectedType, DeltaType>(expected, delta));
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());

      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename MatcherType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<FulfillsConstraint<MatcherType> >, Nil> >::t>
    Fulfilling(const MatcherType& matcher)
    {
      typedef ConstraintAdapter<FulfillsConstraint<MatcherType> > ConstraintAdapterType;
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;

      ConstraintAdapterType constraint(matcher);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());

      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EqualsConstraint<bool> >, Nil> >::t>
    False()
    {
      return EqualTo<bool>(false);
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EqualsConstraint<bool> >, Nil> >::t>
    True()
    {
      return EqualTo<bool>(true);
    }

#ifdef SNOWHOUSE_HAS_NULLPTR
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EqualsConstraint<std::nullptr_t> >, Nil> >::t>
    Null()
    {
      return EqualTo<std::nullptr_t>(nullptr);
    }
#endif

    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EqualsConstraint<std::string> >, Nil> >::t>
    EqualTo(const char* expected)
    {
      return EqualTo<std::string>(std::string(expected));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<IsGreaterThanConstraint<ExpectedType> >, Nil> >::t>
    GreaterThan(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<IsGreaterThanConstraint<ExpectedType> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<IsGreaterThanOrEqualToConstraint<ExpectedType> >, Nil> >::t>
    GreaterThanOrEqualTo(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<IsGreaterThanOrEqualToConstraint<ExpectedType> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<IsLessThanConstraint<ExpectedType> >, Nil> >::t>
    LessThan(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<IsLessThanConstraint<ExpectedType> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<IsLessThanOrEqualToConstraint<ExpectedType> >, Nil> >::t>
    LessThanOrEqualTo(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<IsLessThanOrEqualToConstraint<ExpectedType> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<ContainsConstraint<ExpectedType> >, Nil> >::t>
    Containing(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<ContainsConstraint<ExpectedType> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<ContainsConstraint<std::string> >, Nil> >::t>
    Containing(const char* expected)
    {
      return Containing<std::string>(std::string(expected));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EndsWithConstraint<ExpectedType> >, Nil> >::t>
    EndingWith(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<EndsWithConstraint<ExpectedType> > ConstraintAdapterType;
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;

      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EndsWithConstraint<std::string> >, Nil> >::t>
    EndingWith(const char* expected)
    {
      return EndingWith(std::string(expected));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<StartsWithConstraint<ExpectedType> >, Nil> >::t>
    StartingWith(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<StartsWithConstraint<ExpectedType> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<StartsWithConstraint<std::string> >, Nil> >::t>
    StartingWith(const char* expected)
    {
      return StartingWith(std::string(expected));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<HasLengthConstraint<ExpectedType> >, Nil> >::t>
    OfLength(const ExpectedType& expected)
    {
      typedef ConstraintAdapter<HasLengthConstraint<ExpectedType> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(expected);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<IsEmptyConstraint>, Nil> >::t>
    Empty()
    {
      typedef ConstraintAdapter<IsEmptyConstraint> ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(0);
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename ExpectedType>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EqualsContainerConstraint<ExpectedType, bool (*)(const typename ExpectedType::value_type&, const typename ExpectedType::value_type&)> >, Nil> >::t>
    EqualToContainer(const ExpectedType& expected)
    {
      typedef bool (*DefaultBinaryPredivateType)(const typename ExpectedType::value_type&, const typename ExpectedType::value_type&);
      typedef ConstraintAdapter<EqualsContainerConstraint<ExpectedType, DefaultBinaryPredivateType> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(EqualsContainerConstraint<ExpectedType, DefaultBinaryPredivateType>(expected, constraint_internal::default_comparer<typename ExpectedType::value_type>));
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename ExpectedType, typename BinaryPredicate>
    ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapter<EqualsContainerConstraint<ExpectedType, BinaryPredicate> >, Nil> >::t>
    EqualToContainer(const ExpectedType& expected, const BinaryPredicate predicate)
    {
      typedef ConstraintAdapter<EqualsContainerConstraint<ExpectedType, BinaryPredicate> > ConstraintAdapterType;

      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ConstraintList<ConstraintAdapterType, Nil> >::t> BuilderType;
      ConstraintAdapterType constraint(EqualsContainerConstraint<ExpectedType, BinaryPredicate>(expected, predicate));
      ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    typedef ConstraintList<AndOperator, Nil> AndOperatorNode;
    typedef ConstraintList<OrOperator, Nil> OrOperatorNode;
    typedef ConstraintList<NotOperator, Nil> NotOperatorNode;
    typedef ConstraintList<AllOperator, Nil> AllOperatorNode;
    typedef ConstraintList<AtLeastOperator, Nil> AtLeastOperatorNode;
    typedef ConstraintList<ExactlyOperator, Nil> ExactlyOperatorNode;
    typedef ConstraintList<AtMostOperator, Nil> AtMostOperatorNode;
    typedef ConstraintList<NoneOperator, Nil> NoneOperatorNode;

    ExpressionBuilder<typename type_concat<ConstraintListType, AllOperatorNode>::t> All()
    {
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, AllOperatorNode>::t> BuilderType;
      AllOperator op;
      AllOperatorNode node(op, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, AtLeastOperatorNode>::t> AtLeast(unsigned int expected)
    {
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, AtLeastOperatorNode>::t> BuilderType;
      AtLeastOperator op(expected);
      AtLeastOperatorNode node(op, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, ExactlyOperatorNode>::t> Exactly(unsigned int expected)
    {
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, ExactlyOperatorNode>::t> BuilderType;
      ExactlyOperator op(expected);
      ExactlyOperatorNode node(op, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, AtMostOperatorNode>::t> AtMost(unsigned int expected)
    {
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, AtMostOperatorNode>::t> BuilderType;
      AtMostOperator op(expected);
      AtMostOperatorNode node(op, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, NoneOperatorNode>::t> None()
    {
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, NoneOperatorNode>::t> BuilderType;
      NoneOperator op;
      NoneOperatorNode node(op, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, AndOperatorNode>::t> And()
    {
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, AndOperatorNode>::t> BuilderType;
      AndOperator op;
      AndOperatorNode node(op, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, OrOperatorNode>::t> Or()
    {
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, OrOperatorNode>::t> BuilderType;
      OrOperator op;
      OrOperatorNode node(op, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    ExpressionBuilder<typename type_concat<ConstraintListType, NotOperatorNode>::t> Not()
    {
      typedef ExpressionBuilder<typename type_concat<ConstraintListType, NotOperatorNode>::t> BuilderType;
      NotOperator op;
      NotOperatorNode node(op, Nil());
      return BuilderType(Concatenate(m_constraint_list, node));
    }

    template<typename ActualType>
    void Evaluate(ResultStack& result, OperatorStack& operators, const ActualType& actual)
    {
      EvaluateConstraintList(m_constraint_list, result, operators, actual);
    }

    ConstraintListType m_constraint_list;
  };

  template<typename T>
  inline void StringizeConstraintList(const T& list, std::ostringstream& stm)
  {
    if (stm.tellp() > 0)
      stm << " ";

    stm << snowhouse::Stringize(list.m_head);
    StringizeConstraintList(list.m_tail, stm);
  }

  inline void StringizeConstraintList(const Nil&, std::ostringstream&)
  {
  }

  template<typename ConstraintListType>
  struct Stringizer<ExpressionBuilder<ConstraintListType> >
  {
    static std::string ToString(const ExpressionBuilder<ConstraintListType>& builder)
    {
      std::ostringstream stm;
      StringizeConstraintList(builder.m_constraint_list, stm);

      return stm.str();
    }
  };
}

#endif
