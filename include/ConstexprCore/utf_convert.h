#ifndef CONSTEXPRCORE_UTF_CONVERT_H
#define CONSTEXPRCORE_UTF_CONVERT_H

#include <ConstexprCore/fixed_string.h>

#include <cstddef>
#include <cstdint>

namespace ConstexprCore {

// ============================================================================
// Compile-time UTF conversion utilities
// ============================================================================

namespace detail {

// UTF-8 sequence length from lead byte
constexpr std::size_t utf8_seq_len(unsigned char lead) noexcept {
  if ((lead & 0x80) == 0) return 1;
  if ((lead & 0xE0) == 0xC0) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  return 4;
}

// UTF-16 encoded size for a code point
constexpr std::size_t utf16_encoded_size(char32_t cp) noexcept {
  return cp >= 0x10000 ? 2 : 1;
}

// Decode one UTF-8 code point, returns {code_point, bytes_consumed}
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

// Size calculation for fixed_string
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

// Encode a single code point to UTF-16, returns number of code units written
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

}  // namespace detail

// ============================================================================
// UTF-8 to UTF-16 conversion
// ============================================================================

/**
 * Convert UTF-8 string to UTF-16 at compile time.
 *
 * @tparam Str The UTF-8 string literal as fixed_string
 * @return A static_string containing the UTF-16 result
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

// ============================================================================
// UTF-8 to UTF-32 conversion
// ============================================================================

/**
 * Convert UTF-8 string to UTF-32 at compile time.
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

// ============================================================================
// Code point counting utilities
// ============================================================================

/**
 * Count the number of Unicode code points in a UTF-8 string.
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
 * Count the number of bytes needed to encode a UTF-8 string as UTF-16.
 */
template <fixed_string Str>
consteval std::size_t utf8_to_utf16_length() noexcept {
  return detail::utf8_to_utf16_size<Str>();
}

// ============================================================================
// Validation utilities
// ============================================================================

namespace detail {

constexpr bool is_valid_utf8_continuation(unsigned char c) noexcept {
  return (c & 0xC0) == 0x80;
}

}  // namespace detail

/**
 * Check if a string is valid UTF-8 at compile time.
 *
 * Example:
 *   static_assert(is_valid_utf8<"Hello">());
 *   static_assert(is_valid_utf8<"\xE2\x82\xAC">());  // Valid Euro sign
 */
template <fixed_string Str>
consteval bool is_valid_utf8() noexcept {
  std::size_t i = 0;
  while (i < Str.size()) {
    unsigned char lead = static_cast<unsigned char>(Str.data[i]);

    if ((lead & 0x80) == 0) {
      // ASCII
      i += 1;
    } else if ((lead & 0xE0) == 0xC0) {
      // 2-byte sequence
      if (i + 1 >= Str.size()) return false;
      if (!detail::is_valid_utf8_continuation(static_cast<unsigned char>(Str.data[i + 1]))) return false;
      // Check for overlong encoding
      if (lead < 0xC2) return false;
      i += 2;
    } else if ((lead & 0xF0) == 0xE0) {
      // 3-byte sequence
      if (i + 2 >= Str.size()) return false;
      if (!detail::is_valid_utf8_continuation(static_cast<unsigned char>(Str.data[i + 1]))) return false;
      if (!detail::is_valid_utf8_continuation(static_cast<unsigned char>(Str.data[i + 2]))) return false;
      // Check for overlong encoding and surrogate range
      char32_t cp = ((lead & 0x0F) << 12) |
                    ((static_cast<unsigned char>(Str.data[i + 1]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(Str.data[i + 2]) & 0x3F);
      if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
      i += 3;
    } else if ((lead & 0xF8) == 0xF0) {
      // 4-byte sequence
      if (i + 3 >= Str.size()) return false;
      if (!detail::is_valid_utf8_continuation(static_cast<unsigned char>(Str.data[i + 1]))) return false;
      if (!detail::is_valid_utf8_continuation(static_cast<unsigned char>(Str.data[i + 2]))) return false;
      if (!detail::is_valid_utf8_continuation(static_cast<unsigned char>(Str.data[i + 3]))) return false;
      // Check for overlong encoding and valid range
      char32_t cp = ((lead & 0x07) << 18) |
                    ((static_cast<unsigned char>(Str.data[i + 1]) & 0x3F) << 12) |
                    ((static_cast<unsigned char>(Str.data[i + 2]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(Str.data[i + 3]) & 0x3F);
      if (cp < 0x10000 || cp > 0x10FFFF) return false;
      i += 4;
    } else {
      // Invalid lead byte
      return false;
    }
  }
  return true;
}

}  // namespace ConstexprCore

#endif  // CONSTEXPRCORE_UTF_CONVERT_H
