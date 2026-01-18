#ifndef COMPTIME_ESCAPE_STRING_H
#define COMPTIME_ESCAPE_STRING_H

#include <comptime/fixed_string.h>

#include <cstddef>

namespace comptime {

// ============================================================================
// Compile-time string escaping utilities
// ============================================================================

namespace detail {

// Hex digit conversion
constexpr char to_hex_digit(unsigned char nibble) noexcept {
  return nibble < 10 ? ('0' + nibble) : ('a' + nibble - 10);
}

// JSON escape output size for a single character
constexpr std::size_t json_char_escape_size(char c) noexcept {
  switch (c) {
    case '"':
    case '\\':
    case '\b':
    case '\f':
    case '\n':
    case '\r':
    case '\t':
      return 2;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        return 6;  // \uXXXX
      }
      return 1;
  }
}

// Write a JSON-escaped character, returns number of chars written
constexpr std::size_t write_json_escaped_char(char c, char* out) noexcept {
  switch (c) {
    case '"':
      out[0] = '\\';
      out[1] = '"';
      return 2;
    case '\\':
      out[0] = '\\';
      out[1] = '\\';
      return 2;
    case '\b':
      out[0] = '\\';
      out[1] = 'b';
      return 2;
    case '\f':
      out[0] = '\\';
      out[1] = 'f';
      return 2;
    case '\n':
      out[0] = '\\';
      out[1] = 'n';
      return 2;
    case '\r':
      out[0] = '\\';
      out[1] = 'r';
      return 2;
    case '\t':
      out[0] = '\\';
      out[1] = 't';
      return 2;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        out[0] = '\\';
        out[1] = 'u';
        out[2] = '0';
        out[3] = '0';
        out[4] = to_hex_digit((c >> 4) & 0x0F);
        out[5] = to_hex_digit(c & 0x0F);
        return 6;
      }
      out[0] = c;
      return 1;
  }
}

// Calculate total JSON escaped size
template <fixed_string Str>
consteval std::size_t json_escape_output_size() noexcept {
  std::size_t result = 0;
  for (std::size_t i = 0; i < Str.size(); ++i) {
    result += json_char_escape_size(Str.data[i]);
  }
  return result;
}

// Calculate JSON quoted size (with surrounding quotes)
template <fixed_string Str>
consteval std::size_t json_quoted_output_size() noexcept {
  return 2 + json_escape_output_size<Str>();
}

// Percent encoding requirements (URL encoding)
constexpr bool needs_percent_encode(char c) noexcept {
  // Unreserved characters in RFC 3986
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
      (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
    return false;
  }
  return true;
}

// Calculate percent-encoded size
template <fixed_string Str>
consteval std::size_t percent_encode_output_size() noexcept {
  std::size_t result = 0;
  for (std::size_t i = 0; i < Str.size(); ++i) {
    result += needs_percent_encode(Str.data[i]) ? 3 : 1;
  }
  return result;
}

}  // namespace detail

// ============================================================================
// JSON escaping
// ============================================================================

/**
 * Escape a string for use in JSON at compile time.
 *
 * Escapes the following characters:
 * - " -> \"
 * - \ -> \\
 * - Control characters (< 0x20) -> \uXXXX
 * - Special characters: \b, \f, \n, \r, \t
 *
 * Example:
 *   constexpr auto escaped = json_escape<"Hello \"World\"">();
 *   // Result: Hello \"World\"
 */
template <fixed_string Str>
consteval auto json_escape() {
  constexpr std::size_t output_len = detail::json_escape_output_size<Str>();

  fixed_string<char, output_len> result{};
  std::size_t out_idx = 0;

  for (std::size_t i = 0; i < Str.size(); ++i) {
    out_idx += detail::write_json_escaped_char(Str.data[i], &result.data[out_idx]);
  }

  return result;
}

/**
 * Quote and escape a string for JSON at compile time.
 *
 * Adds surrounding double quotes and escapes the content.
 *
 * Example:
 *   constexpr auto quoted = json_quoted<"Hello \"World\"">();
 *   // Result: "Hello \"World\""
 */
template <fixed_string Str>
consteval auto json_quoted() {
  constexpr std::size_t output_len = detail::json_quoted_output_size<Str>();

  fixed_string<char, output_len> result{};
  std::size_t out_idx = 0;

  result.data[out_idx++] = '"';
  for (std::size_t i = 0; i < Str.size(); ++i) {
    out_idx += detail::write_json_escaped_char(Str.data[i], &result.data[out_idx]);
  }
  result.data[out_idx++] = '"';

  return result;
}

// ============================================================================
// URL percent encoding
// ============================================================================

