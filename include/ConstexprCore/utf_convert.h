#ifndef CONSTEXPRCORE_UTF_CONVERT_H
#define CONSTEXPRCORE_UTF_CONVERT_H

#include <constexprcore/constexpr_check.h>
#include <constexprcore/fixed_string.h>

#include <cstddef>
#include <cstdint>

namespace ConstexprCore {

// ============================================================================
// UTF-8 validation error types
// ============================================================================

/**
 * Specific UTF-8 validation error codes.
 *
 * These provide detailed information about why validation failed.
 */
enum class utf8_error {
  none = 0,                    // No error (valid UTF-8)
  truncated_sequence,          // Multi-byte sequence cut off before end
  invalid_continuation_byte,   // Expected continuation byte (10xxxxxx), got something else
  overlong_encoding,           // Code point encoded with more bytes than necessary
  surrogate_code_point,        // UTF-16 surrogate (U+D800-U+DFFF) encoded in UTF-8
  invalid_code_point,          // Code point > U+10FFFF
  invalid_lead_byte            // Byte doesn't match any valid UTF-8 lead pattern
};

/**
 * UTF-8 validation result with error details.
 */
using utf8_validation_result = error_result<utf8_error>;

// ============================================================================
// Compile-time UTF conversion utilities (internal)
// ============================================================================

namespace detail {

/**
 * UTF-8 sequence length from lead byte.
 *
 * @note Does NOT validate. Returns 4 for any byte >= 0xF0, even invalid ones.
 *       Use validate_utf8<>() for validation.
 */
constexpr std::size_t utf8_seq_len(unsigned char lead) noexcept {
  if ((lead & 0x80) == 0) return 1;
  if ((lead & 0xE0) == 0xC0) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  return 4;
}

/**
 * UTF-16 encoded size for a code point.
 *
 * @note Does NOT validate. Assumes cp is a valid Unicode code point.
 */
constexpr std::size_t utf16_encoded_size(char32_t cp) noexcept {
  return cp >= 0x10000 ? 2 : 1;
}

/**
 * Decode one UTF-8 code point, returns {code_point, bytes_consumed}.
 *
 * @note Does NOT validate. Assumes the input is well-formed UTF-8.
 *       For untrusted input, use validate_utf8<>() first.
 */
constexpr std::pair<char32_t, std::size_t> decode_utf8(const char* str) noexcept {
  unsigned char lead = static_cast<unsigned char>(*str);

  if ((lead & 0x80) == 0) {
    return {lead, 1};
  }
  if ((lead & 0xE0) == 0xC0) {
    char32_t cp = (lead & 0x1F) << 6;
    cp |= (static_cast<unsigned char>(str[1]) & 0x3F);
    return {cp, 2};
  }
  if ((lead & 0xF0) == 0xE0) {
    char32_t cp = (lead & 0x0F) << 12;
    cp |= (static_cast<unsigned char>(str[1]) & 0x3F) << 6;
    cp |= (static_cast<unsigned char>(str[2]) & 0x3F);
    return {cp, 3};
  }
  char32_t cp = (lead & 0x07) << 18;
  cp |= (static_cast<unsigned char>(str[1]) & 0x3F) << 12;
  cp |= (static_cast<unsigned char>(str[2]) & 0x3F) << 6;
  cp |= (static_cast<unsigned char>(str[3]) & 0x3F);
  return {cp, 4};
}

// Size calculation for fixed_string (assumes valid UTF-8)
template <fixed_string Str>
consteval std::size_t utf8_to_utf16_size() noexcept {
  std::size_t result = 0;
  std::size_t i = 0;
  while (i < Str.size()) {
    auto [cp, consumed] = decode_utf8(&Str.data[i]);
    result += utf16_encoded_size(cp);
    i += consumed;
  }
  return result;
}

template <fixed_string Str>
consteval std::size_t utf8_to_utf32_size() noexcept {
  std::size_t result = 0;
  std::size_t i = 0;
  while (i < Str.size()) {
    i += utf8_seq_len(static_cast<unsigned char>(Str.data[i]));
    ++result;
  }
  return result;
}

/**
 * Encode a single code point to UTF-16, returns number of code units written.
 *
 * @note Does NOT validate. Assumes cp is a valid Unicode code point (not a surrogate).
 */
constexpr std::size_t encode_utf16(char32_t cp, char16_t* out) noexcept {
  if (cp < 0x10000) {
    out[0] = static_cast<char16_t>(cp);
    return 1;
  }
  char32_t adjusted = cp - 0x10000;
  out[0] = static_cast<char16_t>(0xD800 | (adjusted >> 10));
  out[1] = static_cast<char16_t>(0xDC00 | (adjusted & 0x3FF));
  return 2;
}

constexpr bool is_valid_utf8_continuation(unsigned char c) noexcept {
  return (c & 0xC0) == 0x80;
}

}  // namespace detail

// ============================================================================
// UTF-8 validation
// ============================================================================

/**
 * Validate UTF-8 string and return detailed error information.
 *
 * Returns a utf8_validation_result containing:
 * - error: The specific error code (utf8_error::none if valid)
 * - position: Byte offset where the error was detected
 *
 * Example:
 *   constexpr auto result = validate_utf8<"Hello">();
 *   static_assert(result.ok());
 *
 *   constexpr auto bad = validate_utf8<"\xFF">();
 *   static_assert(bad.error == utf8_error::invalid_lead_byte);
 *   static_assert(bad.position == 0);
 */
template <fixed_string Str>
consteval utf8_validation_result validate_utf8() noexcept {
  std::size_t i = 0;
  while (i < Str.size()) {
    unsigned char lead = static_cast<unsigned char>(Str.data[i]);

    if ((lead & 0x80) == 0) {
      // ASCII - always valid
      i += 1;
    } else if ((lead & 0xE0) == 0xC0) {
      // 2-byte sequence
      if (i + 1 >= Str.size()) {
        return {utf8_error::truncated_sequence, i};
      }
      if (!detail::is_valid_utf8_continuation(
              static_cast<unsigned char>(Str.data[i + 1]))) {
        return {utf8_error::invalid_continuation_byte, i + 1};
      }
      // Check for overlong encoding (code points < 0x80 must use 1 byte)
      if (lead < 0xC2) {
        return {utf8_error::overlong_encoding, i};
      }
      i += 2;
    } else if ((lead & 0xF0) == 0xE0) {
      // 3-byte sequence
      if (i + 2 >= Str.size()) {
        return {utf8_error::truncated_sequence, i};
      }
      if (!detail::is_valid_utf8_continuation(
              static_cast<unsigned char>(Str.data[i + 1]))) {
        return {utf8_error::invalid_continuation_byte, i + 1};
      }
      if (!detail::is_valid_utf8_continuation(
              static_cast<unsigned char>(Str.data[i + 2]))) {
        return {utf8_error::invalid_continuation_byte, i + 2};
      }
      // Decode and check for overlong encoding and surrogate range
      char32_t cp =
          ((lead & 0x0F) << 12) |
          ((static_cast<unsigned char>(Str.data[i + 1]) & 0x3F) << 6) |
          (static_cast<unsigned char>(Str.data[i + 2]) & 0x3F);
      if (cp < 0x800) {
        return {utf8_error::overlong_encoding, i};
      }
      if (cp >= 0xD800 && cp <= 0xDFFF) {
        return {utf8_error::surrogate_code_point, i};
      }
      i += 3;
    } else if ((lead & 0xF8) == 0xF0) {
      // 4-byte sequence
      if (i + 3 >= Str.size()) {
        return {utf8_error::truncated_sequence, i};
      }
      if (!detail::is_valid_utf8_continuation(
              static_cast<unsigned char>(Str.data[i + 1]))) {
        return {utf8_error::invalid_continuation_byte, i + 1};
      }
      if (!detail::is_valid_utf8_continuation(
              static_cast<unsigned char>(Str.data[i + 2]))) {
        return {utf8_error::invalid_continuation_byte, i + 2};
      }
      if (!detail::is_valid_utf8_continuation(
              static_cast<unsigned char>(Str.data[i + 3]))) {
        return {utf8_error::invalid_continuation_byte, i + 3};
      }
      // Decode and check range
      char32_t cp =
          ((lead & 0x07) << 18) |
          ((static_cast<unsigned char>(Str.data[i + 1]) & 0x3F) << 12) |
          ((static_cast<unsigned char>(Str.data[i + 2]) & 0x3F) << 6) |
          (static_cast<unsigned char>(Str.data[i + 3]) & 0x3F);
      if (cp < 0x10000) {
        return {utf8_error::overlong_encoding, i};
      }
      if (cp > 0x10FFFF) {
        return {utf8_error::invalid_code_point, i};
      }
      i += 4;
    } else {
      // Invalid lead byte (0x80-0xBF are continuation bytes, 0xF8+ invalid)
      return {utf8_error::invalid_lead_byte, i};
    }
  }
  return {utf8_error::none, 0};
}

/**
 * Check if a string is valid UTF-8 at compile time.
 *
 * Simple boolean check. For detailed error information, use validate_utf8<>().
 *
 * Example:
 *   static_assert(is_valid_utf8<"Hello">());
 *   static_assert(is_valid_utf8<"\xE2\x82\xAC">());  // Valid Euro sign
 */
template <fixed_string Str>
consteval bool is_valid_utf8() noexcept {
  return validate_utf8<Str>().ok();
}

/**
 * Require valid UTF-8, triggering a compile-time error if invalid.
 *
 * Use this when you want a clear compile-time error for invalid input.
 *
 * Example:
 *   template <fixed_string Str>
 *   consteval auto process_utf8() {
 *     require_valid_utf8<Str>();  // Compile error if invalid
 *     return utf8_to_utf32<Str>();
 *   }
 */
template <fixed_string Str>
consteval void require_valid_utf8() {
  constexpr auto result = validate_utf8<Str>();
  if constexpr (!result.ok()) {
    // Provide specific error messages based on error type
    if constexpr (result.error == utf8_error::truncated_sequence) {
      static_error("Invalid UTF-8: truncated multi-byte sequence");
    } else if constexpr (result.error == utf8_error::invalid_continuation_byte) {
      static_error("Invalid UTF-8: expected continuation byte");
    } else if constexpr (result.error == utf8_error::overlong_encoding) {
      static_error("Invalid UTF-8: overlong encoding");
    } else if constexpr (result.error == utf8_error::surrogate_code_point) {
      static_error("Invalid UTF-8: surrogate code point (U+D800-U+DFFF)");
    } else if constexpr (result.error == utf8_error::invalid_code_point) {
      static_error("Invalid UTF-8: code point exceeds U+10FFFF");
    } else if constexpr (result.error == utf8_error::invalid_lead_byte) {
      static_error("Invalid UTF-8: invalid lead byte");
    } else {
      static_error("Invalid UTF-8");
    }
  }
}

// ============================================================================
// UTF-8 to UTF-16 conversion
// ============================================================================

/**
 * Convert UTF-8 string to UTF-16 at compile time.
 *
 * @note Does NOT validate input. For untrusted input, call require_valid_utf8<Str>()
 *       first, or use is_valid_utf8<Str>() to check before conversion.
 *
 * @tparam Str The UTF-8 string literal as fixed_string
 * @return A fixed_string<char16_t, N> containing the UTF-16 result
 *
 * Example:
 *   constexpr auto utf16 = utf8_to_utf16<"Hello">();
 *   constexpr auto utf16_euro = utf8_to_utf16<"Price: \xE2\x82\xAC">();
 */
template <fixed_string Str>
consteval auto utf8_to_utf16() {
  constexpr std::size_t output_len = detail::utf8_to_utf16_size<Str>();

  fixed_string<char16_t, output_len> result{};

  std::size_t in_idx = 0;
  std::size_t out_idx = 0;

  while (in_idx < Str.size()) {
    auto [cp, consumed] = detail::decode_utf8(&Str.data[in_idx]);
    in_idx += consumed;
    out_idx += detail::encode_utf16(cp, &result.data[out_idx]);
  }

  return result;
}

/**
 * Convert UTF-8 to UTF-16 with validation.
 *
 * Triggers a compile-time error if the input is not valid UTF-8.
 *
 * Example:
 *   constexpr auto utf16 = utf8_to_utf16_checked<"Hello">();  // OK
 *   constexpr auto bad = utf8_to_utf16_checked<"\xFF">();     // Compile error!
 */
template <fixed_string Str>
consteval auto utf8_to_utf16_checked() {
  require_valid_utf8<Str>();
  return utf8_to_utf16<Str>();
}

// ============================================================================
// UTF-8 to UTF-32 conversion
// ============================================================================

/**
 * Convert UTF-8 string to UTF-32 at compile time.
 *
 * @note Does NOT validate input. For untrusted input, call require_valid_utf8<Str>()
 *       first, or use is_valid_utf8<Str>() to check before conversion.
 *
 * Example:
 *   constexpr auto utf32 = utf8_to_utf32<"Hello">();
 */
template <fixed_string Str>
consteval auto utf8_to_utf32() {
  constexpr std::size_t output_len = detail::utf8_to_utf32_size<Str>();

  fixed_string<char32_t, output_len> result{};

  std::size_t in_idx = 0;
  std::size_t out_idx = 0;

  while (in_idx < Str.size()) {
    auto [cp, consumed] = detail::decode_utf8(&Str.data[in_idx]);
    result.data[out_idx++] = cp;
    in_idx += consumed;
  }

  return result;
}

/**
 * Convert UTF-8 to UTF-32 with validation.
 *
 * Triggers a compile-time error if the input is not valid UTF-8.
 *
 * Example:
 *   constexpr auto utf32 = utf8_to_utf32_checked<"Hello">();  // OK
 *   constexpr auto bad = utf8_to_utf32_checked<"\xFF">();     // Compile error!
 */
template <fixed_string Str>
consteval auto utf8_to_utf32_checked() {
  require_valid_utf8<Str>();
  return utf8_to_utf32<Str>();
}

// ============================================================================
// Code point counting utilities
// ============================================================================

/**
 * Count the number of Unicode code points in a UTF-8 string.
 *
 * @note Does NOT validate. For invalid UTF-8, the count may be incorrect.
 *
 * Example:
 *   static_assert(utf8_code_point_count<"Hello">() == 5);
 *   static_assert(utf8_code_point_count<"\xE2\x82\xAC">() == 1);  // Euro sign
 */
template <fixed_string Str>
consteval std::size_t utf8_code_point_count() noexcept {
  return detail::utf8_to_utf32_size<Str>();
}

/**
 * Count the number of UTF-16 code units needed to encode a UTF-8 string.
 *
 * @note Does NOT validate. For invalid UTF-8, the count may be incorrect.
 */
template <fixed_string Str>
consteval std::size_t utf8_to_utf16_length() noexcept {
  return detail::utf8_to_utf16_size<Str>();
}

}  // namespace ConstexprCore

#endif  // CONSTEXPRCORE_UTF_CONVERT_H
