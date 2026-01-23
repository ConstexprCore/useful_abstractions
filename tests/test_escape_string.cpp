#include <doctest/doctest.h>
#include <ConstexprCore/escape_string.h>

#include <string_view>

using namespace ConstexprCore;

// Helper to get string_view from fixed_string<char, N>
template <std::size_t N>
std::string_view to_sv(const fixed_string<char, N>& s) {
  return {s.data.data(), s.size()};
}

// ============================================================================
// JSON escaping tests
// ============================================================================

TEST_SUITE("json_escape") {
  TEST_CASE("plain ASCII string") {
    constexpr auto result = json_escape<"Hello World">();

    static_assert(result.size() == 11);
    static_assert(result[0] == 'H');
    static_assert(result[10] == 'd');

    CHECK(result.size() == 11);
    CHECK(to_sv(result) == "Hello World");
  }

  TEST_CASE("empty string") {
    constexpr auto result = json_escape<"">();

    static_assert(result.size() == 0);
    CHECK(result.size() == 0);
  }

  TEST_CASE("escape double quotes") {
    // "say \"hello\"" is 11 chars: s,a,y, ,",h,e,l,l,o,"
    // 2 quotes each become 2 chars -> 11 + 2 = 13
    constexpr auto result = json_escape<"say \"hello\"">();

    static_assert(result.size() == 13);

    CHECK(to_sv(result) == "say \\\"hello\\\"");
  }

  TEST_CASE("escape backslash") {
    // "path\to\file" is 12 chars with 2 backslashes
    // 2 backslashes each become 2 chars -> 12 + 2 = 14
    constexpr auto result = json_escape<"path\\to\\file">();

    static_assert(result.size() == 14);

    CHECK(to_sv(result) == "path\\\\to\\\\file");
  }

  TEST_CASE("escape newline") {
    // "line1\nline2" is 11 chars, \n becomes \n (2 chars) -> 11 + 1 = 12
    constexpr auto result = json_escape<"line1\nline2">();

    static_assert(result.size() == 12);

    CHECK(to_sv(result) == "line1\\nline2");
  }

  TEST_CASE("escape carriage return") {
    constexpr auto result = json_escape<"line1\rline2">();

    static_assert(result.size() == 12);

    CHECK(to_sv(result) == "line1\\rline2");
  }

  TEST_CASE("escape tab") {
    // "col1\tcol2" is 9 chars, \t becomes \t (2 chars) -> 9 + 1 = 10
    constexpr auto result = json_escape<"col1\tcol2">();

    static_assert(result.size() == 10);

    CHECK(to_sv(result) == "col1\\tcol2");
  }

  TEST_CASE("escape backspace") {
    // "a\bb" is 3 chars, \b becomes \b (2 chars) -> 3 + 1 = 4
    constexpr auto result = json_escape<"a\bb">();

    static_assert(result.size() == 4);

    CHECK(to_sv(result) == "a\\bb");
  }

  TEST_CASE("escape form feed") {
    constexpr auto result = json_escape<"a\fb">();

    static_assert(result.size() == 4);

    CHECK(to_sv(result) == "a\\fb");
  }

  TEST_CASE("escape control characters") {
    // 0x01 character -> \u0001 (6 chars)
    constexpr auto result = json_escape<"\x01">();

    static_assert(result.size() == 6);

    CHECK(to_sv(result) == "\\u0001");
  }

  TEST_CASE("mixed escaping") {
    // "\"Hello\nWorld\"" is 14 chars: ",H,e,l,l,o,\n,W,o,r,l,d,"
    // 2 quotes (+2) + 1 newline (+1) = 14 + 3 = 17... wait let me count again
    // The string is: quote, H, e, l, l, o, newline, W, o, r, l, d, quote = 13 chars
    // 2 quotes each +1, newline +1 = 13 + 3 = 16
    constexpr auto result = json_escape<"\"Hello\nWorld\"">();

    static_assert(result.size() == 16);

    CHECK(to_sv(result) == "\\\"Hello\\nWorld\\\"");
  }
}

// ============================================================================
// JSON quoted tests
// ============================================================================

