#include <doctest/doctest.h>
#include <comptime/constexpr_hash.h>

#include <string>
#include <string_view>

using namespace comptime;
using namespace comptime::literals;

// ============================================================================
// fnv1a function tests
// ============================================================================

TEST_SUITE("fnv1a") {
  TEST_CASE("basic string hashing") {
    constexpr auto h1 = fnv1a(std::string_view{"hello"});
    constexpr auto h2 = fnv1a(std::string_view{"hello"});
    constexpr auto h3 = fnv1a(std::string_view{"world"});

    // Same input produces same hash
    static_assert(h1 == h2);
    CHECK(h1 == h2);

    // Different input produces different hash
    static_assert(h1 != h3);
    CHECK(h1 != h3);
  }

  TEST_CASE("empty string") {
    constexpr auto h = fnv1a(std::string_view{""});

    // Empty string should hash to the offset basis
    static_assert(h == detail::fnv1a_constants<std::size_t>::offset_basis);
    CHECK(h == detail::fnv1a_constants<std::size_t>::offset_basis);
  }

  TEST_CASE("single character") {
    constexpr auto ha = fnv1a(std::string_view{"a"});
    constexpr auto hb = fnv1a(std::string_view{"b"});

    static_assert(ha != hb);
    CHECK(ha != hb);
  }

  TEST_CASE("C-string with length") {
    constexpr auto h1 = fnv1a("hello", 5);
    constexpr auto h2 = fnv1a(std::string_view{"hello"});

    static_assert(h1 == h2);
    CHECK(h1 == h2);
  }

  TEST_CASE("null-terminated C-string") {
    constexpr auto h1 = fnv1a("hello");
    constexpr auto h2 = fnv1a(std::string_view{"hello"});

    static_assert(h1 == h2);
    CHECK(h1 == h2);
  }

  TEST_CASE("fixed_string overload") {
    constexpr fixed_string fs{"hello"};
    constexpr auto h1 = fnv1a(fs);
    constexpr auto h2 = fnv1a(std::string_view{"hello"});

    static_assert(h1 == h2);
    CHECK(h1 == h2);
  }

  TEST_CASE("fixed_string with explicit template params") {
    constexpr fixed_string<char, 5> fs{"hello"};
    constexpr auto h1 = fnv1a(fs);
    constexpr auto h2 = fnv1a(std::string_view{"hello"});

    static_assert(h1 == h2);
    CHECK(h1 == h2);
  }

  TEST_CASE("32-bit hash") {
    constexpr auto h32 = fnv1a<char, std::uint32_t>(std::string_view{"hello"});

    // Should fit in 32 bits
    static_assert(h32 <= std::numeric_limits<std::uint32_t>::max());
    CHECK(h32 <= std::numeric_limits<std::uint32_t>::max());
  }

  TEST_CASE("64-bit hash") {
    constexpr auto h64 = fnv1a<char, std::uint64_t>(std::string_view{"hello"});

    // Verify it's computing something
    static_assert(h64 != 0);
    CHECK(h64 != 0);
  }

  TEST_CASE("wide string") {
    constexpr auto h = fnv1a(std::wstring_view{L"hello"});

    static_assert(h != 0);
    CHECK(h != 0);
  }

  TEST_CASE("u8 string") {
    constexpr auto h = fnv1a(std::u8string_view{u8"hello"});

    static_assert(h != 0);
    CHECK(h != 0);
  }

  TEST_CASE("u16 string") {
    constexpr auto h = fnv1a(std::u16string_view{u"hello"});

    static_assert(h != 0);
    CHECK(h != 0);
  }

  TEST_CASE("u32 string") {
    constexpr auto h = fnv1a(std::u32string_view{U"hello"});

    static_assert(h != 0);
    CHECK(h != 0);
  }

  TEST_CASE("order matters") {
    constexpr auto h1 = fnv1a(std::string_view{"ab"});
    constexpr auto h2 = fnv1a(std::string_view{"ba"});

    static_assert(h1 != h2);
    CHECK(h1 != h2);
  }

  TEST_CASE("similar strings produce different hashes") {
    constexpr auto h1 = fnv1a(std::string_view{"hello"});
    constexpr auto h2 = fnv1a(std::string_view{"hello!"});
    constexpr auto h3 = fnv1a(std::string_view{"Hello"});

    static_assert(h1 != h2);
    static_assert(h1 != h3);
    static_assert(h2 != h3);

    CHECK(h1 != h2);
    CHECK(h1 != h3);
    CHECK(h2 != h3);
  }
}

