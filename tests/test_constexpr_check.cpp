#include <doctest/doctest.h>
#include <ConstexprCore/constexpr_check.h>

using namespace ConstexprCore;

// ============================================================================
// static_error tests
// ============================================================================

TEST_SUITE("static_error") {
  // Note: We can't easily test that static_error causes a compile-time error
  // in a runtime test. These tests verify the basic structure works.

  TEST_CASE("static_require passes for true condition") {
    // This should compile and run without issues
    constexpr auto test = []() consteval {
      static_require(true, "This should not trigger");
      return 42;
    };

    static_assert(test() == 42);
    CHECK(test() == 42);
  }

  // Uncomment to verify compile-time error (will fail compilation):
  // TEST_CASE("static_require fails for false condition") {
  //   constexpr auto test = []() consteval {
  //     static_require(false, "This SHOULD trigger");
  //     return 42;
  //   };
  //   static_assert(test() == 42);  // Compile error here
  // }
}

// ============================================================================
// error_result tests
// ============================================================================

TEST_SUITE("error_result") {
  enum class test_error { none = 0, error_a, error_b };

  TEST_CASE("ok() returns true for no error") {
    constexpr error_result<test_error> result{test_error::none, 0};

    static_assert(result.ok());
    static_assert(result);  // bool conversion

    CHECK(result.ok());
    CHECK(result);
  }

  TEST_CASE("ok() returns false for error") {
    constexpr error_result<test_error> result{test_error::error_a, 5};

    static_assert(!result.ok());
    static_assert(!result);  // bool conversion
    static_assert(result.error == test_error::error_a);
    static_assert(result.position == 5);

    CHECK(!result.ok());
    CHECK(!result);
    CHECK(result.error == test_error::error_a);
    CHECK(result.position == 5);
  }

  TEST_CASE("require_ok passes for ok result") {
    constexpr auto test = []() consteval {
      error_result<test_error> result{test_error::none, 0};
      require_ok(result, "This should not trigger");
      return 42;
    };

    static_assert(test() == 42);
    CHECK(test() == 42);
  }

  // Uncomment to verify compile-time error:
  // TEST_CASE("require_ok fails for error result") {
  //   constexpr auto test = []() consteval {
  //     error_result<test_error> result{test_error::error_a, 0};
  //     require_ok(result, "This SHOULD trigger");
  //     return 42;
  //   };
  //   static_assert(test() == 42);  // Compile error here
  // }
}

// ============================================================================
// Practical usage tests
// ============================================================================

TEST_SUITE("practical usage") {
  TEST_CASE("consteval function with validation") {
    // Example: A function that requires positive input
    constexpr auto safe_sqrt_approx = [](int x) consteval {
      static_require(x >= 0, "safe_sqrt_approx requires non-negative input");
      // Simple approximation for testing
      if (x == 0) return 0;
      int result = x;
      for (int i = 0; i < 10; ++i) {
        result = (result + x / result) / 2;
      }
      return result;
    };

    static_assert(safe_sqrt_approx(0) == 0);
    static_assert(safe_sqrt_approx(4) == 2);
    static_assert(safe_sqrt_approx(100) == 10);

    CHECK(safe_sqrt_approx(0) == 0);
    CHECK(safe_sqrt_approx(4) == 2);
    CHECK(safe_sqrt_approx(100) == 10);
  }

  TEST_CASE("error_result for detailed validation") {
    enum class range_error { none = 0, too_small, too_large };

    constexpr auto validate_range = [](int x, int min, int max) consteval {
      if (x < min) return error_result<range_error>{range_error::too_small, 0};
      if (x > max) return error_result<range_error>{range_error::too_large, 0};
      return error_result<range_error>{range_error::none, 0};
    };

    constexpr auto r1 = validate_range(5, 0, 10);
    static_assert(r1.ok());

    constexpr auto r2 = validate_range(-1, 0, 10);
    static_assert(!r2.ok());
    static_assert(r2.error == range_error::too_small);

    constexpr auto r3 = validate_range(15, 0, 10);
    static_assert(!r3.ok());
    static_assert(r3.error == range_error::too_large);

    CHECK(r1.ok());
    CHECK(!r2.ok());
    CHECK(!r3.ok());
  }
}