TEST_SUITE("json_quoted") {
  TEST_CASE("simple string") {
    constexpr auto result = json_quoted<"hello">();

    static_assert(result.size() == 7);  // 5 + 2 quotes
    static_assert(result[0] == '"');
    static_assert(result[6] == '"');

    CHECK(to_sv(result) == "\"hello\"");
  }

  TEST_CASE("empty string") {
    constexpr auto result = json_quoted<"">();

    static_assert(result.size() == 2);

    CHECK(to_sv(result) == "\"\"");
  }

  TEST_CASE("string with escapes") {
    // "say \"hi\"" is 8 chars: s,a,y,space,",h,i,"
    // 2 quotes each +1 = 8 + 2 = 10, plus 2 surrounding quotes = 12
    constexpr auto result = json_quoted<"say \"hi\"">();

    static_assert(result.size() == 12);

    CHECK(to_sv(result) == "\"say \\\"hi\\\"\"");
  }

  TEST_CASE("string with newline") {
    // "a\nb" is 3 chars, newline +1 = 4, plus 2 quotes = 6
    constexpr auto result = json_quoted<"a\nb">();

    static_assert(result.size() == 6);

    CHECK(to_sv(result) == "\"a\\nb\"");
  }
}

// ============================================================================
// Percent encoding tests
// ============================================================================

TEST_SUITE("percent_encode") {
  TEST_CASE("unreserved characters unchanged") {
    // A-Z, a-z, 0-9, -, _, ., ~ are unreserved
    constexpr auto result = percent_encode<"Hello-World_123.test~">();

    static_assert(result.size() == 21);

    CHECK(to_sv(result) == "Hello-World_123.test~");
  }

  TEST_CASE("empty string") {
    constexpr auto result = percent_encode<"">();

    static_assert(result.size() == 0);
    CHECK(result.size() == 0);
  }

  TEST_CASE("encode space") {
    // "Hello World" = 11 chars, space becomes %20 (3 chars) -> 10 + 3 = 13
    constexpr auto result = percent_encode<"Hello World">();

    static_assert(result.size() == 13);

    CHECK(to_sv(result) == "Hello%20World");
  }

  TEST_CASE("encode special characters") {
    // "a+b=c" = 5 chars, + and = each become 3 chars -> 3 + 3 + 3 = 9
    constexpr auto result = percent_encode<"a+b=c">();

    static_assert(result.size() == 9);

    CHECK(to_sv(result) == "a%2bb%3dc");
  }

  TEST_CASE("encode slash") {
    // "path/to/file" = 12 chars, 2 slashes each become 3 chars -> 10 + 6 = 16
    constexpr auto result = percent_encode<"path/to/file">();

    static_assert(result.size() == 16);

    CHECK(to_sv(result) == "path%2fto%2ffile");
  }

  TEST_CASE("encode question mark and ampersand") {
    // "a?b&c" = 5 chars, ? and & each become 3 chars -> 3 + 3 + 3 = 9
    constexpr auto result = percent_encode<"a?b&c">();

    static_assert(result.size() == 9);

    CHECK(to_sv(result) == "a%3fb%26c");
  }

  TEST_CASE("encode percent sign") {
    // "100%" = 4 chars, % becomes %25 (3 chars) -> 3 + 3 = 6
    constexpr auto result = percent_encode<"100%">();

    static_assert(result.size() == 6);

    CHECK(to_sv(result) == "100%25");
  }

  TEST_CASE("encode non-ASCII") {
    // UTF-8 encoded é (0xC3 0xA9)
    // "café" but we use hex escapes: "caf\xC3\xA9"
    // c,a,f = 3 chars (unreserved), 0xC3 -> %c3 (3), 0xA9 -> %a9 (3) = 9
    constexpr auto result = percent_encode<"caf\xC3\xA9">();

    static_assert(result.size() == 9);

    CHECK(to_sv(result) == "caf%c3%a9");
  }
}

// ============================================================================
// HTML escaping tests
// ============================================================================

