//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_STRINGIZERS_H
#define SNOWHOUSE_STRINGIZERS_H

#include "stringize.h"

namespace snowhouse
{
  namespace detail
  {
    template<typename Container>
    struct SequentialContainerStringizer
    {
      static std::string
      ToString(const Container& cont)
      {
        std::ostringstream stm;
        typedef typename Container::const_iterator Iterator;

        stm << "[ ";
        for (Iterator it = cont.begin(); it != cont.end();)
        {
          stm << snowhouse::Stringize(*it);

          if (++it != cont.end())
          {
            stm << ", ";
          }
        }
        stm << " ]";
        return stm.str();
      }
    };
  }

  namespace typing
  {
    template<typename T>
    struct is_const_iterable
    {
      template<typename CLASS>
      static yes compile_time_check_const_iterator(typename CLASS::const_iterator*);

      template<typename>
      static no compile_time_check_const_iterator(...);

      static const bool value = sizeof(compile_time_check_const_iterator<T>(0)) == sizeof(yes);
    };

    template<typename T, bool = is_const_iterable<T>::value>
    struct is_container
    {
      typedef typename T::const_iterator Iterator;

      struct FallbackBeginEnd
      {
        Iterator begin() const;
        Iterator end() const;
      };

      typedef Iterator (FallbackBeginEnd::*fallback_method)() const;

      template<typename CLASS, CLASS>
      struct is_of_type;

      template<typename CLASS>
      static no compile_time_check_begin(is_of_type<fallback_method, &CLASS::begin>*);

      template<typename CLASS>
      static no compile_time_check_end(is_of_type<fallback_method, &CLASS::end>*);

      template<typename>
      static yes compile_time_check_begin(...);

      template<typename>
      static yes compile_time_check_end(...);

      struct FallbackContainer : T, FallbackBeginEnd
      {
      };

      static const bool has_begin = sizeof(compile_time_check_begin<FallbackContainer>(0)) == sizeof(yes);
      static const bool has_end = sizeof(compile_time_check_end<FallbackContainer>(0)) == sizeof(yes);

      static const bool value = has_begin && has_end;
    };

    template<typename T>
    struct is_container<T, false>
    {
      static const bool value = false;
    };

    template<>
    struct is_container<std::string>
    {
      static const bool value = false;
    };

    template<bool, typename = yes>
    struct enable_if
    {
    };

    template<typename TRUE_TYPE>
    struct enable_if<true, TRUE_TYPE>
    {
      typedef TRUE_TYPE type;
    };
  }

  template<typename T>
  struct Stringizer<T,
      typename typing::enable_if<typing::is_container<T>::value>::type>
      : detail::SequentialContainerStringizer<T>
  {
  };
}

#endif
