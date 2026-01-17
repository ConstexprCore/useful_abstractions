#include <doctest/doctest.h>
#include <comptime/utf_convert.h>

using namespace comptime;

// ============================================================================
// utf8_to_utf16 tests
// ============================================================================

TEST_SUITE("utf8_to_utf16") {
  TEST_CASE("ASCII string") {
    constexpr auto result = utf8_to_utf16<"Hello">();

    static_assert(result.size() == 5);
    static_assert(result[0] == u'H');
    static_assert(result[1] == u'e');
    static_assert(result[2] == u'l');
    static_assert(result[3] == u'l');
    static_assert(result[4] == u'o');

    CHECK(result.size() == 5);
    CHECK(result[0] == u'H');
    CHECK(result[4] == u'o');
  }

  TEST_CASE("empty string") {
    constexpr auto result = utf8_to_utf16<"">();

    static_assert(result.size() == 0);
    CHECK(result.size() == 0);
  }

  TEST_CASE("2-byte UTF-8 sequences") {
    // "café" - the 'é' is 2 bytes in UTF-8 (0xC3 0xA9 = U+00E9)
    constexpr auto result = utf8_to_utf16<"caf\xC3\xA9">();

    static_assert(result.size() == 4);
    static_assert(result[0] == u'c');
    static_assert(result[1] == u'a');
    static_assert(result[2] == u'f');
    static_assert(result[3] == 0x00E9);  // é

    CHECK(result.size() == 4);
    CHECK(result[3] == 0x00E9);
  }

  TEST_CASE("3-byte UTF-8 sequences") {
    // Euro sign € is 3 bytes in UTF-8 (0xE2 0x82 0xAC = U+20AC)
    constexpr auto result = utf8_to_utf16<"\xE2\x82\xAC">();

    static_assert(result.size() == 1);
    static_assert(result[0] == 0x20AC);  // €

    CHECK(result.size() == 1);
    CHECK(result[0] == 0x20AC);
  }

  TEST_CASE("4-byte UTF-8 to surrogate pair") {
    // 𝄞 (musical G clef) is U+1D11E, requires surrogate pair in UTF-16
    // UTF-8: 0xF0 0x9D 0x84 0x9E
    constexpr auto result = utf8_to_utf16<"\xF0\x9D\x84\x9E">();

    static_assert(result.size() == 2);  // Surrogate pair
    static_assert(result[0] == 0xD834);  // High surrogate
    static_assert(result[1] == 0xDD1E);  // Low surrogate

    CHECK(result.size() == 2);
    CHECK(result[0] == 0xD834);
    CHECK(result[1] == 0xDD1E);
  }

  TEST_CASE("mixed ASCII and multibyte") {
    // "a€b" - ASCII, 3-byte UTF-8, ASCII
    // Use string concatenation to avoid hex digit ambiguity
    constexpr auto result = utf8_to_utf16<"a\xE2\x82\xAC" "b">();

    static_assert(result.size() == 3);
    static_assert(result[0] == u'a');
    static_assert(result[1] == 0x20AC);
    static_assert(result[2] == u'b');

    CHECK(result.size() == 3);
  }
}

// ============================================================================
// utf8_to_utf32 tests
// ============================================================================

TEST_SUITE("utf8_to_utf32") {
  TEST_CASE("ASCII string") {
    constexpr auto result = utf8_to_utf32<"Hello">();

    static_assert(result.size() == 5);
    static_assert(result[0] == U'H');
    static_assert(result[4] == U'o');

    CHECK(result.size() == 5);
  }

  TEST_CASE("multibyte characters") {
    // "€" (U+20AC)
    constexpr auto result = utf8_to_utf32<"\xE2\x82\xAC">();

    static_assert(result.size() == 1);
    static_assert(result[0] == 0x20AC);

    CHECK(result.size() == 1);
    CHECK(result[0] == 0x20AC);
  }

  TEST_CASE("4-byte UTF-8") {
    // 𝄞 (U+1D11E)
    constexpr auto result = utf8_to_utf32<"\xF0\x9D\x84\x9E">();

    static_assert(result.size() == 1);
    static_assert(result[0] == 0x1D11E);

    CHECK(result.size() == 1);
    CHECK(result[0] == 0x1D11E);
  }

  TEST_CASE("mixed string") {
    // "a€𝄞b"
    constexpr auto result = utf8_to_utf32<"a\xE2\x82\xAC\xF0\x9D\x84\x9E" "b">();

    static_assert(result.size() == 4);
    static_assert(result[0] == U'a');
    static_assert(result[1] == 0x20AC);
    static_assert(result[2] == 0x1D11E);
    static_assert(result[3] == U'b');

    CHECK(result.size() == 4);
  }
}

// ============================================================================
// Code point counting tests
// ============================================================================

