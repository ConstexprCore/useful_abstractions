#ifndef CONSTEXPRCORE_CONSTEXPR_CHECK_H
#define CONSTEXPRCORE_CONSTEXPR_CHECK_H

#include <cstddef>

namespace ConstexprCore {

// ============================================================================
// Compile-time error utilities
// ============================================================================

/**
 * Triggers a compile-time error with a descriptive message.
 *
 * In a consteval context, throwing causes a compile-time error. The message
 * becomes part of the compiler's error output (compiler-dependent formatting).
 *
 * Example:
 *   consteval int must_be_positive(int x) {
 *     if (x <= 0) static_error("Value must be positive");
 *     return x;
 *   }
 *
 *   constexpr int a = must_be_positive(5);   // OK
 *   constexpr int b = must_be_positive(-1);  // Compile error!
 */
[[noreturn]] consteval void static_error(const char* message) {
  throw message;
}

/**
 * Compile-time assertion that can use runtime-computed conditions.
 *
 * Unlike static_assert, this works inside consteval functions with
 * conditions that depend on template parameters or consteval computation.
 *
 * Example:
 *   template <int N>
 *   consteval int factorial() {
 *     static_require(N >= 0, "factorial requires non-negative input");
 *     if constexpr (N <= 1) return 1;
 *     else return N * factorial<N - 1>();
 *   }
 */
consteval void static_require(bool condition, const char* message) {
  if (!condition) {
    throw message;
  }
}

// ============================================================================
// Error result types for detailed error reporting
// ============================================================================

/**
 * A compile-time error result that carries position information.
 *
 * Use this when you need to report where an error occurred in input data.
 */
template <typename ErrorEnum>
struct error_result {
  ErrorEnum error;
  std::size_t position;

  [[nodiscard]] constexpr bool ok() const noexcept {
    return error == ErrorEnum{};  // Assumes default value means "no error"
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return ok();
  }
};

// ============================================================================
// Require helper for error_result
// ============================================================================

/**
 * Throws a compile-time error if the result indicates failure.
 *
 * The message parameter should describe what was being validated.
 * For more specific error messages, handle error codes individually.
 */
template <typename ErrorEnum>
consteval void require_ok(const error_result<ErrorEnum>& result,
                          const char* message) {
  if (!result.ok()) {
    throw message;
  }
}

}  // namespace ConstexprCore

#endif  // CONSTEXPRCORE_CONSTEXPR_CHECK_H
