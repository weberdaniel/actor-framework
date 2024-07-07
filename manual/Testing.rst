.. _testing:

CAF Unit Testing
================

CAF includes a unit testing framework that we use to test CAF itself. It is also
available for users to test their own code.

The testing framework is integrated with CAF, which is also the reason why we
have decided to not use an existing framework. While there are many excellent
testing frameworks available, we needed a testing framework that integrates well
with CAF's inspection API, logging, and scheduling. Aside from the basic unit
testing features, ``caf_test`` also includes various fixtures that enable users
to test CAF code in a deterministic environment.

Concepts
--------

- A **check** represents a single verification of a boolean statement.
- A **section** contains any number of checks.
- A **test** contains any number of checks and/or sections.
- A **scenario** offers a BDD-style API to write tests.
- A **suite** groups tests together.
- A **fixture** equips a test with a fixed environment.

Checks
------

Checks are the basic building blocks of tests. They are used to verify
conditions and can be used in any context. The following checks are available:

- ``check(what)`` verifies that ``what`` is ``true``.
- ``check_eq(lhs, rhs)`` verifies that ``lhs == rhs``.
- ``check_ne(lhs, rhs)`` verifies that ``lhs != rhs``.
- ``check_lt(lhs, rhs)`` verifies that ``lhs < rhs``.
- ``check_le(lhs, rhs)`` verifies that ``lhs <= rhs``.
- ``check_gt(lhs, rhs)`` verifies that ``lhs > rhs``.
- ``check_ge(lhs, rhs)`` verifies that ``lhs >= rhs``.
- ``check_throws(fn)`` verifies that ``fn()`` throws an exception.
- ``check_throws<E>(fn)`` verifies that ``fn()`` throws an exception of type ``E``.

All checks return ``true`` if the condition is met and ``false`` otherwise. This
allows users to write checks in ``if`` statements. This is especially useful for
writing checks that depend on other checks. For example:

.. code-block:: C++

  if (check(!xs.empty())) {
    check_eq(xs.back(), 42);
  }

Approximate Comparisons
-----------------------

When working with floating point numbers, comparing for equality is generally
unsafe due to rounding errors and limited precision. To account for this, the
testing framework includes the template class ``caf::test::approx`` for
approximate comparisons. The class provides a constructor that takes a value and
when passing ``approx`` to ``check_eq`` or ``require_eq``, the comparison will
accept the value if it is within a certain range of the expected value.

By default, the allowed deviation is ``std::numeric_limits<T>::epsilon()``.
Users can change the deviation by using the ``epsilon`` member function. For
example:

.. code-block:: C++

  check_eq(my_fn(), approx{3.117}); // uses the default epsilon
  check_eq(my_fn(), approx{3.117}.epsilon(0.001)); // allows 3.116 <= x <= 3.118

Requirements
------------

Requirements are checks that will abort the current test if they fail.

- ``require(what)`` verifies that ``what`` is ``true``.
- ``require_eq(lhs, rhs)`` verifies that ``lhs == rhs``.
- ``require_ne(lhs, rhs)`` verifies that ``lhs != rhs``.
- ``require_lt(lhs, rhs)`` verifies that ``lhs < rhs``.
- ``require_le(lhs, rhs)`` verifies that ``lhs <= rhs``.
- ``require_gt(lhs, rhs)`` verifies that ``lhs > rhs``.
- ``require_ge(lhs, rhs)`` verifies that ``lhs >= rhs``.
- ``require_throws(fn)`` verifies that ``fn()`` throws an exception.
- ``require_throws<E>(fn)`` verifies that ``fn()`` throws an exception of type
  ``E``.

Tests
-----

A test is a runnable unit of code that verifies a specific behavior. Tests have
a description and can contain any number of checks and/or sections.

.. code-block:: C++

  TEST("push_back adds elements to the end of a vector") {
    std::vector<int> xs;
    xs.push_back(42);
    if (check_eq(xs.size(), 1u))
      check_eq(xs.back(), 42);
    xs.push_back(23);
    if (check_eq(xs.size(), 2u))
      check_eq(xs.back(), 23);
  }

Sections
--------

Sections allow users to split up tests into smaller units. Sections can contain
any number of checks and/or nested sections. Sections form a tree structure and
CAF will run *one branch* of the tree at a time. This means that one test can
have multiple execution paths and that neighboring sections do not interfere
with each other.

.. code-block:: C++

  TEST("pop_back and erase both remove elements from a vector") {
    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    SECTION("pop_back") {
      xs.pop_back();
      if (check_eq(xs.size(), 2u))
        check_eq(xs.back(), 2);
    }
    SECTION("erase") {
      xs.erase(xs.begin());
      if (check_eq(xs.size(), 2u)) {
        check_eq(xs.front(), 2);
        check_eq(xs.back(), 3);
      }
    }
  }

