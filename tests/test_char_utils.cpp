#include <doctest/doctest.h>
#include <ua/char_utils.h>

#include <cstdint>
#include <string_view>

using namespace ua;

// ============================================================================
// Case conversion tests
// ============================================================================

TEST_SUITE("case conversion") {
  TEST_CASE("to_lower") {
    static_assert(to_lower('A') == 'a');
    static_assert(to_lower('Z') == 'z');
    static_assert(to_lower('M') == 'm');
    static_assert(to_lower('a') == 'a');  // Already lowercase
    static_assert(to_lower('z') == 'z');

    CHECK(to_lower('A') == 'a');
    CHECK(to_lower('Z') == 'z');
    CHECK(to_lower('a') == 'a');

    // All uppercase letters
    for (char c = 'A'; c <= 'Z'; ++c) {
      CHECK(to_lower(c) == static_cast<char>(c + 32));
    }
  }

  TEST_CASE("to_upper") {
    static_assert(to_upper('a') == 'A');
    static_assert(to_upper('z') == 'Z');
    static_assert(to_upper('m') == 'M');
    static_assert(to_upper('A') == 'A');  // Already uppercase
    static_assert(to_upper('Z') == 'Z');

    CHECK(to_upper('a') == 'A');
    CHECK(to_upper('z') == 'Z');

    // All lowercase letters
    for (char c = 'a'; c <= 'z'; ++c) {
      CHECK(to_upper(c) == static_cast<char>(c - 32));
    }
  }
}

// ============================================================================
// Character classification tests
// ============================================================================