// ============================================================================
// hash_v template variable tests
// ============================================================================

TEST_SUITE("hash_v") {
  TEST_CASE("basic usage") {
    constexpr auto h = hash_v<"hello">;

    static_assert(h != 0);
    CHECK(h != 0);
  }

  TEST_CASE("same string same hash") {
    constexpr auto h1 = hash_v<"test">;
    constexpr auto h2 = hash_v<"test">;

    static_assert(h1 == h2);
    CHECK(h1 == h2);
  }

  TEST_CASE("different strings different hash") {
    constexpr auto h1 = hash_v<"foo">;
    constexpr auto h2 = hash_v<"bar">;

    static_assert(h1 != h2);
    CHECK(h1 != h2);
  }

  TEST_CASE("empty string") {
    constexpr auto h = hash_v<"">;

    static_assert(h == detail::fnv1a_constants<std::size_t>::offset_basis);
    CHECK(h == detail::fnv1a_constants<std::size_t>::offset_basis);
  }

  TEST_CASE("consistent with fnv1a function") {
    constexpr auto h1 = hash_v<"hello">;
    constexpr auto h2 = fnv1a(std::string_view{"hello"});

    static_assert(h1 == h2);
    CHECK(h1 == h2);
  }
}

// ============================================================================
// User-defined literal tests
// ============================================================================

TEST_SUITE("hash literals") {
  TEST_CASE("_hash literal") {
    constexpr auto h = "hello"_hash;

    static_assert(h != 0);
    static_assert(h == fnv1a(std::string_view{"hello"}));

    CHECK(h != 0);
    CHECK(h == fnv1a(std::string_view{"hello"}));
  }

  TEST_CASE("_hash with empty string") {
    constexpr auto h = ""_hash;

    static_assert(h == detail::fnv1a_constants<std::size_t>::offset_basis);
    CHECK(h == detail::fnv1a_constants<std::size_t>::offset_basis);
  }

  TEST_CASE("_hash32 literal") {
    constexpr auto h = "hello"_hash32;

    static_assert(std::is_same_v<decltype(h), const std::uint32_t>);
    static_assert(h == fnv1a<char, std::uint32_t>(std::string_view{"hello"}));

    CHECK(h == fnv1a<char, std::uint32_t>(std::string_view{"hello"}));
  }

  TEST_CASE("_hash64 literal") {
    constexpr auto h = "hello"_hash64;

    static_assert(std::is_same_v<decltype(h), const std::uint64_t>);
    static_assert(h == fnv1a<char, std::uint64_t>(std::string_view{"hello"}));

    CHECK(h == fnv1a<char, std::uint64_t>(std::string_view{"hello"}));
  }

  TEST_CASE("wide string _hash literal") {
    constexpr auto h = L"hello"_hash;

    static_assert(h != 0);
    CHECK(h != 0);
  }

  TEST_CASE("u8 string _hash literal") {
    constexpr auto h = u8"hello"_hash;

    static_assert(h != 0);
    CHECK(h != 0);
  }

  TEST_CASE("u16 string _hash literal") {
    constexpr auto h = u"hello"_hash;

    static_assert(h != 0);
    CHECK(h != 0);
  }

  TEST_CASE("u32 string _hash literal") {
    constexpr auto h = U"hello"_hash;

    static_assert(h != 0);
    CHECK(h != 0);
  }
}

// ============================================================================
// constexpr_hash functor tests
// ============================================================================

TEST_SUITE("constexpr_hash functor") {
  TEST_CASE("string_view hash") {
    constexpr constexpr_hash<std::string_view> hasher;
    constexpr auto h = hasher("hello");

    static_assert(h == fnv1a(std::string_view{"hello"}));
    CHECK(h == fnv1a(std::string_view{"hello"}));
  }

  TEST_CASE("wstring_view hash") {
    constexpr constexpr_hash<std::wstring_view> hasher;
    constexpr auto h = hasher(L"hello");

    static_assert(h == fnv1a(std::wstring_view{L"hello"}));
    CHECK(h == fnv1a(std::wstring_view{L"hello"}));
  }

  TEST_CASE("fixed_string hash") {
    constexpr constexpr_hash<fixed_string<char, 5>> hasher;
    constexpr fixed_string fs{"hello"};
    constexpr auto h = hasher(fs);

    static_assert(h == fnv1a(std::string_view{"hello"}));
    CHECK(h == fnv1a(std::string_view{"hello"}));
  }
}

