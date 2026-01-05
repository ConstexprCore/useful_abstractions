#ifndef UA_CHAR_UTILS_H
#define UA_CHAR_UTILS_H

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ua {

// ============================================================================
// Internal helpers
// ============================================================================

namespace detail {

// Helper to silence warnings about intentional bitwise operations on booleans.
// Branch-free code intentionally uses & and | instead of && and ||.
[[nodiscard]] constexpr int to_int(bool b) noexcept {
  return static_cast<int>(b);
}

}  // namespace detail

// ============================================================================
// ASCII Character Classification (Fast, Branch-Free)
// ============================================================================

/**
 * Convert ASCII character to lowercase using bitwise OR.
 *
 * Only valid for ASCII letters (A-Z, a-z). For non-letters,
 * the result is not meaningful but the function is still safe.
 *
 * Cost: Single bitwise OR operation (no branching).
 */
[[nodiscard]] constexpr char to_lower(char c) noexcept {
  return static_cast<char>(static_cast<unsigned char>(c) | 0x20u);
}

/**
 * Convert ASCII character to uppercase using bitwise AND.
 *
 * Only valid for ASCII letters (A-Z, a-z). For non-letters,
 * the result is not meaningful but the function is still safe.
 *
 * Cost: Single bitwise AND operation (no branching).
 */
[[nodiscard]] constexpr char to_upper(char c) noexcept {
  return static_cast<char>(static_cast<unsigned char>(c) & ~0x20u);
}

/**
 * Check if character is an ASCII digit (0-9).
 *
 * Cost: Two comparisons combined with bitwise AND (branch-free).
 */
[[nodiscard]] constexpr bool is_digit(char c) noexcept {
  return (c >= '0') & (c <= '9');
}

/**
 * Check if character is an ASCII hexadecimal digit (0-9, a-f, A-F).
 *
 * Cost: Three range checks combined with bitwise OR (branch-free).
 */
[[nodiscard]] constexpr bool is_hex_digit(char c) noexcept {
  using detail::to_int;
  return to_int(is_digit(c)) | (to_int(to_lower(c) >= 'a') & to_int(to_lower(c) <= 'f'));
}

/**
 * Check if character is an ASCII letter (a-z, A-Z).
 *
 * Cost: Single lowercase conversion + two comparisons (branch-free).
 */
[[nodiscard]] constexpr bool is_alpha(char c) noexcept {
  const char lower = to_lower(c);
  return (lower >= 'a') & (lower <= 'z');
}

/**
 * Check if character is alphanumeric (a-z, A-Z, 0-9).
 *
 * Cost: Combines is_alpha and is_digit (branch-free).
 */
[[nodiscard]] constexpr bool is_alnum(char c) noexcept {
  using detail::to_int;
  return to_int(is_alpha(c)) | to_int(is_digit(c));
}

/**
 * Check if character is ASCII whitespace (space, tab, newline, etc.).
 *
 * Matches: ' ' (0x20), '\t' (0x09), '\n' (0x0A), '\v' (0x0B),
 *          '\f' (0x0C), '\r' (0x0D)
 */
[[nodiscard]] constexpr bool is_whitespace(char c) noexcept {
  return (c == ' ') | (c == '\t') | (c == '\n') | (c == '\v') | (c == '\f') |
         (c == '\r');
}

/**
 * Check if character is printable ASCII (0x20-0x7E).
 */
[[nodiscard]] constexpr bool is_printable(char c) noexcept {
  const auto u = static_cast<unsigned char>(c);
  return (u >= 0x20) & (u <= 0x7E);
}

/**
 * Check if character is an ASCII control character (0x00-0x1F, 0x7F).
 */
[[nodiscard]] constexpr bool is_control(char c) noexcept {
  const auto u = static_cast<unsigned char>(c);
  return (u < 0x20) | (u == 0x7F);
}

/**
 * Check if character is ASCII (0x00-0x7F).
 */
[[nodiscard]] constexpr bool is_ascii(char c) noexcept {
  return static_cast<unsigned char>(c) <= 0x7F;
}

/**
 * Check if byte is a UTF-8 continuation byte (10xxxxxx).
 */
[[nodiscard]] constexpr bool is_utf8_continuation(char c) noexcept {
  return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

/**
 * Check if byte is a UTF-8 leading byte (not ASCII, not continuation).
 */
[[nodiscard]] constexpr bool is_utf8_leading(char c) noexcept {
  const auto u = static_cast<unsigned char>(c);
  return (u >= 0xC0) & (u <= 0xF7);
}

// ============================================================================
// Numeric Value Extraction
// ============================================================================

/**
 * Convert digit character to its numeric value.
 *
 * @return 0-9 for valid digits, undefined for non-digits
 */
[[nodiscard]] constexpr int digit_value(char c) noexcept {
  return static_cast<int>(c - '0');
}

/**
 * Convert hexadecimal digit to its numeric value.
 *
 * @return 0-15 for valid hex digits, undefined for non-hex
 */
[[nodiscard]] constexpr int hex_value(char c) noexcept {
  if (is_digit(c)) {
    return c - '0';
  }
  return (to_lower(c) - 'a') + 10;
}

/**
 * Convert a value 0-15 to its hex character representation (lowercase).
 */
[[nodiscard]] constexpr char to_hex_lower(int value) noexcept {
  return (value < 10) ? static_cast<char>('0' + value)
                      : static_cast<char>('a' + value - 10);
}

/**
 * Convert a value 0-15 to its hex character representation (uppercase).
 */
[[nodiscard]] constexpr char to_hex_upper(int value) noexcept {
  return (value < 10) ? static_cast<char>('0' + value)
                      : static_cast<char>('A' + value - 10);
}

// ============================================================================
// Prefix Detection (Optimized for Short Patterns)
// ============================================================================

/**
 * Check if string starts with "0x" or "0X" (hex prefix).
 *
 * Uses endian-aware word comparison for efficiency.
 *
 * @param s Input string (must have at least 2 characters)
 * @return true if starts with hex prefix
 */
[[nodiscard]] constexpr bool has_hex_prefix(std::string_view s) noexcept {
  if (s.size() < 2) {
    return false;
  }

  // Check '0' followed by 'x' or 'X'
  using detail::to_int;
  return to_int(s[0] == '0') & (to_int(s[1] == 'x') | to_int(s[1] == 'X'));
}

/**
 * Optimized hex prefix check using word comparison (unsafe version).
 *
 * @pre s.size() >= 2 (caller must ensure)
 */
[[nodiscard]] constexpr bool has_hex_prefix_unsafe(std::string_view s) noexcept {
  // Load two bytes as uint16_t
  constexpr bool is_little_endian = std::endian::native == std::endian::little;
  constexpr uint16_t target_0x = is_little_endian ? 0x7830 : 0x3078;  // "0x"

  uint16_t two_bytes = static_cast<uint16_t>(static_cast<uint8_t>(s[0])) |
                       static_cast<uint16_t>(static_cast<uint16_t>(
                           static_cast<uint8_t>(s[1])) << 8);

  // Apply mask to make 'x' case-insensitive (0x20 bit difference)
  if constexpr (is_little_endian) {
    two_bytes |= 0x2000;
  } else {
    two_bytes |= 0x0020;
  }

  return two_bytes == target_0x;
}

/**
 * Check if string starts with "0b" or "0B" (binary prefix).
 */
[[nodiscard]] constexpr bool has_bin_prefix(std::string_view s) noexcept {
  if (s.size() < 2) {
    return false;
  }
  using detail::to_int;
  return to_int(s[0] == '0') & (to_int(s[1] == 'b') | to_int(s[1] == 'B'));
}

/**
 * Check if string starts with "0o" or "0O" (octal prefix).
 */
[[nodiscard]] constexpr bool has_oct_prefix(std::string_view s) noexcept {
  if (s.size() < 2) {
    return false;
  }
  using detail::to_int;
  return to_int(s[0] == '0') & (to_int(s[1] == 'o') | to_int(s[1] == 'O'));
}

// ============================================================================
// Path/URL Character Utilities
// ============================================================================

/**
 * Check if string looks like a Windows drive letter (C:, D:, etc.).
 *
 * Matches: letter followed by ':' or '|'
 */
[[nodiscard]] constexpr bool
is_windows_drive_letter(std::string_view s) noexcept {
  if (s.size() < 2) {
    return false;
  }
  using detail::to_int;
  return to_int(is_alpha(s[0])) & (to_int(s[1] == ':') | to_int(s[1] == '|'));
}

/**
 * Check if string is a normalized Windows drive letter (C:, not C|).
 */
[[nodiscard]] constexpr bool
is_normalized_windows_drive(std::string_view s) noexcept {
  if (s.size() < 2) {
    return false;
  }
  using detail::to_int;
  return to_int(is_alpha(s[0])) & to_int(s[1] == ':');
}

// ============================================================================
// Case-Insensitive Comparison Utilities
// ============================================================================

/**
 * Case-insensitive character comparison (ASCII only).
 */
[[nodiscard]] constexpr bool iequal_char(char a, char b) noexcept {
  return to_lower(a) == to_lower(b);
}

/**
 * Case-insensitive string comparison (ASCII only).
 */
[[nodiscard]] constexpr bool iequal(std::string_view a,
                                    std::string_view b) noexcept {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (!iequal_char(a[i], b[i])) {
      return false;
    }
  }
  return true;
}

/**
 * Case-insensitive prefix check (ASCII only).
 */
[[nodiscard]] constexpr bool istarts_with(std::string_view s,
                                          std::string_view prefix) noexcept {
  if (s.size() < prefix.size()) {
    return false;
  }
  return iequal(s.substr(0, prefix.size()), prefix);
}

/**
 * Case-insensitive suffix check (ASCII only).
 */
[[nodiscard]] constexpr bool iends_with(std::string_view s,
                                        std::string_view suffix) noexcept {
  if (s.size() < suffix.size()) {
    return false;
  }
  return iequal(s.substr(s.size() - suffix.size()), suffix);
}

// ============================================================================
// Lookup Tables for Percent-Encoding (URL)
// ============================================================================

namespace detail {

/**
 * Pre-computed percent-encoded hex strings for bytes 0x00-0xFF.
 *
 * Each entry is a 4-character string: "%XX\0"
 * Access: hex_encode_table[byte * 4] gives pointer to "%XX"
 */
inline constexpr char hex_encode_table[] =
    "%00\0%01\0%02\0%03\0%04\0%05\0%06\0%07\0"
    "%08\0%09\0%0A\0%0B\0%0C\0%0D\0%0E\0%0F\0"
    "%10\0%11\0%12\0%13\0%14\0%15\0%16\0%17\0"
    "%18\0%19\0%1A\0%1B\0%1C\0%1D\0%1E\0%1F\0"
    "%20\0%21\0%22\0%23\0%24\0%25\0%26\0%27\0"
    "%28\0%29\0%2A\0%2B\0%2C\0%2D\0%2E\0%2F\0"
    "%30\0%31\0%32\0%33\0%34\0%35\0%36\0%37\0"
    "%38\0%39\0%3A\0%3B\0%3C\0%3D\0%3E\0%3F\0"
    "%40\0%41\0%42\0%43\0%44\0%45\0%46\0%47\0"
    "%48\0%49\0%4A\0%4B\0%4C\0%4D\0%4E\0%4F\0"
    "%50\0%51\0%52\0%53\0%54\0%55\0%56\0%57\0"
    "%58\0%59\0%5A\0%5B\0%5C\0%5D\0%5E\0%5F\0"
    "%60\0%61\0%62\0%63\0%64\0%65\0%66\0%67\0"
    "%68\0%69\0%6A\0%6B\0%6C\0%6D\0%6E\0%6F\0"
    "%70\0%71\0%72\0%73\0%74\0%75\0%76\0%77\0"
    "%78\0%79\0%7A\0%7B\0%7C\0%7D\0%7E\0%7F\0"
    "%80\0%81\0%82\0%83\0%84\0%85\0%86\0%87\0"
    "%88\0%89\0%8A\0%8B\0%8C\0%8D\0%8E\0%8F\0"
    "%90\0%91\0%92\0%93\0%94\0%95\0%96\0%97\0"
    "%98\0%99\0%9A\0%9B\0%9C\0%9D\0%9E\0%9F\0"
    "%A0\0%A1\0%A2\0%A3\0%A4\0%A5\0%A6\0%A7\0"
    "%A8\0%A9\0%AA\0%AB\0%AC\0%AD\0%AE\0%AF\0"
    "%B0\0%B1\0%B2\0%B3\0%B4\0%B5\0%B6\0%B7\0"
    "%B8\0%B9\0%BA\0%BB\0%BC\0%BD\0%BE\0%BF\0"
    "%C0\0%C1\0%C2\0%C3\0%C4\0%C5\0%C6\0%C7\0"
    "%C8\0%C9\0%CA\0%CB\0%CC\0%CD\0%CE\0%CF\0"
    "%D0\0%D1\0%D2\0%D3\0%D4\0%D5\0%D6\0%D7\0"
    "%D8\0%D9\0%DA\0%DB\0%DC\0%DD\0%DE\0%DF\0"
    "%E0\0%E1\0%E2\0%E3\0%E4\0%E5\0%E6\0%E7\0"
    "%E8\0%E9\0%EA\0%EB\0%EC\0%ED\0%EE\0%EF\0"
    "%F0\0%F1\0%F2\0%F3\0%F4\0%F5\0%F6\0%F7\0"
    "%F8\0%F9\0%FA\0%FB\0%FC\0%FD\0%FE\0%FF\0";

}  // namespace detail

/**
 * Get the percent-encoded representation of a byte.
 *
 * @return 3-character string_view like "%2F" (without null terminator)
 */
[[nodiscard]] constexpr std::string_view
percent_encode(unsigned char byte) noexcept {
  return {detail::hex_encode_table + (byte * 4), 3};
}

/**
 * Check if a character needs percent-encoding for URLs.
 *
 * Characters that don't need encoding: A-Z, a-z, 0-9, - _ . ~
 */
[[nodiscard]] constexpr bool needs_percent_encode(char c) noexcept {
  return !is_alnum(c) & (c != '-') & (c != '_') & (c != '.') & (c != '~');
}

// ============================================================================
// Bit Manipulation Utilities
// ============================================================================

/**
 * Count leading zeros in a 32-bit integer.
 *
 * Uses compiler intrinsics when available for maximum performance.
 */
[[nodiscard]] inline int leading_zeros(uint32_t x) noexcept {
  if (x == 0) return 32;
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_clz(x);
#elif defined(_MSC_VER)
  unsigned long index;
  _BitScanReverse(&index, x);
  return 31 - static_cast<int>(index);
#else
  return std::countl_zero(x);
#endif
}

/**
 * Count the number of decimal digits in a 32-bit integer.
 *
 * Uses a precomputed table for O(1) performance.
 */
[[nodiscard]] inline int digit_count(uint32_t x) noexcept {
  // Use log10 approximation via leading zeros
  // floor(log2(x)) / log2(10) ≈ floor(log10(x))
  if (x == 0) return 1;

  static constexpr uint64_t table[] = {
      4294967296,  8589934582,  8589934582,  8589934582,  12884901788,
      12884901788, 12884901788, 17179868184, 17179868184, 17179868184,
      21474826480, 21474826480, 21474826480, 21474826480, 25769703776,
      25769703776, 25769703776, 30063771072, 30063771072, 30063771072,
      34357838368, 34357838368, 34357838368, 34357838368, 38554705664,
      38554705664, 38554705664, 41949672960, 41949672960, 41949672960,
      42949672960, 42949672960};

  int log2 = 31 - leading_zeros(x | 1);
  return static_cast<int>((x + table[log2]) >> 32);
}

}  // namespace ua

#endif  // UA_CHAR_UTILS_H