TEST_SUITE("character classification") {
  TEST_CASE("is_digit") {
    // All digits should pass
    static_assert(is_digit('0'));
    static_assert(is_digit('5'));
    static_assert(is_digit('9'));

    // Non-digits should fail
    static_assert(!is_digit('a'));
    static_assert(!is_digit('Z'));
    static_assert(!is_digit(' '));
    static_assert(!is_digit('/'));  // Just before '0'
    static_assert(!is_digit(':'));  // Just after '9'

    for (char c = '0'; c <= '9'; ++c) {
      CHECK(is_digit(c));
    }
    CHECK_FALSE(is_digit('a'));
    CHECK_FALSE(is_digit('!'));
  }

  TEST_CASE("is_hex_digit") {
    // Decimal digits
    static_assert(is_hex_digit('0'));
    static_assert(is_hex_digit('9'));

    // Lowercase hex
    static_assert(is_hex_digit('a'));
    static_assert(is_hex_digit('f'));
    static_assert(!is_hex_digit('g'));

    // Uppercase hex
    static_assert(is_hex_digit('A'));
    static_assert(is_hex_digit('F'));
    static_assert(!is_hex_digit('G'));

    CHECK(is_hex_digit('0'));
    CHECK(is_hex_digit('a'));
    CHECK(is_hex_digit('F'));
    CHECK_FALSE(is_hex_digit('g'));
    CHECK_FALSE(is_hex_digit('z'));
  }

  TEST_CASE("is_alpha") {
    // Lowercase
    static_assert(is_alpha('a'));
    static_assert(is_alpha('m'));
    static_assert(is_alpha('z'));

    // Uppercase
    static_assert(is_alpha('A'));
    static_assert(is_alpha('M'));
    static_assert(is_alpha('Z'));

    // Non-alpha
    static_assert(!is_alpha('0'));
    static_assert(!is_alpha(' '));
    static_assert(!is_alpha('@'));  // Just before 'A'
    static_assert(!is_alpha('['));  // Just after 'Z'

    for (char c = 'a'; c <= 'z'; ++c) {
      CHECK(is_alpha(c));
    }
    for (char c = 'A'; c <= 'Z'; ++c) {
      CHECK(is_alpha(c));
    }
    CHECK_FALSE(is_alpha('1'));
  }

  TEST_CASE("is_alnum") {
    static_assert(is_alnum('a'));
    static_assert(is_alnum('Z'));
    static_assert(is_alnum('0'));
    static_assert(is_alnum('9'));

    static_assert(!is_alnum(' '));
    static_assert(!is_alnum('!'));
    static_assert(!is_alnum('-'));

    CHECK(is_alnum('a'));
    CHECK(is_alnum('5'));
    CHECK_FALSE(is_alnum('_'));
  }

  TEST_CASE("is_whitespace") {
    static_assert(is_whitespace(' '));
    static_assert(is_whitespace('\t'));
    static_assert(is_whitespace('\n'));
    static_assert(is_whitespace('\r'));
    static_assert(is_whitespace('\v'));
    static_assert(is_whitespace('\f'));

    static_assert(!is_whitespace('a'));
    static_assert(!is_whitespace('0'));
    static_assert(!is_whitespace('\0'));

    CHECK(is_whitespace(' '));
    CHECK(is_whitespace('\n'));
    CHECK_FALSE(is_whitespace('x'));
  }

  TEST_CASE("is_printable") {
    static_assert(is_printable(' '));  // 0x20
    static_assert(is_printable('~'));  // 0x7E
    static_assert(is_printable('a'));
    static_assert(is_printable('0'));
    static_assert(is_printable('!'));

    static_assert(!is_printable('\0'));
    static_assert(!is_printable('\n'));
    static_assert(!is_printable('\x7F'));  // DEL

    CHECK(is_printable('A'));
    CHECK_FALSE(is_printable('\t'));
  }

  TEST_CASE("is_control") {
    static_assert(is_control('\0'));
    static_assert(is_control('\n'));
    static_assert(is_control('\t'));
    static_assert(is_control('\x1F'));
    static_assert(is_control('\x7F'));  // DEL

    static_assert(!is_control(' '));
    static_assert(!is_control('a'));
    static_assert(!is_control('~'));

    CHECK(is_control('\0'));
    CHECK(is_control('\x7F'));
    CHECK_FALSE(is_control(' '));
  }

  TEST_CASE("is_ascii") {
    static_assert(is_ascii('\0'));
    static_assert(is_ascii('a'));
    static_assert(is_ascii('\x7F'));

    // Non-ASCII (bytes >= 0x80)
    static_assert(!is_ascii('\x80'));
    static_assert(!is_ascii('\xFF'));

    CHECK(is_ascii('A'));
    CHECK_FALSE(is_ascii(static_cast<char>(0x80)));
  }

  TEST_CASE("is_utf8_continuation") {
    // Continuation bytes: 10xxxxxx (0x80-0xBF)
    static_assert(is_utf8_continuation('\x80'));
    static_assert(is_utf8_continuation('\x8F'));
    static_assert(is_utf8_continuation('\xBF'));

    // Not continuation bytes
    static_assert(!is_utf8_continuation('\x00'));
    static_assert(!is_utf8_continuation('\x7F'));
    static_assert(!is_utf8_continuation('\xC0'));
    static_assert(!is_utf8_continuation('\xFF'));

    CHECK(is_utf8_continuation(static_cast<char>(0x80)));
    CHECK_FALSE(is_utf8_continuation('a'));
  }

  TEST_CASE("is_utf8_leading") {
    // Leading bytes: 110xxxxx (2-byte), 1110xxxx (3-byte), 11110xxx (4-byte)
    static_assert(is_utf8_leading('\xC0'));  // 2-byte start
    static_assert(is_utf8_leading('\xDF'));
    static_assert(is_utf8_leading('\xE0'));  // 3-byte start
    static_assert(is_utf8_leading('\xEF'));
    static_assert(is_utf8_leading('\xF0'));  // 4-byte start
    static_assert(is_utf8_leading('\xF7'));

    // Not leading bytes
    static_assert(!is_utf8_leading('a'));
    static_assert(!is_utf8_leading('\x80'));  // Continuation
    static_assert(!is_utf8_leading('\xF8'));  // Invalid

    CHECK(is_utf8_leading(static_cast<char>(0xC0)));
    CHECK_FALSE(is_utf8_leading('A'));
  }
}

