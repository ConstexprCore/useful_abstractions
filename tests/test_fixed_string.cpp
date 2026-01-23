#include <doctest/doctest.h>
#include <constexprcore/fixed_string.h>

#include <string>
#include <type_traits>
#include <vector>

using namespace ConstexprCore;
using namespace ConstexprCore::literals;

// ============================================================================
// fixed_string basic tests
// ============================================================================

TEST_SUITE("fixed_string basics") {
  TEST_CASE("construction from string literal") {
    constexpr fixed_string fs{"hello"};

    static_assert(fs.size() == 5);
    static_assert(fs.length() == 5);
    static_assert(!fs.empty());

    CHECK(fs.view() == "hello");
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

  TEST_CASE("three-way comparison") {
    constexpr fixed_string a{"abc"};
    constexpr fixed_string b{"abd"};
    constexpr fixed_string c{"abc"};

    static_assert((a <=> c) == std::strong_ordering::equal);
    static_assert((a <=> b) == std::strong_ordering::less);
    static_assert((b <=> a) == std::strong_ordering::greater);

    CHECK((a <=> c) == std::strong_ordering::equal);
    CHECK((a <=> b) == std::strong_ordering::less);
    CHECK((b <=> a) == std::strong_ordering::greater);
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
// fixed_string extended features (formerly static_string)
// ============================================================================

TEST_SUITE("fixed_string extended") {
  TEST_CASE("default construction") {
    constexpr fixed_string<char, 3> fs{};

    static_assert(fs.size() == 3);
    CHECK(fs[0] == '\0');
    CHECK(fs[1] == '\0');
    CHECK(fs[2] == '\0');
  }

  TEST_CASE("construction from range") {
    std::vector<char> vec{'a', 'b', 'c'};
    fixed_string<char, 3> fs{vec};

    CHECK(fs[0] == 'a');
    CHECK(fs[1] == 'b');
    CHECK(fs[2] == 'c');
  }

  TEST_CASE("data access") {
    constexpr fixed_string fs{"test"};

    CHECK(fs.data[0] == 't');
    CHECK(fs.front() == 't');
    CHECK(fs.back() == 't');
  }

  TEST_CASE("span conversion") {
    static constexpr fixed_string fs{"test"};
    constexpr auto sp = fs.span();

    static_assert(sp.size() == 4);
    CHECK(sp[0] == 't');
    CHECK(sp[3] == 't');
  }

  TEST_CASE("shrink") {
    constexpr fixed_string fs{"hello"};
    constexpr auto shrunk = fs.shrink<3>();

    static_assert(shrunk.size() == 3);
    static_assert(shrunk[0] == 'h');
    static_assert(shrunk[1] == 'e');
    static_assert(shrunk[2] == 'l');

    CHECK(shrunk[0] == 'h');
    CHECK(shrunk[1] == 'e');
    CHECK(shrunk[2] == 'l');
  }

  TEST_CASE("as_array conversion") {
    constexpr fixed_string fs{"abc"};
    constexpr auto arr = fs.as_array<unsigned char>();

    static_assert(arr.size() == 3);
    static_assert(arr[0] == 'a');

    CHECK(arr[0] == 'a');
    CHECK(arr[1] == 'b');
    CHECK(arr[2] == 'c');
  }

  TEST_CASE("endianness tracking") {
    constexpr fixed_string<char16_t, 3, std::endian::little> le{};
    constexpr fixed_string<char16_t, 3, std::endian::big> be{};
    constexpr fixed_string<char16_t, 3, std::endian::native> native{};

    static_assert(le.endianness == std::endian::little);
    static_assert(be.endianness == std::endian::big);
    static_assert(native.endianness == std::endian::native);
  }

  TEST_CASE("UTF-16 fixed_string") {
    constexpr fixed_string fs{u"hello"};

    static_assert(fs.size() == 5);
    CHECK(fs[0] == u'h');
    CHECK(fs[4] == u'o');
  }

  TEST_CASE("UTF-32 fixed_string") {
    constexpr fixed_string fs{U"hello"};

    static_assert(fs.size() == 5);
    CHECK(fs[0] == U'h');
    CHECK(fs[4] == U'o');
  }
}

// ============================================================================
// Type traits and concepts tests
// ============================================================================

TEST_SUITE("fixed_string type traits") {
  TEST_CASE("is_fixed_string") {
    static_assert(is_fixed_string_v<fixed_string<char, 5>>);
    static_assert(is_fixed_string_v<fixed_string<char16_t, 3, std::endian::little>>);
    static_assert(!is_fixed_string_v<std::string>);
    static_assert(!is_fixed_string_v<int>);
  }

  TEST_CASE("any_fixed_string concept") {
    static_assert(any_fixed_string<fixed_string<char, 5>>);
    static_assert(any_fixed_string<const fixed_string<char, 5>&>);
    static_assert(!any_fixed_string<std::string>);
  }

  TEST_CASE("utf8_fixed_string concept") {
    static_assert(utf8_fixed_string<fixed_string<char, 5>>);
    static_assert(utf8_fixed_string<fixed_string<char8_t, 5>>);
    static_assert(!utf8_fixed_string<fixed_string<char16_t, 5>>);
  }

  TEST_CASE("utf16_fixed_string concept") {
    static_assert(utf16_fixed_string<fixed_string<char16_t, 5>>);
    static_assert(!utf16_fixed_string<fixed_string<char, 5>>);
  }

  TEST_CASE("utf32_fixed_string concept") {
    static_assert(utf32_fixed_string<fixed_string<char32_t, 5>>);
    static_assert(!utf32_fixed_string<fixed_string<char, 5>>);
  }
}

// ============================================================================
// User-defined literal tests
// ============================================================================

TEST_SUITE("string literals") {
  TEST_CASE("_fs literal") {
    constexpr auto fs = "hello"_fs;

    static_assert(fs.size() == 5);
    static_assert(std::same_as<decltype(fs)::value_type, char>);

    CHECK(fs[0] == 'h');
    CHECK(fs[4] == 'o');
  }

  TEST_CASE("_u16 literal") {
    constexpr auto fs = u"hello"_u16;

    static_assert(fs.size() == 5);
    static_assert(std::same_as<decltype(fs)::value_type, char16_t>);
    static_assert(fs.endianness == std::endian::native);

    CHECK(fs[0] == u'h');
  }

  TEST_CASE("_u16le literal") {
    constexpr auto fs = u"test"_u16le;

    static_assert(fs.size() == 4);
    static_assert(fs.endianness == std::endian::little);
  }

  TEST_CASE("_u16be literal") {
    constexpr auto fs = u"test"_u16be;

    static_assert(fs.size() == 4);
    static_assert(fs.endianness == std::endian::big);
  }

  TEST_CASE("_u32 literal") {
    constexpr auto fs = U"hello"_u32;

    static_assert(fs.size() == 5);
    static_assert(std::same_as<decltype(fs)::value_type, char32_t>);

    CHECK(fs[0] == U'h');
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

  TEST_CASE("all extended operations are constexpr") {
    // Use static to allow span() to produce constant expression
    static constexpr fixed_string fs{"hello"};

    constexpr auto size = fs.size();
    constexpr auto first = fs[0];
    constexpr auto front = fs.front();
    constexpr auto back = fs.back();
    constexpr auto span = fs.span();
    constexpr auto shrunk = fs.shrink<3>();
    constexpr auto arr = fs.as_array<unsigned char>();

    static_assert(size == 5);
    static_assert(first == 'h');
    static_assert(front == 'h');
    static_assert(back == 'o');
    static_assert(span.size() == 5);
    static_assert(shrunk.size() == 3);
    static_assert(arr[0] == 'h');
  }
}