In the example above, CAF will first run the ``pop_back`` section and skip the
``erase`` section. After running the test once, CAF checks whether all branches
of the section tree have been executed. If not, CAF will run the test again and
skip all branches that have already been executed. This process repeats until
all branches have been executed. Hence, the example above will run the
``pop_back`` section first and the ``erase`` section second. Each section will
see the vector with all three elements.

Scenarios
---------

Scenarios offer a BDD-style API to write tests. A scenario may contain any
number of ``GIVEN`` or ``WHEN`` blocks. Each ``GIVEN`` block may contain any
number of ``WHEN`` blocks. Each ``WHEN`` block may contain any number of
``THEN`` blocks. The ``THEN`` block is the final nesting level.

There are also  ``AND_GIVEN``, ``AND_WHEN``, and ``AND_THEN`` blocks that can be
used to add to the previous block.

Just like sections, scenarios form a tree structure and CAF will run *one
branch* of the tree at a time. The `AND_*` blocks are executed in the same
context as the previous block.

.. code-block:: C++

  SCENARIO("vectors allow adding and removing elements") {
    GIVEN("a vector with two elements: [1, 2]") {
      std::vector xs{1, 2};
      WHEN("adding 3 to the vector") {
          xs.push_back(3);
      }
      AND_WHEN("the user erases the first element") {
        xs.erase(xs.begin());
        THEN("the vector has two elements") {
          check_eq(xs.size(), 2u);
        }
        AND_THEN("the last element is 3") {
          check_eq(xs.back(), 3);
        }
      }
      WHEN("the user calls front") {
        THEN("the result is 1") {
          check_eq(xs.front(), 1);
        }
      }
    }
    WHEN("default-constructing a vector") {
      std::vector<int> xs;
      THEN("the vector is empty") {
        check(xs.empty());
      }
    }
  }

Again, CAF will run the test branches independently. Hence, each ``WHEN`` block
will see the vector with its two initial elements. However, since ``AND_*``
blocks run after the previous block, the ``AND_WHEN`` block in the example above
will see the vector with three elements.

Scenario Outlines
-----------------

Scenario outlines allow users to run the same scenario with different inputs.
The ``GIVEN``, ``WHEN``, ``THEN``, etc. blocks inside of a scenario outline can
use Gherkin-style placeholders to refer to input values using the syntax
``<name>``. Further, an outline must contain an ``EXAMPLE`` block that defines
the input values in Markdown table notation.

To retrieve the values(s) for the placeholder(s) in a block, users can call
``block_parameters<...>()`` with ``...`` replaced by the desired type(s).

The following example generates two scenarios from the outline. The first
scenario will have ``start = 12``, ``eat = 5``, and ``left = 7``. The second
scenario will have ``start = 20``, ``eat = 5``, and ``left = 15``.

.. code-block:: C++

  OUTLINE("eating cucumbers") {
    GIVEN("there are <start> cucumbers") {
      auto start = block_parameters<int>();
      auto cucumbers = start;
      WHEN("I eat <eat> cucumbers") {
        auto eat = block_parameters<int>();
        cucumbers -= eat;
        THEN("I should have <left> cucumbers") {
          auto left = block_parameters<int>();
          check_eq(cucumbers, left);
        }
      }
    }
    EXAMPLES = R"(
      | start | eat | left |
      |    12 |   5 |    7 |
      |    20 |   5 |   15 |
    )";
  }

At runtime, CAF will replace all placeholders with their corresponding values
from the ``EXAMPLES`` block in the test output. Further, the individual scenario
runs are numbered in the test output. The example above would print ``Scenario:
eating cucumbers #1`` when using the values from the first row of the
``EXAMPLES`` block and ``Scenario: eating cucumbers #2`` when using the values
from the second row.

A block may have any number of placeholders. When calling ``block_parameters``,
the number of template parameters must match the number of placeholders in the
description. When passing more than one template parameter, the function returns
a ``std::tuple``:

.. code-block:: C++

  OUTLINE("adding two numbers") {
    GIVEN("the numbers <x> and <y>") {
      auto [x, y] = block_parameters<double, double>();
      WHEN("adding both numbers") {
        auto result = x + y;
        THEN("the result should be <sum>") {
          auto sum = block_parameters<double>();
          check_eq(result, sum);
        }
      }
    }
    EXAMPLES = R"(
      |   x |   y | sum |
      |   1 |   2 |   3 |
      | 2.5 | 3.5 |   6 |
    )";
  }

Suites
------

Suites allow users to optionally group tests together. Suites can contain any
number of tests and/or scenarios. However, a single translation unit may only
contain one suite.

When running a suite, CAF will run all tests and scenarios in the suite. Tests
that are not part of a suite will automatically be added to a default suite.

.. code-block:: C++

  SUITE("my-suite") {

  TEST("test 1") {
    // ...
  }

  TEST("test 2") {
    // ...
  }

  SCENARIO("scenario 1") {
    GIVEN("...") {
      WHEN("...") {
        THEN("...") {
          // ...
        }
      }
    }
  }

  } // SUITE("my-suite")

Fixtures
--------