// ============================================================================
// Numeric value extraction tests
// ============================================================================

TEST_SUITE("numeric value extraction") {
  TEST_CASE("digit_value") {
    static_assert(digit_value('0') == 0);
    static_assert(digit_value('5') == 5);
    static_assert(digit_value('9') == 9);

    for (int i = 0; i <= 9; ++i) {
      CHECK(digit_value(static_cast<char>('0' + i)) == i);
    }
  }

  TEST_CASE("hex_value") {
    // Decimal digits
    static_assert(hex_value('0') == 0);
    static_assert(hex_value('9') == 9);

    // Lowercase hex
    static_assert(hex_value('a') == 10);
    static_assert(hex_value('f') == 15);

    // Uppercase hex
    static_assert(hex_value('A') == 10);
    static_assert(hex_value('F') == 15);

    CHECK(hex_value('0') == 0);
    CHECK(hex_value('a') == 10);
    CHECK(hex_value('F') == 15);
  }

  TEST_CASE("to_hex_lower") {
    static_assert(to_hex_lower(0) == '0');
    static_assert(to_hex_lower(9) == '9');
    static_assert(to_hex_lower(10) == 'a');
    static_assert(to_hex_lower(15) == 'f');

    for (int i = 0; i < 16; ++i) {
      char expected = (i < 10) ? static_cast<char>('0' + i)
                               : static_cast<char>('a' + i - 10);
      CHECK(to_hex_lower(i) == expected);
    }
  }

  TEST_CASE("to_hex_upper") {
    static_assert(to_hex_upper(0) == '0');
    static_assert(to_hex_upper(9) == '9');
    static_assert(to_hex_upper(10) == 'A');
    static_assert(to_hex_upper(15) == 'F');

    for (int i = 0; i < 16; ++i) {
      char expected = (i < 10) ? static_cast<char>('0' + i)
                               : static_cast<char>('A' + i - 10);
      CHECK(to_hex_upper(i) == expected);
    }
  }
}

// ============================================================================
// Prefix detection tests
// ============================================================================

TEST_SUITE("prefix detection") {
  TEST_CASE("has_hex_prefix") {
    static_assert(has_hex_prefix("0x123"));
    static_assert(has_hex_prefix("0X123"));
    static_assert(has_hex_prefix("0xABCD"));

    static_assert(!has_hex_prefix(""));
    static_assert(!has_hex_prefix("0"));
    static_assert(!has_hex_prefix("123"));
    static_assert(!has_hex_prefix("x0"));

    CHECK(has_hex_prefix("0x1"));
    CHECK(has_hex_prefix("0XFF"));
    CHECK_FALSE(has_hex_prefix("hex"));
    CHECK_FALSE(has_hex_prefix("0"));
  }

  TEST_CASE("has_hex_prefix_unsafe") {
    static_assert(has_hex_prefix_unsafe("0x123"));
    static_assert(has_hex_prefix_unsafe("0X123"));

    static_assert(!has_hex_prefix_unsafe("ab"));
    static_assert(!has_hex_prefix_unsafe("00"));

    CHECK(has_hex_prefix_unsafe("0xAB"));
    CHECK_FALSE(has_hex_prefix_unsafe("xy"));
  }

  TEST_CASE("has_bin_prefix") {
    static_assert(has_bin_prefix("0b101"));
    static_assert(has_bin_prefix("0B101"));

    static_assert(!has_bin_prefix("0x10"));
    static_assert(!has_bin_prefix("b0"));
    static_assert(!has_bin_prefix(""));

    CHECK(has_bin_prefix("0b1"));
    CHECK_FALSE(has_bin_prefix("0x1"));
  }

  TEST_CASE("has_oct_prefix") {
    static_assert(has_oct_prefix("0o777"));
    static_assert(has_oct_prefix("0O777"));

    static_assert(!has_oct_prefix("0x10"));
    static_assert(!has_oct_prefix("o0"));

    CHECK(has_oct_prefix("0o7"));
    CHECK_FALSE(has_oct_prefix("0b1"));
  }
}

