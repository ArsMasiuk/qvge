//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_COLLECTIONCONSTRAINTEVALUATOR_H
#define SNOWHOUSE_COLLECTIONCONSTRAINTEVALUATOR_H

#include <vector>

#include "../constraintoperator.h"

namespace snowhouse
{
  template<typename ConstraintListType, typename ActualType>
  struct CollectionConstraintEvaluator
  {
    static unsigned int Evaluate(const ConstraintOperator& op,
        ConstraintListType& expression, ResultStack& result,
        OperatorStack& operators, const ActualType& actual)
    {
      ConstraintOperator::EvaluateOperatorsWithLessOrEqualPrecedence(op,
          operators, result);

      unsigned int passed_elements = 0;
      typename ActualType::const_iterator it;
      for (it = actual.begin(); it != actual.end(); ++it)
      {
        if (ConstraintOperator::EvaluateElementAgainstRestOfExpression(expression, *it))
        {
          ++passed_elements;
        }
      }

      return passed_elements;
    }
  };

  struct StringLineParser
  {
    static void Parse(const std::string& str, std::vector<std::string>& res)
    {
      size_t start = 0;
      size_t newline = FindNewline(str, start);

      while (newline != std::string::npos)
      {
        StoreLine(str, start, newline, res);
        start = MoveToNextLine(str, newline);
        newline = FindNewline(str, start);
      }

      if (start < str.size())
      {
        StoreLine(str, start, std::string::npos, res);
      }
    }

  private:
    static size_t FindNewline(const std::string& str, size_t start)
    {
      return str.find_first_of("\r\n", start);
    }

    static void StoreLine(const std::string& str, size_t start, size_t end,
        std::vector<std::string>& res)
    {
      std::string line = str.substr(start, end - start);
      res.push_back(line);
    }

    static size_t MoveToNextLine(const std::string& str, size_t newline)
    {
      if (str.find("\r\n", newline) == newline)
      {
        return newline + 2;
      }

      if (str.find("\n", newline) == newline)
      {
        return newline + 1;
      }

      if (str.find("\r", newline) == newline)
      {
        return newline + 1;
      }

      std::ostringstream stm;
      stm << "This string seems to contain an invalid line ending at position "
          << newline << ":" << std::endl
          << str << std::endl;
      throw InvalidExpressionException(stm.str());
    }
  };

  template<typename ConstraintListType>
  struct CollectionConstraintEvaluator<ConstraintListType, std::string>
  {
    static unsigned int Evaluate(const ConstraintOperator& op,
        ConstraintListType& expression, ResultStack& result,
        OperatorStack& operators, const std::string& actual)
    {
      std::vector<std::string> lines;
      StringLineParser::Parse(actual, lines);
      return CollectionConstraintEvaluator<ConstraintListType, std::vector<std::string> >::Evaluate(op, expression, result, operators, lines);
    }
  };
}

#endif
