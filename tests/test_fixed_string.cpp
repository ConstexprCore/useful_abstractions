#include <doctest/doctest.h>
#include <comptime/fixed_string.h>

#include <string>
#include <type_traits>
#include <vector>

using namespace comptime;
using namespace comptime::literals;

// ============================================================================
// fixed_string tests
// ============================================================================

TEST_SUITE("fixed_string") {
  TEST_CASE("construction from string literal") {
    constexpr fixed_string fs{"hello"};

    static_assert(fs.size() == 5);
    static_assert(fs.length() == 5);
    static_assert(!fs.empty());

    CHECK(fs.view() == "hello");
    CHECK(fs.c_str() == std::string{"hello"});
  }

  TEST_CASE("empty string") {
    constexpr fixed_string fs{""};

    static_assert(fs.size() == 0);
    static_assert(fs.empty());

    CHECK(fs.view() == "");
  }

  TEST_CASE("indexing") {
    constexpr fixed_string fs{"abc"};

    static_assert(fs[0] == 'a');
    static_assert(fs[1] == 'b');
    static_assert(fs[2] == 'c');

    CHECK(fs[0] == 'a');
    CHECK(fs[1] == 'b');
    CHECK(fs[2] == 'c');
  }

  TEST_CASE("iteration") {
    constexpr fixed_string fs{"test"};

    std::string result;
    for (char c : fs) {
      result += c;
    }
    CHECK(result == "test");
  }

  TEST_CASE("equality comparison") {
    constexpr fixed_string a{"hello"};
    constexpr fixed_string b{"hello"};
    constexpr fixed_string c{"world"};
    constexpr fixed_string d{"hi"};

    static_assert(a == b);
    static_assert(!(a == c));
    static_assert(!(a == d));  // Different sizes

    CHECK(a == b);
    CHECK_FALSE(a == c);
    CHECK_FALSE(a == d);
  }

  TEST_CASE("concatenation") {
    constexpr fixed_string a{"hello"};
    constexpr fixed_string b{" world"};
    constexpr auto c = a + b;

    static_assert(c.size() == 11);
    static_assert(c.view() == "hello world");

    CHECK(c.view() == "hello world");
  }

  TEST_CASE("string_view conversion") {
    static constexpr fixed_string fs{"test"};
    constexpr std::string_view sv = fs;

    static_assert(sv == "test");
    CHECK(sv == "test");
  }

  TEST_CASE("use as template parameter") {
    // This is the key feature - NTTP usage
    constexpr auto test = []<fixed_string S>() {
      return S.size();
    };

    static_assert(test.template operator()<"hello">() == 5);
    static_assert(test.template operator()<"">() == 0);
  }
}

// ============================================================================
// static_string tests
// ============================================================================

TEST_SUITE("static_string") {
  TEST_CASE("construction from literal") {
    constexpr static_string<char, 5> ss{"hello"};

    static_assert(ss.size() == 5);
    static_assert(!ss.empty());

    CHECK(ss[0] == 'h');
    CHECK(ss[4] == 'o');
  }

  TEST_CASE("default construction") {
    constexpr static_string<char, 3> ss{};

    static_assert(ss.size() == 3);
    CHECK(ss[0] == '\0');
    CHECK(ss[1] == '\0');
    CHECK(ss[2] == '\0');
  }

  TEST_CASE("construction from range") {
    std::vector<char> vec{'a', 'b', 'c'};
    static_string<char, 3> ss{vec};

    CHECK(ss[0] == 'a');
    CHECK(ss[1] == 'b');
    CHECK(ss[2] == 'c');
  }

  TEST_CASE("data access") {
    constexpr static_string<char, 4> ss{"test"};

    CHECK(ss.data()[0] == 't');
    CHECK(ss.front() == 't');
    CHECK(ss.back() == 't');
  }

  TEST_CASE("iteration") {
    constexpr static_string<char, 3> ss{"abc"};

    std::string result;
    for (char c : ss) {
      result += c;
    }
    CHECK(result == "abc");
  }

  TEST_CASE("span conversion") {
    static constexpr static_string<char, 4> ss{"test"};
    constexpr auto sp = ss.span();

    static_assert(sp.size() == 4);
    CHECK(sp[0] == 't');
    CHECK(sp[3] == 't');
  }

  TEST_CASE("shrink") {
    constexpr static_string<char, 5> ss{"hello"};
    constexpr auto shrunk = ss.shrink<3>();

    static_assert(shrunk.size() == 3);
    static_assert(shrunk[0] == 'h');
    static_assert(shrunk[1] == 'e');
    static_assert(shrunk[2] == 'l');

    CHECK(shrunk[0] == 'h');
    CHECK(shrunk[1] == 'e');
    CHECK(shrunk[2] == 'l');
  }

  TEST_CASE("as_array conversion") {
    constexpr static_string<char, 3> ss{"abc"};
    constexpr auto arr = ss.as_array<unsigned char>();

    static_assert(arr.size() == 3);
    static_assert(arr[0] == 'a');

    CHECK(arr[0] == 'a');
    CHECK(arr[1] == 'b');
    CHECK(arr[2] == 'c');
  }

  TEST_CASE("equality comparison") {
    constexpr static_string<char, 3> a{"abc"};
    constexpr static_string<char, 3> b{"abc"};
    constexpr static_string<char, 3> c{"def"};

    static_assert(a == b);
    static_assert(!(a == c));

    CHECK(a == b);
    CHECK_FALSE(a == c);
  }

  TEST_CASE("endianness tracking") {
    constexpr static_string<char16_t, 3, std::endian::little> le{};
    constexpr static_string<char16_t, 3, std::endian::big> be{};
    constexpr static_string<char16_t, 3, std::endian::native> native{};

    static_assert(le.endianness == std::endian::little);
    static_assert(be.endianness == std::endian::big);
    static_assert(native.endianness == std::endian::native);
  }

  TEST_CASE("UTF-16 static_string") {
    constexpr static_string<char16_t, 5> ss{u"hello"};

    static_assert(ss.size() == 5);
    CHECK(ss[0] == u'h');
    CHECK(ss[4] == u'o');
  }

  TEST_CASE("UTF-32 static_string") {
    constexpr static_string<char32_t, 5> ss{U"hello"};

    static_assert(ss.size() == 5);
    CHECK(ss[0] == U'h');
    CHECK(ss[4] == U'o');
  }
}

