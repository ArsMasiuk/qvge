//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_STRINGIZE_H
#define SNOWHOUSE_STRINGIZE_H

#include <iostream>
#include <sstream>

#include "macros.h"

namespace snowhouse
{
  namespace typing
  {
    // This type soaks up any implicit conversions and makes the following operator<<
    // less preferred than any other such operator found via ADL.
    struct any
    {
      // Conversion constructor for any type.
      template<typename T>
      any(T const&);
    };

    // A tag type returned by operator<< for the any struct in this namespace
    // when T does not support <<.
    struct tag
    {
    };

    // Fallback operator<< for types T that don't support <<.
    tag operator<<(std::ostream&, any const&);

    // Two overloads to distinguish whether T supports a certain operator expression.
    // The first overload returns a reference to a two-element character array and is chosen if
    // T does not support the expression, such as <<, whereas the second overload returns a char
    // directly and is chosen if T supports the expression. So using sizeof(check(<expression>))
    // returns 2 for the first overload and 1 for the second overload.
    typedef char yes;
    typedef char (&no)[2];

    no check(tag);

    template<typename T>
    yes check(T const&);

    template<typename T>
    struct is_output_streamable
    {
      static const T& x;
      static const bool value = sizeof(check(std::cout << x)) == sizeof(yes);
    };

#ifdef SNOWHOUSE_HAS_NULLPTR
    template<>
    struct is_output_streamable<std::nullptr_t>
    {
      static const bool value = false;
    };
#endif
  }

  namespace detail
  {
    template<typename T, bool type_is_streamable>
    struct DefaultStringizer
    {
      static std::string ToString(const T& value)
      {
        std::ostringstream buf;
        buf << value;
        return buf.str();
      }
    };

    template<typename T>
    struct DefaultStringizer<T, false>
    {
      static std::string ToString(const T&)
      {
        return "[unsupported type]";
      }
    };
  }

  template<typename T, typename = typing::yes>
  struct Stringizer;

  template<typename T>
  std::string Stringize(const T& value)
  {
    return Stringizer<T>::ToString(value);
  }

  // NOTE: Specialize snowhouse::Stringizer to customize assertion messages
  template<typename T, typename>
  struct Stringizer
  {
    static std::string ToString(const T& value)
    {
      return detail::DefaultStringizer<T, typing::is_output_streamable<T>::value>::ToString(value);
    }
  };

#ifdef SNOWHOUSE_HAS_NULLPTR
  // We need this because nullptr_t has ambiguous overloads of operator<< in the standard library.
  template<>
  struct Stringizer<std::nullptr_t>
  {
    static std::string ToString(std::nullptr_t)
    {
      return "nullptr";
    }
  };
#endif
}

#endif
