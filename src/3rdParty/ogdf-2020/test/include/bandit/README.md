bandit
======
[![Travis CI Status](https://travis-ci.org/banditcpp/bandit.svg?branch=master)](https://travis-ci.org/banditcpp/bandit)
[![AppVeyor Status](https://ci.appveyor.com/api/projects/status/github/banditcpp/bandit?branch=master&svg=true)](https://ci.appveyor.com/project/banditcpp/bandit)

Human-friendly unit testing for C++11

Bandit is a framework for C++11 that wants to make working with unit tests a
pleasant experience.

Bandit is released under the [MIT license](LICENSE.txt)

Please note that
[this repository](https://github.com/ogdf/bandit) is a fork of the
[original bandit framework](https://github.com/joakimkarlsson/bandit) v1.1.4.
The fork is used within the [Open Graph Drawing Framework](http://ogdf.net).

## An example

This is a complete test application written in bandit:

```c++
#include <bandit/bandit.h>

using namespace snowhouse;
using namespace bandit;

// Tell bandit there are tests here.
go_bandit([]() {
  // We're describing how a fuzzbox works.
  describe("fuzzbox:", []() {
    guitar_ptr guitar;
    fuzzbox_ptr fuzzbox;

    // Make sure each test has a fresh setup with
    // a guitar with a fuzzbox connected to it.
    before_each([&]() {
      guitar = guitar_ptr(new struct guitar());
      fuzzbox = fuzzbox_ptr(new struct fuzzbox());
      guitar->add_effect(fuzzbox.get());
    });

    it("starts in clean mode", [&]() {
      AssertThat(guitar->sound(), Equals(sounds::clean));
    });

    // Describe what happens when we turn on the fuzzbox.
    describe("in distorted mode", [&]() {
      // Turn on the fuzzbox.
      before_each([&]() {
        fuzzbox->flip();
      });

      it("sounds distorted", [&]() {
        AssertThat(guitar->sound(), Equals(sounds::distorted));
      });
    });
  });
});

int main(int argc, char* argv[]) {
  // Run the tests.
  return bandit::run(argc, argv);
}
```

## Installing

Bandit is header-only, so there is no need for additional compilation before you
can start using it. Download bandit and add its root directory to your project's
include directories and you are ready to go.

## Compilers

Bandit has been tested with the following compilers:

 * GCC ≥ 4.5
 * Clang ≥ 3.2
 * MSVC ≥ 2012

If you want to see if bandit works for your compiler, bandit is shipped with a
cmake project for generating bandit's self tests. Let us know how it goes.

If your compiler does not support the C++11 features required by Bandit, we
suggest that you take a look at [Igloo](http://igloo-testing.org), which is
built on the same philosophy but works without C++11.

## Online resources

 * [The bandit website](//banditcpp.github.io/bandit/)
 * [Bandit on GitHub](//github.com/banditcpp/bandit)
 * [Issues with bandit](//github.com/banditcpp/bandit/issues?state=open)