// ============================================================================
// Type traits and concepts tests
// ============================================================================

TEST_SUITE("static_string type traits") {
  TEST_CASE("is_static_string") {
    static_assert(is_static_string_v<static_string<char, 5>>);
    static_assert(is_static_string_v<static_string<char16_t, 3, std::endian::little>>);
    static_assert(!is_static_string_v<std::string>);
    static_assert(!is_static_string_v<int>);
  }

  TEST_CASE("any_static_string concept") {
    static_assert(any_static_string<static_string<char, 5>>);
    static_assert(any_static_string<const static_string<char, 5>&>);
    static_assert(!any_static_string<std::string>);
  }

  TEST_CASE("utf8_static_string concept") {
    static_assert(utf8_static_string<static_string<char, 5>>);
    static_assert(utf8_static_string<static_string<char8_t, 5>>);
    static_assert(!utf8_static_string<static_string<char16_t, 5>>);
  }

  TEST_CASE("utf16_static_string concept") {
    static_assert(utf16_static_string<static_string<char16_t, 5>>);
    static_assert(!utf16_static_string<static_string<char, 5>>);
  }

  TEST_CASE("utf32_static_string concept") {
    static_assert(utf32_static_string<static_string<char32_t, 5>>);
    static_assert(!utf32_static_string<static_string<char, 5>>);
  }
}

// ============================================================================
// User-defined literal tests
// ============================================================================

TEST_SUITE("string literals") {
  TEST_CASE("_ss literal") {
    constexpr auto ss = "hello"_ss;

    static_assert(ss.size() == 5);
    static_assert(std::same_as<decltype(ss)::value_type, char>);

    CHECK(ss[0] == 'h');
    CHECK(ss[4] == 'o');
  }

  TEST_CASE("_u16 literal") {
    constexpr auto ss = u"hello"_u16;

    static_assert(ss.size() == 5);
    static_assert(std::same_as<decltype(ss)::value_type, char16_t>);
    static_assert(ss.endianness == std::endian::native);

    CHECK(ss[0] == u'h');
  }

  TEST_CASE("_u16le literal") {
    constexpr auto ss = u"test"_u16le;

    static_assert(ss.size() == 4);
    static_assert(ss.endianness == std::endian::little);
  }

  TEST_CASE("_u16be literal") {
    constexpr auto ss = u"test"_u16be;

    static_assert(ss.size() == 4);
    static_assert(ss.endianness == std::endian::big);
  }

  TEST_CASE("_u32 literal") {
    constexpr auto ss = U"hello"_u32;

    static_assert(ss.size() == 5);
    static_assert(std::same_as<decltype(ss)::value_type, char32_t>);

    CHECK(ss[0] == U'h');
  }
}

// ============================================================================
// string_constant tests
// ============================================================================

TEST_SUITE("string_constant") {
  TEST_CASE("basic usage") {
    using greeting = string_constant<"hello">;

    static_assert(greeting::value == "hello");
    static_assert(greeting::string.size() == 5);

    CHECK(greeting::value == "hello");
  }

  TEST_CASE("string_view conversion") {
    using msg = string_constant<"test">;
    // string_constant uses static storage, so this works
    static_assert(msg::value == "test");
    CHECK(msg::value == "test");
  }

  TEST_CASE("use in template") {
    constexpr auto get_length = []<typename SC>() {
      return SC::value.size();
    };

    using hello = string_constant<"hello">;
    using world = string_constant<"world!">;

    static_assert(get_length.template operator()<hello>() == 5);
    static_assert(get_length.template operator()<world>() == 6);
  }
}

// ============================================================================
// Compile-time evaluation tests
// ============================================================================

TEST_SUITE("constexpr evaluation") {
  TEST_CASE("all fixed_string operations are constexpr") {
    // Use static to allow view() to produce constant expression
    static constexpr fixed_string a{"hello"};
    static constexpr fixed_string b{" world"};
    static constexpr auto c = a + b;

    constexpr auto size = c.size();
    constexpr auto first = c[0];
    constexpr auto view = c.view();

    static_assert(size == 11);
    static_assert(first == 'h');
    static_assert(view == "hello world");
  }

  TEST_CASE("all static_string operations are constexpr") {
    // Use static to allow span() to produce constant expression
    static constexpr static_string<char, 5> ss{"hello"};

    constexpr auto size = ss.size();
    constexpr auto first = ss[0];
    constexpr auto front = ss.front();
    constexpr auto back = ss.back();
    constexpr auto span = ss.span();
    constexpr auto shrunk = ss.shrink<3>();
    constexpr auto arr = ss.as_array<unsigned char>();

    static_assert(size == 5);
    static_assert(first == 'h');
    static_assert(front == 'h');
    static_assert(back == 'o');
    static_assert(span.size() == 5);
    static_assert(shrunk.size() == 3);
    static_assert(arr[0] == 'h');
  }
}