Fixtures provide a way to share data as well as setup and teardown code between
tests. Each run of a test or scenario will create a new instance of the fixture
type. The fixture type must provide a default constructor.

.. code-block:: C++

  struct my_fixture {
    my_fixture() {
      xs.push_back(1);
      xs.push_back(2);
      xs.push_back(3);
    }

    std::vector<int> xs;
  };

  WITH_FIXTURE(my_fixture) {

  TEST("test 1") {
    require(xs.size(), 3u);
    // ...
  }

  TEST("test 2") {
    require(xs.size(), 3u);
    // ...
  }

  SCENARIO("scenario 1") {
    require(xs.size(), 3u);
    GIVEN("...") {
      WHEN("...") {
        THEN("...") {
          // ...
        }
      }
    }
  }

  } // WITH_FIXTURE(my_fixture)

Test Binaries
-------------

To build a test binary, simply add the following to your ``CMakeLists.txt``:

.. code-block:: CMake

  find_package(CAF COMPONENTS test REQUIRED)
  add_executable(my-test test.cpp)
  target_link_libraries(my-test PRIVATE CAF::test)

In ``test.cpp``, include ``caf/test/caf_test_main.hpp`` and use the macro
``CAF_TEST_MAIN`` to generate a ``main`` function for your test binary.

All tests and scenarios will be automatically registered with CAF. This also
works when splitting tests into multiple files. In this case, create an
executable with all test files and use the ``CAF_TEST_MAIN`` macro in exactly
one of them.

Like ``CAF_MAIN``, the ``CAF_TEST_MAIN`` macro accepts any number of type ID
blocks as well as CAF modules. For example, wen writing tests that use feature
from ``caf::net``, use ``CAF_TEST_MAIN(caf::net::middleman)`` to properly
initialize the meta object tables and networking.

Command Line Interface
----------------------

When using the ``CAF_TEST_MAIN`` macro, the resulting binary will provide a
command line interface to list and run tests. Running the binary without any
arguments will run all tests and scenarios in all suites. However, there are
several command line options to filter suites and tests using a regex, configure
the verbosity, etc. Please run the binary with ``--help`` to see all available
options.

Migrating from the Legacy Test Framework
========================================

The new testing framework has been added with CAF 0.19.3. Prior to that, CAF
shipped a legacy testing framework that is still available. However, we
recommend to no longer use the legacy framework for new projects and to migrate
to the new framework when still using the legacy framework, because we will
remove the legacy framework in a future release.

The legacy framework looked like this:

.. code-block:: C++

  #define CAF_SUITE core
  #include <caf/test/unit_test.hpp>

  CAF_TEST(multiply)
  {
    CAF_REQUIRE(0 * 1 == 0);
    CAF_CHECK(42 + 42 == 84);
  }

  struct fixture {
    fixture() {
      CAF_MESSAGE("entering test");
    }

    ~fixture() {
      CAF_MESSAGE("leaving test");
    }
  };

  CAF_TEST_FIXTURE_SCOPE(tracing_scope, fixture)

  CAF_TEST(divide)
  {
    CAF_FAIL(0 / 1 == 0);
    CAF_CHECK(1 / 1 == 0);  // fails
  }

  CAF_TEST_FIXTURE_SCOPE_END()

To migrate to the new framework:

- Include ``caf/test/test.hpp`` instead of ``caf/test/unit_test.hpp``.
- Replace ``CAF_TEST`` with ``TEST``.
- Instead of defining ``CAF_SUITE`` prior to including the header, use the
  ``SUITE`` macro.
- Instead of defining a fixture type with ``CAF_TEST_FIXTURE_SCOPE`` and
  ``CAF_TEST_FIXTURE_SCOPE_END``, use ``WITH_FIXTURE``.
- Instead of using ``CAF_REQUIRE``, ``CAF_CHECK``, etc. use ``require``,
  ``check``, etc. Note that the new framework uses actual C++ functions instead
  of macros for checks and requirements.

For users of the old ``caf/test/dsl.hpp`` header:

- Include ``caf/test/scenario.hpp`` instead of ``caf/test/dsl.hpp``.
- The BDD-style macros still have the same names. However, CAF now runs each
  branch of a BDD-style test independently. Hence, old tests with multiple
  ``WHEN`` blocks might behave slightly differently now.
- Instead of using the ``test_coordinator_fixture``, use
  ``caf::test::fixture::deterministic``. The interface is mostly the same, but
  there are some differences. Please refer to the class documentation for more
  details.

Macros such as ``expect``, ``allow`` and ``inject`` have been replaced by member
functions on the new deterministic fixture. Hence, the syntax has changed
slightly:

.. code-block:: none

  old: expect((int), with(42).from(foo).to(bar))
  new: expect<int>().with(42).from(foo).to(bar)

  old: allow((int), with(42).from(foo).to(bar))
  new: allow<int>().with(42).from(foo).to(bar)

  old: inject((int), with(42).from(foo).to(bar))
  new: inject().with(42).from(foo).to(bar)
