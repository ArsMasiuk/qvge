snowhouse
=========
[![Travis CI Status](https://travis-ci.org/banditcpp/snowhouse.svg?branch=master)](https://travis-ci.org/banditcpp/snowhouse)
[![AppVeyor Status](https://ci.appveyor.com/api/projects/status/github/banditcpp/snowhouse?branch=master&svg=true)](https://ci.appveyor.com/project/banditcpp/snowhouse)

An assertion library for C++

Snowhouse is a stand-alone assertion framework for C++.

## Usage

```C++
#include <snowhouse/snowhouse.h>

using namespace snowhouse;

int main()
{
  std::cout << "Testing that 23 is 23" << std::endl;
  AssertThat(23, Is().EqualTo(23));

  try
  {
    AssertThat(12, Is().LessThan(11).And().GreaterThan(99));
  }
  catch(const AssertionException& ex)
  {
    std::cout << "Apparently this failed:" << std::endl;
    std::cout << ex.GetMessage() << std::endl;
  }

  return 0;
}
```

### Assertions

Snowhouse uses a constraint-based assertion model that is heavily inspired by the
model used in [NUnit](http://nunit.org/). An assertion in Snowhouse is written
using the following format:

```cpp
AssertThat(actual_value, <constraint expression>);
```

where `<constraint expression>` is an expression that `actual_value` is
evaluated against when the test is executed.

Constraint expressions come in two basic forms: composite and fluent expressions.

#### Composite Expressions

With composite expressions, you can create compact, powerful expressions that
combine a set of predefined constraints with ones that you provide yourself.

Example:

```cpp
AssertThat(length, IsGreaterThan(4) && !Equals(10));
```

Composite expressions can be any combination of constraints and the
standard logical C++ operators.

You can also add your own constraints to be used within composite expressions.

#### Fluent Expressions

With fluent expressions, you can create assertions that better convey the intent
of a test without exposing implementation-specific details.
Fluent expressions aim to help you create tests that are not just by developers
for developers, but rather can be read and understood by domain experts.

Fluent expressions also have the ability to make assertions on the elements in a
container in a way you cannot achieve with composite expressions.

Example:

```cpp
AssertThat(length, Is().GreaterThan(4).And().Not().EqualTo(10));
```

### Basic Constraints

#### Equality Constraint

Used to verify equality between actual and expected.

```cpp
AssertThat(x, Equals(12));
AssertThat(x, Is().EqualTo(12));
```

#### EqualityWithDelta Constraint

Used to verify equality between actual and expected,
allowing the two to differ by a delta.

```cpp
AssertThat(2.49, EqualsWithDelta(2.5, 0.1));
AssertThat(2.49, Is().EqualToWithDelta(2.5, 0.1));
```

#### GreaterThan Constraint

Used to verify that actual is greater than a value.

```cpp
AssertThat(x, IsGreaterThan(4));
AssertThat(x, Is().GreaterThan(4));
```

#### LessThan Constraint

Used to verify that actual is less than a value.

```cpp
AssertThat(x, IsLessThan(3));
AssertThat(x, Is().LessThan(3));
```

#### GreaterThanOrEqualTo Constraint

Used to verify that actual is greater than or equal to a value.

```cpp
AssertThat(x, IsGreaterThanOrEqualTo(5));
AssertThat(x, Is().GreaterThanOrEqualTo(5));
```

#### LessThanOrEqualTo Constraint

Used to verify that actual is less than or equal to a value.

```cpp
AssertThat(x, IsLessThanOrEqualTo(6));
AssertThat(x, Is().LessThanOrEqualTo(6));
```

### Pointer Constraints

Used to check for `nullptr` equality.

```cpp
AssertThat(x, IsNull());
AssertThat(x, Is().Null());
```

Note that this feature is only available for C++11-compliant compilers.
In this case, the `SNOWHOUSE_HAS_NULLPTR` macro is defined.

### String Constraints

String assertions in Snowhouse are used to verify the values of
STL strings (`std::string`).

#### Equality Constraints

Used to verify that actual is equal to an expected value.

```cpp
AssertThat(actual_str, Equals("foo"));
AssertThat(actual_str, Is().EqualTo("foo"));
```

#### Contains Constraint

Used to verify that a string contains a substring.

```cpp
AssertThat(actual_str, Contains("foo"));
AssertThat(actual_str, Is().Containing("foo"));
```

#### EndsWith Constraint

Used to verify that a string ends with an expected substring.

```cpp
AssertThat(actual_str, EndsWith("foo"));
AssertThat(actual_str, Is().EndingWith("foo"));
```

#### StartsWith Constraint

Used to verify that a string starts with an expected substring.

```cpp
AssertThat(actual_str, StartsWith("foo"));
AssertThat(actual_str, Is().StartingWith("foo"));
```

#### HasLength Constraint

Used to verify that a string is of the expected length.

```cpp
AssertThat(actual_str, HasLength(5));
AssertThat(actual_str, Is().OfLength(5));
```

### Constraints on Multiline Strings

If you have a string that contains multiple lines, you can use the collection
constraints to make assertions on the content of that string.
This may be handy if you have a string that, for instance, represents the
resulting content of a file or a network transmission.

Snowhouse can handle both Windows (CR+LF) and Unix (LF) line endings.

```cpp
std::string lines = "First line\r\nSecond line\r\nThird line";
AssertThat(lines, Has().Exactly(1).StartingWith("Second"));
```

### Container Constraints

The following constraints can be applied to containers in the standard template library.

#### Contains Constraint

Used to verify that a container contains an expected value.

```cpp
AssertThat(container, Contains(12));
AssertThat(container, Is().Containing(12));
```

#### HasLength Constraint

Used to verify that a container has the expected length.

```cpp
AssertThat(container, HasLength(3));
AssertThat(container, Is().OfLength(3));
```

#### IsEmpty Constraint

Used to verify that a container is empty.

```cpp
AssertThat(container, IsEmpty());
AssertThat(container, Is().Empty());
```

#### All

Used to verify that all elements of a STL sequence container matches an expectation.

```cpp
AssertThat(container, Has().All().LessThan(5).Or().EqualTo(66));
```

#### AtLeast

Used to verify that at least a specified amount of elements in a STL sequence
container matches an expectation.

```cpp
AssertThat(container, Has().AtLeast(3).StartingWith("foo"));
```

#### AtMost

Used to verify that at most a specified amount of elements in a STL sequence
container matches an expectation.

```cpp
Assert:That(container, Has().AtMost(2).Not().Containing("failed"));
```

#### Exactly

Used to verify that a STL sequence container has exactly a specified amount
of elements that matches an expectation.

```cpp
AssertThat(container, Has().Exactly(3).GreaterThan(10).And().LessThan(20));
```

#### EqualsContainer

Used to verify that two STL sequence containers are equal.

```cpp
AssertThat(container1, EqualsContainer(container2));
AssertThat(container1, Is().EqualToContainer(container2));
```

##### Predicate functions

You can supply a predicate function or a functor to `EqualsContainer` to
customize how to compare the elements in the two containers.

With a predicate function:

```cpp
static bool are_my_types_equal(const my_type& lhs, const my_type& rhs)
{
  return lhs.my_val_ == rhs.my_val_;
}

AssertThat(container1, EqualsContainer(container2, are_my_types_equal));
```

With a functor as predicate:

```cpp
struct within_delta
{
  within_delta(int delta) : delta_(delta) {}

  bool operator()(const my_type& lhs, const my_type& rhs) const
  {
    return abs(lhs.my_val_ - rhs.my_val_) <= delta_;
  }

private:
  int delta_;
};

AssertThat(container1, Is().EqualToContainer(container1, within_delta(1));
```

### Exceptions

Exception constraints can be used to verify that your code throws the correct exceptions.

#### AssertThrows

`AssertThrows` succeeds if the exception thrown by the call is of the supplied
type (or one of its subtypes).

```cpp
AssertThrows(std::logic_error, myObject.a_method(42));
```

#### Making Assertions on the Thrown Exceptions

If `AssertThrows` succeeds, it will store the thrown exception so that you can
make more detailed assertions on it.

```cpp
AssertThrows(std::logic_error, myObject.a_method(42));
AssertThat(LastException<std::logic_error>().what(), Is().Containing("logic failure"));
```

The `LastException<>` is available in the scope of the call to `AssertThrows`.
An exception is not available between specs in order to avoid the result of
one spec contaminating another.

### Custom Constraints

You can add your own constraints to Snowhouse to create more expressive specifications.

#### Fulfills Constraints

By defining the following matcher

```cpp
struct IsEvenNumber
{
  bool Matches(const int actual) const
  {
    return (actual % 2) == 0;
  }

  friend std::ostream& operator<<(std::ostream& stm, const IsEvenNumber& );
};

std::ostream& operator<<(std::ostream& stm, const IsEvenNumber& )
{
  stm << "An even number";
  return stm;
}
```

You can create the following constraints in Snowhouse:

```cpp
AssertThat(42, Fulfills(IsEvenNumber()));
AssertThat(42, Is().Fulfilling(IsEvenNumber()));
```

Your custom matcher should implement a method called `Matches()` that takes
a parameter of the type you expect and returns true if the passed parameter
fulfills the constraint.

To get more expressive failure messages, you should also implement the
streaming operator as in the example above.

## Getting better output for your types

Whenever Snowhouse prints an error message for a type, it will use the
stream operator for that type, otherwise it will print "[unsupported type]"
as a placeholder.

```cpp
struct MyType { int x; char c; };

AssertThat(myType, Fulfills(MyConstraint());
```

Will output the following if the constraint fails:

```
Expected: To fulfill my constraint
Actual: [unsupported type]
```

If we add a stream operator:

```cpp
std::ostream& operator<<(std::ostream& stream, const MyType& a)
{
  stream << a.c << a.x;
  return stream;
}
```

the output will be a bit more readable:

```
Expected: To fulfill my constraint
Actual: f23
```

If it is necessary to print an object in a different manner than the
usual output stream operator provides, for example, to output more detailed
information, we can use a specialization of the `Stringizer` class template:

```cpp
namespace snowhouse
{
  template<>
  struct Stringizer<MyType>
  {
    static std::string ToString(const MyType& a)
    {
      std::stringstream stream;
      stream << "MyType(x = " << a.x << ", c = " << int(a.c) << "('" << a.c << "'))";
      return stream.str();
    }
  };
}
```

with output:

```
Expected: To fulfill my constraint
Actual: MyType(x = 23, c = 102('f'))
```

## Configurable Failure Handlers

You can provide Snowhouse with custom failure handlers, for example to
call `std::terminate` instead of throwing an exception.
See `DefaultFailureHandler` for an example of a failure handler.
You can derive your own macros with custom failure handlers using
`SNOWHOUSE_ASSERT_THAT` and `SNOWHOUSE_ASSERT_THROWS`.
See the definitions of `AssertThat` and `AssertThrows` for examples of these.
Define `SNOWHOUSE_NO_MACROS` to disable the unprefixed macros `AssertThat`
and `AssertThrows`.

### Example Use Cases

#### Assert Program State

Log an error immediately as we may crash if we try to continue.
Do not attempt to unwind the stack as we may be inside a destructor
or `nothrow` function.
We may want to call `std::terminate`, or attempt to muddle along
with the rest of the program.

#### Assert Program State in Safe Builds

As above, but only in debug builds.

#### Test Assert

Assert that a test behaved as expected.
Throw an exception and let our testing framework deal with the test failure.

## Versioning

Snowhouse uses [Semantic Versioning 2.0.0](http://semver.org/spec/v2.0.0.html) since
version 3.0.0.
The macros `SNOWHOUSE_MAJOR`, `SNOWHOUSE_MINOR` and `SNOWHOUSE_PATCH` are defined
accordingly and `SNOWHOUSE_VERSION` contains the version string.
Note that in prior versions `SNOWHOUSE_VERSION` was the only defined macro.

## Contributing

The development of Snowhouse takes place on [GitHub](//github.com/banditcpp/snowhouse).

Snowhouse is licensed under the Boost Software License.
See LICENSE_1_0.txt for further information.

By making available code for inclusion into Snowhouse (e.g., by opening a
pull request on GitHub), you guarantee that the code is licensed under the
same license as Snowhouse.

Please make sure to be consistent with the project's coding style.
The `.clang-format` file allows easy checking and implementation of the
coding style.

C++ code should comply to C++98, C++03- and C++11.
Please use `__cplusplus` guards if you want to use language features of
a certain C++ version.

## Responsibilities

Snowhouse was originally developed as part of the [Igloo](//github.com/joakimkarlsson/igloo)
testing framework by [Joakim Karlsson](//github.com/joakimkarlsson).
It has been extracted to be usable in other contexts, for example,
[Bandit](//github.com/banditcpp/bandit).

Snowhouse is maintained by [Stephan Beyer](//github.com/sbeyer) since
[October 2016](//twitter.com/JHKarlsson/status/789332548799332352).