TEST_SUITE("html_escape") {
  TEST_CASE("plain text unchanged") {
    constexpr auto result = html_escape<"Hello World">();

    static_assert(result.size() == 11);

    CHECK(to_sv(result) == "Hello World");
  }

  TEST_CASE("empty string") {
    constexpr auto result = html_escape<"">();

    static_assert(result.size() == 0);
    CHECK(result.size() == 0);
  }

  TEST_CASE("escape ampersand") {
    // "a & b" = 5 chars, & becomes &amp; (5 chars) -> 4 + 5 = 9
    constexpr auto result = html_escape<"a & b">();

    static_assert(result.size() == 9);

    CHECK(to_sv(result) == "a &amp; b");
  }

  TEST_CASE("escape less than") {
    // "a < b" = 5 chars, < becomes &lt; (4 chars) -> 4 + 4 = 8
    constexpr auto result = html_escape<"a < b">();

    static_assert(result.size() == 8);

    CHECK(to_sv(result) == "a &lt; b");
  }

  TEST_CASE("escape greater than") {
    constexpr auto result = html_escape<"a > b">();

    static_assert(result.size() == 8);

    CHECK(to_sv(result) == "a &gt; b");
  }

  TEST_CASE("escape double quote") {
    // "say \"hi\"" = 8 chars: s,a,y,space,",h,i,"
    // 2 quotes each become &quot; (6 chars) -> 6 + 12 = 18
    constexpr auto result = html_escape<"say \"hi\"">();

    static_assert(result.size() == 18);

    CHECK(to_sv(result) == "say &quot;hi&quot;");
  }

  TEST_CASE("escape single quote") {
    // "it's" = 4 chars, ' becomes &#x27; (6 chars) -> 3 + 6 = 9
    constexpr auto result = html_escape<"it's">();

    static_assert(result.size() == 9);

    CHECK(to_sv(result) == "it&#x27;s");
  }

  TEST_CASE("escape script tag") {
    // "<script>" = 8 chars, < and > each become 4 chars -> 6 + 4 + 4 = 14
    constexpr auto result = html_escape<"<script>">();

    static_assert(result.size() == 14);

    CHECK(to_sv(result) == "&lt;script&gt;");
  }

  TEST_CASE("XSS prevention") {
    constexpr auto result = html_escape<"<script>alert('XSS')</script>">();

    CHECK(to_sv(result) == "&lt;script&gt;alert(&#x27;XSS&#x27;)&lt;/script&gt;");
  }

  TEST_CASE("mixed HTML entities") {
    constexpr auto result = html_escape<"<a href=\"test\">">();

    CHECK(to_sv(result) == "&lt;a href=&quot;test&quot;&gt;");
  }
}

// ============================================================================
// Size query function tests
// ============================================================================

TEST_SUITE("size queries") {
  TEST_CASE("json_escape_size") {
    static_assert(json_escape_size<"hello">() == 5);
    static_assert(json_escape_size<"\"hi\"">() == 6);  // 4 + 2 for quotes
    static_assert(json_escape_size<"a\nb">() == 4);    // 3 + 1 for \n
    static_assert(json_escape_size<"\x01">() == 6);    // \u0001

    CHECK(json_escape_size<"hello">() == 5);
  }

  TEST_CASE("percent_encode_size") {
    static_assert(percent_encode_size<"hello">() == 5);
    static_assert(percent_encode_size<"a b">() == 5);  // a + %20 + b
    static_assert(percent_encode_size<"100%">() == 6); // 100 + %25

    CHECK(percent_encode_size<"hello">() == 5);
  }

  TEST_CASE("html_escape_size") {
    static_assert(html_escape_size<"hello">() == 5);
    static_assert(html_escape_size<"&">() == 5);    // &amp;
    static_assert(html_escape_size<"<">() == 4);    // &lt;
    static_assert(html_escape_size<"\"">() == 6);   // &quot;

    CHECK(html_escape_size<"hello">() == 5);
  }
}

// ============================================================================
// Practical usage tests
// ============================================================================

TEST_SUITE("practical usage") {
  TEST_CASE("JSON object construction") {
    constexpr auto key = json_quoted<"name">();
    constexpr auto value = json_quoted<"John \"Doc\" Smith">();

    CHECK(to_sv(key) == "\"name\"");
    CHECK(to_sv(value) == "\"John \\\"Doc\\\" Smith\"");
  }

  TEST_CASE("URL query parameter") {
    constexpr auto param = percent_encode<"search query with spaces">();

    CHECK(to_sv(param) == "search%20query%20with%20spaces");
  }

  TEST_CASE("safe HTML output") {
    constexpr auto safe = html_escape<"User input: <dangerous>">();

    CHECK(to_sv(safe) == "User input: &lt;dangerous&gt;");
  }

  TEST_CASE("compile-time size calculation") {
    // Can calculate output buffer size at compile time
    constexpr std::size_t json_size = json_escape_size<"test \"string\"">();
    constexpr std::size_t url_size = percent_encode_size<"hello world">();
    constexpr std::size_t html_size = html_escape_size<"<tag>">();

    // "test \"string\"" = 13 chars, 2 quotes +2 = 15
    static_assert(json_size == 15);
    // "hello world" = 11 chars, space -> %20 = 10 + 3 = 13
    static_assert(url_size == 13);
    // "<tag>" = 5 chars, < and > each +3 = 3 + 4 + 4 = 11
    static_assert(html_size == 11);

    CHECK(json_size == 15);
    CHECK(url_size == 13);
    CHECK(html_size == 11);
  }
}