// ============================================================================
// hash_combine tests
// ============================================================================

TEST_SUITE("hash_combine") {
  TEST_CASE("two hashes") {
    constexpr auto h1 = fnv1a(std::string_view{"key"});
    constexpr auto h2 = fnv1a(std::string_view{"value"});
    constexpr auto combined = hash_combine(h1, h2);

    // Combined hash should be different from both inputs
    static_assert(combined != h1);
    static_assert(combined != h2);

    CHECK(combined != h1);
    CHECK(combined != h2);
  }

  TEST_CASE("order matters") {
    constexpr auto h1 = fnv1a(std::string_view{"a"});
    constexpr auto h2 = fnv1a(std::string_view{"b"});

    constexpr auto c1 = hash_combine(h1, h2);
    constexpr auto c2 = hash_combine(h2, h1);

    static_assert(c1 != c2);
    CHECK(c1 != c2);
  }

  TEST_CASE("variadic hash_combine") {
    constexpr auto h1 = fnv1a(std::string_view{"a"});
    constexpr auto h2 = fnv1a(std::string_view{"b"});
    constexpr auto h3 = fnv1a(std::string_view{"c"});

    constexpr auto combined = hash_combine(h1, h2, h3);

    // Should be different from individual hashes
    static_assert(combined != h1);
    static_assert(combined != h2);
    static_assert(combined != h3);

    CHECK(combined != h1);
    CHECK(combined != h2);
    CHECK(combined != h3);
  }

  TEST_CASE("deterministic") {
    constexpr auto h1 = fnv1a(std::string_view{"key"});
    constexpr auto h2 = fnv1a(std::string_view{"value"});

    constexpr auto c1 = hash_combine(h1, h2);
    constexpr auto c2 = hash_combine(h1, h2);

    static_assert(c1 == c2);
    CHECK(c1 == c2);
  }
}

// ============================================================================
// Practical usage tests
// ============================================================================

TEST_SUITE("practical usage") {
  TEST_CASE("switch on string hash") {
    // This is the primary use case for compile-time hashing
    constexpr auto test_dispatch = [](std::string_view method) {
      switch (fnv1a(method)) {
        case "GET"_hash:
          return 1;
        case "POST"_hash:
          return 2;
        case "PUT"_hash:
          return 3;
        case "DELETE"_hash:
          return 4;
        default:
          return 0;
      }
    };

    CHECK(test_dispatch("GET") == 1);
    CHECK(test_dispatch("POST") == 2);
    CHECK(test_dispatch("PUT") == 3);
    CHECK(test_dispatch("DELETE") == 4);
    CHECK(test_dispatch("PATCH") == 0);
  }

  TEST_CASE("compile-time lookup table") {
    // Verify hashes are unique for common HTTP methods
    static_assert("GET"_hash != "POST"_hash);
    static_assert("GET"_hash != "PUT"_hash);
    static_assert("GET"_hash != "DELETE"_hash);
    static_assert("POST"_hash != "PUT"_hash);
    static_assert("POST"_hash != "DELETE"_hash);
    static_assert("PUT"_hash != "DELETE"_hash);
  }

  TEST_CASE("runtime string with compile-time cases") {
    std::string runtime_method = "POST";

    int result = 0;
    switch (fnv1a(std::string_view{runtime_method})) {
      case "GET"_hash:
        result = 1;
        break;
      case "POST"_hash:
        result = 2;
        break;
      default:
        result = 0;
    }

    CHECK(result == 2);
  }

  TEST_CASE("hash as template parameter") {
    // hash_v can be used as NTTP
    constexpr auto test = []<std::size_t H>() { return H; };

    constexpr auto h = test.template operator()<hash_v<"hello">>();
    static_assert(h == "hello"_hash);
    CHECK(h == "hello"_hash);
  }
}