TEST_SUITE("code point counting") {
  TEST_CASE("utf8_code_point_count ASCII") {
    static_assert(utf8_code_point_count<"Hello">() == 5);
    CHECK(utf8_code_point_count<"Hello">() == 5);
  }

  TEST_CASE("utf8_code_point_count multibyte") {
    // "a€b" - 3 code points
    static_assert(utf8_code_point_count<"a\xE2\x82\xAC" "b">() == 3);
    CHECK(utf8_code_point_count<"a\xE2\x82\xAC" "b">() == 3);
  }

  TEST_CASE("utf8_code_point_count 4-byte") {
    // Single 4-byte character
    static_assert(utf8_code_point_count<"\xF0\x9D\x84\x9E">() == 1);
    CHECK(utf8_code_point_count<"\xF0\x9D\x84\x9E">() == 1);
  }

  TEST_CASE("utf8_code_point_count empty") {
    static_assert(utf8_code_point_count<"">() == 0);
    CHECK(utf8_code_point_count<"">() == 0);
  }

  TEST_CASE("utf8_to_utf16_length") {
    // ASCII: same length
    static_assert(utf8_to_utf16_length<"Hello">() == 5);

    // 3-byte UTF-8 -> 1 UTF-16 code unit
    static_assert(utf8_to_utf16_length<"\xE2\x82\xAC">() == 1);

    // 4-byte UTF-8 -> 2 UTF-16 code units (surrogate pair)
    static_assert(utf8_to_utf16_length<"\xF0\x9D\x84\x9E">() == 2);
  }
}

// ============================================================================
// UTF-8 validation tests
// ============================================================================

TEST_SUITE("is_valid_utf8") {
  TEST_CASE("ASCII is valid") {
    static_assert(is_valid_utf8<"Hello, World!">());
    static_assert(is_valid_utf8<"">());
    static_assert(is_valid_utf8<"123">());

    CHECK(is_valid_utf8<"Hello, World!">());
  }

  TEST_CASE("valid multibyte sequences") {
    // 2-byte: é (U+00E9)
    static_assert(is_valid_utf8<"\xC3\xA9">());

    // 3-byte: € (U+20AC)
    static_assert(is_valid_utf8<"\xE2\x82\xAC">());

    // 4-byte: 𝄞 (U+1D11E)
    static_assert(is_valid_utf8<"\xF0\x9D\x84\x9E">());

    CHECK(is_valid_utf8<"\xE2\x82\xAC">());
  }

  TEST_CASE("mixed valid UTF-8") {
    static_assert(is_valid_utf8<"Hello \xC3\xA9 World">());
    static_assert(is_valid_utf8<"Price: \xE2\x82\xAC" "100">());

    CHECK(is_valid_utf8<"Price: \xE2\x82\xAC" "100">());
  }

  TEST_CASE("invalid: truncated sequences") {
    // Truncated 2-byte
    static_assert(!is_valid_utf8<"\xC3">());

    // Truncated 3-byte
    static_assert(!is_valid_utf8<"\xE2\x82">());

    // Truncated 4-byte
    static_assert(!is_valid_utf8<"\xF0\x9D\x84">());
  }

  TEST_CASE("invalid: overlong encodings") {
    // Overlong encoding of '/' (should be 0x2F, not C0 AF)
    static_assert(!is_valid_utf8<"\xC0\xAF">());
    static_assert(!is_valid_utf8<"\xC1\xBF">());
  }

  TEST_CASE("invalid: continuation byte as lead") {
    static_assert(!is_valid_utf8<"\x80">());
    static_assert(!is_valid_utf8<"\xBF">());
  }
}

// ============================================================================
// Practical usage tests
// ============================================================================

TEST_SUITE("practical usage") {
  TEST_CASE("compile-time string conversion") {
    constexpr auto wide = utf8_to_utf16<"Hello, World!">();

    static_assert(wide.size() == 13);
    CHECK(wide.size() == 13);
  }

  TEST_CASE("static_string integration") {
    constexpr auto utf16 = utf8_to_utf16<"test">();

    static_assert(utf16.size() == 4);
    static_assert(std::is_same_v<decltype(utf16)::value_type, char16_t>);

    CHECK(utf16.size() == 4);
  }

  TEST_CASE("convert to UTF-32 for processing") {
    // Convert UTF-8 to UTF-32 for easy code point access
    constexpr auto codepoints = utf8_to_utf32<"a\xE2\x82\xAC" "b">();

    static_assert(codepoints.size() == 3);
    static_assert(codepoints[0] == U'a');
    static_assert(codepoints[1] == 0x20AC);  // Euro
    static_assert(codepoints[2] == U'b');

    CHECK(codepoints[1] == 0x20AC);
  }

  TEST_CASE("validation before conversion") {
    // Can validate UTF-8 before converting
    static_assert(is_valid_utf8<"Hello">());
    constexpr auto result = utf8_to_utf16<"Hello">();
    static_assert(result.size() == 5);
  }
}