// ============================================================================
// Path/URL utilities tests
// ============================================================================

TEST_SUITE("path utilities") {
  TEST_CASE("is_windows_drive_letter") {
    static_assert(is_windows_drive_letter("C:"));
    static_assert(is_windows_drive_letter("D:"));
    static_assert(is_windows_drive_letter("c:"));
    static_assert(is_windows_drive_letter("Z:"));
    static_assert(is_windows_drive_letter("C|"));  // Old-style
    static_assert(is_windows_drive_letter("d|"));

    static_assert(!is_windows_drive_letter(""));
    static_assert(!is_windows_drive_letter("C"));
    static_assert(!is_windows_drive_letter("1:"));
    static_assert(!is_windows_drive_letter("::"));

    CHECK(is_windows_drive_letter("C:"));
    CHECK(is_windows_drive_letter("c|"));
    CHECK_FALSE(is_windows_drive_letter("/home"));
  }

  TEST_CASE("is_normalized_windows_drive") {
    static_assert(is_normalized_windows_drive("C:"));
    static_assert(is_normalized_windows_drive("d:"));

    static_assert(!is_normalized_windows_drive("C|"));  // Not normalized
    static_assert(!is_normalized_windows_drive("d|"));
    static_assert(!is_normalized_windows_drive(""));

    CHECK(is_normalized_windows_drive("C:"));
    CHECK_FALSE(is_normalized_windows_drive("C|"));
  }
}

// ============================================================================
// Case-insensitive comparison tests
// ============================================================================

TEST_SUITE("case-insensitive comparison") {
  TEST_CASE("iequal_char") {
    static_assert(iequal_char('a', 'a'));
    static_assert(iequal_char('a', 'A'));
    static_assert(iequal_char('A', 'a'));
    static_assert(iequal_char('Z', 'z'));

    static_assert(!iequal_char('a', 'b'));
    static_assert(!iequal_char('A', 'B'));

    CHECK(iequal_char('X', 'x'));
    CHECK_FALSE(iequal_char('a', 'z'));
  }

  TEST_CASE("iequal") {
    static_assert(iequal("hello", "hello"));
    static_assert(iequal("hello", "HELLO"));
    static_assert(iequal("HELLO", "hello"));
    static_assert(iequal("HeLLo", "hEllO"));

    static_assert(!iequal("hello", "world"));
    static_assert(!iequal("hello", "helloX"));
    static_assert(!iequal("hello", "hell"));

    CHECK(iequal("Test", "TEST"));
    CHECK(iequal("", ""));
    CHECK_FALSE(iequal("abc", "abd"));
  }

  TEST_CASE("istarts_with") {
    static_assert(istarts_with("hello world", "hello"));
    static_assert(istarts_with("hello world", "HELLO"));
    static_assert(istarts_with("HELLO world", "hello"));
    static_assert(istarts_with("test", "test"));
    static_assert(istarts_with("test", ""));

    static_assert(!istarts_with("hello", "world"));
    static_assert(!istarts_with("hi", "hello"));

    CHECK(istarts_with("Content-Type", "content-"));
    CHECK_FALSE(istarts_with("abc", "abcd"));
  }

  TEST_CASE("iends_with") {
    static_assert(iends_with("hello.TXT", ".txt"));
    static_assert(iends_with("hello.txt", ".TXT"));
    static_assert(iends_with("test", "test"));
    static_assert(iends_with("test", ""));

    static_assert(!iends_with("hello.txt", ".html"));
    static_assert(!iends_with("hi", "hello"));

    CHECK(iends_with("file.JSON", ".json"));
    CHECK_FALSE(iends_with("abc", "xabc"));
  }
}

// ============================================================================
// Percent encoding tests
// ============================================================================