/**
 * Percent-encode a string for use in URLs at compile time.
 *
 * Encodes all characters except unreserved characters (A-Z, a-z, 0-9, -, _, ., ~)
 * per RFC 3986.
 *
 * Example:
 *   constexpr auto encoded = percent_encode<"Hello World!">();
 *   // Result: Hello%20World%21
 */
template <fixed_string Str>
consteval auto percent_encode() {
  constexpr std::size_t output_len = detail::percent_encode_output_size<Str>();

  fixed_string<char, output_len> result{};
  std::size_t out_idx = 0;

  for (std::size_t i = 0; i < Str.size(); ++i) {
    char c = Str.data[i];

    if (!detail::needs_percent_encode(c)) {
      result.data[out_idx++] = c;
    } else {
      result.data[out_idx++] = '%';
      result.data[out_idx++] =
          detail::to_hex_digit((static_cast<unsigned char>(c) >> 4) & 0x0F);
      result.data[out_idx++] =
          detail::to_hex_digit(static_cast<unsigned char>(c) & 0x0F);
    }
  }

  return result;
}

// ============================================================================
// HTML entity encoding
// ============================================================================

namespace detail {

// HTML escape requirements
constexpr bool needs_html_escape(char c) noexcept {
  return c == '&' || c == '<' || c == '>' || c == '"' || c == '\'';
}

// HTML escape output size
constexpr std::size_t html_escape_size(char c) noexcept {
  switch (c) {
    case '&':
      return 5;  // &amp;
    case '<':
      return 4;  // &lt;
    case '>':
      return 4;  // &gt;
    case '"':
      return 6;  // &quot;
    case '\'':
      return 6;  // &#x27;
    default:
      return 1;
  }
}

template <fixed_string Str>
consteval std::size_t html_escape_output_size() noexcept {
  std::size_t result = 0;
  for (std::size_t i = 0; i < Str.size(); ++i) {
    result += html_escape_size(Str.data[i]);
  }
  return result;
}

}  // namespace detail

/**
 * Escape a string for safe use in HTML at compile time.
 *
 * Escapes the following characters:
 * - & -> &amp;
 * - < -> &lt;
 * - > -> &gt;
 * - " -> &quot;
 * - ' -> &#x27;
 *
 * Example:
 *   constexpr auto escaped = html_escape<"<script>alert('XSS')</script>">();
 *   // Result: &lt;script&gt;alert(&#x27;XSS&#x27;)&lt;/script&gt;
 */
template <fixed_string Str>
consteval auto html_escape() {
  constexpr std::size_t output_len = detail::html_escape_output_size<Str>();

  fixed_string<char, output_len> result{};
  std::size_t out_idx = 0;

  for (std::size_t i = 0; i < Str.size(); ++i) {
    char c = Str.data[i];

    switch (c) {
      case '&':
        result.data[out_idx++] = '&';
        result.data[out_idx++] = 'a';
        result.data[out_idx++] = 'm';
        result.data[out_idx++] = 'p';
        result.data[out_idx++] = ';';
        break;
      case '<':
        result.data[out_idx++] = '&';
        result.data[out_idx++] = 'l';
        result.data[out_idx++] = 't';
        result.data[out_idx++] = ';';
        break;
      case '>':
        result.data[out_idx++] = '&';
        result.data[out_idx++] = 'g';
        result.data[out_idx++] = 't';
        result.data[out_idx++] = ';';
        break;
      case '"':
        result.data[out_idx++] = '&';
        result.data[out_idx++] = 'q';
        result.data[out_idx++] = 'u';
        result.data[out_idx++] = 'o';
        result.data[out_idx++] = 't';
        result.data[out_idx++] = ';';
        break;
      case '\'':
        result.data[out_idx++] = '&';
        result.data[out_idx++] = '#';
        result.data[out_idx++] = 'x';
        result.data[out_idx++] = '2';
        result.data[out_idx++] = '7';
        result.data[out_idx++] = ';';
        break;
      default:
        result.data[out_idx++] = c;
        break;
    }
  }

  return result;
}

// ============================================================================
// Size query functions
// ============================================================================

/**
 * Get the size of a JSON-escaped string without allocating.
 */
template <fixed_string Str>
consteval std::size_t json_escape_size() noexcept {
  return detail::json_escape_output_size<Str>();
}

/**
 * Get the size of a percent-encoded string without allocating.
 */
template <fixed_string Str>
consteval std::size_t percent_encode_size() noexcept {
  return detail::percent_encode_output_size<Str>();
}

/**
 * Get the size of an HTML-escaped string without allocating.
 */
template <fixed_string Str>
consteval std::size_t html_escape_size() noexcept {
  return detail::html_escape_output_size<Str>();
}

}  // namespace comptime

#endif  // COMPTIME_ESCAPE_STRING_H