TEST_SUITE("percent encoding") {
  TEST_CASE("percent_encode") {
    static_assert(percent_encode(0x00) == "%00");
    static_assert(percent_encode(0x20) == "%20");  // Space
    static_assert(percent_encode(0x2F) == "%2F");  // Slash
    static_assert(percent_encode(0x41) == "%41");  // 'A'
    static_assert(percent_encode(0xFF) == "%FF");

    CHECK(percent_encode(' ') == "%20");
    CHECK(percent_encode('/') == "%2F");
    CHECK(percent_encode('\xFF') == "%FF");
  }

  TEST_CASE("needs_percent_encode") {
    // These don't need encoding: A-Z, a-z, 0-9, - _ . ~
    static_assert(!needs_percent_encode('a'));
    static_assert(!needs_percent_encode('Z'));
    static_assert(!needs_percent_encode('0'));
    static_assert(!needs_percent_encode('9'));
    static_assert(!needs_percent_encode('-'));
    static_assert(!needs_percent_encode('_'));
    static_assert(!needs_percent_encode('.'));
    static_assert(!needs_percent_encode('~'));

    // These need encoding
    static_assert(needs_percent_encode(' '));
    static_assert(needs_percent_encode('/'));
    static_assert(needs_percent_encode('?'));
    static_assert(needs_percent_encode('#'));
    static_assert(needs_percent_encode('%'));
    static_assert(needs_percent_encode('\0'));

    CHECK(!needs_percent_encode('a'));
    CHECK(needs_percent_encode(' '));
    CHECK(needs_percent_encode('/'));
  }
}

// ============================================================================
// Bit manipulation tests
// ============================================================================

TEST_SUITE("bit manipulation") {
  TEST_CASE("leading_zeros") {
    CHECK(leading_zeros(0) == 32);
    CHECK(leading_zeros(1) == 31);
    CHECK(leading_zeros(2) == 30);
    CHECK(leading_zeros(0x80000000) == 0);
    CHECK(leading_zeros(0x7FFFFFFF) == 1);
    CHECK(leading_zeros(0x00010000) == 15);
  }

  TEST_CASE("digit_count") {
    CHECK(digit_count(0) == 1);
    CHECK(digit_count(1) == 1);
    CHECK(digit_count(9) == 1);
    CHECK(digit_count(10) == 2);
    CHECK(digit_count(99) == 2);
    CHECK(digit_count(100) == 3);
    CHECK(digit_count(999) == 3);
    CHECK(digit_count(1000) == 4);
    CHECK(digit_count(1000000) == 7);
    CHECK(digit_count(999999999) == 9);
    CHECK(digit_count(1000000000) == 10);
    CHECK(digit_count(4294967295u) == 10);  // Max uint32_t
  }
}

// ============================================================================
// Practical usage tests
// ============================================================================

TEST_SUITE("practical usage") {
  TEST_CASE("URL path encoding") {
    constexpr auto encode_path_char = [](char c) -> std::string_view {
      if (!needs_percent_encode(c)) {
        return {};  // No encoding needed
      }
      return percent_encode(static_cast<unsigned char>(c));
    };

    CHECK(encode_path_char('a').empty());
    CHECK(encode_path_char(' ') == "%20");
    CHECK(encode_path_char('/') == "%2F");
  }

  TEST_CASE("parse hex string") {
    constexpr auto parse_hex_byte = [](char hi, char lo) -> uint8_t {
      return static_cast<uint8_t>((hex_value(hi) << 4) | hex_value(lo));
    };

    static_assert(parse_hex_byte('F', 'F') == 255);
    static_assert(parse_hex_byte('0', '0') == 0);
    static_assert(parse_hex_byte('A', 'B') == 0xAB);

    CHECK(parse_hex_byte('C', 'D') == 0xCD);
  }

  TEST_CASE("case-insensitive content-type check") {
    auto is_json = [](std::string_view content_type) {
      return iequal(content_type, "application/json") ||
             istarts_with(content_type, "application/json;");
    };

    CHECK(is_json("application/json"));
    CHECK(is_json("Application/JSON"));
    CHECK(is_json("application/json; charset=utf-8"));
    CHECK_FALSE(is_json("text/plain"));
  }
}
