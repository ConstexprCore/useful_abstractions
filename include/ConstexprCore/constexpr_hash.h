#ifndef CONSTEXPRCORE_CONSTEXPR_HASH_H
#define CONSTEXPRCORE_CONSTEXPR_HASH_H

#include <ConstexprCore/fixed_string.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ConstexprCore {

// ============================================================================
// FNV-1a Hash Algorithm - Compile-time string hashing
// ============================================================================

namespace detail {

// FNV-1a constants based on hash size
template <std::size_t BitWidth>
struct fnv1a_params;

template <>
struct fnv1a_params<32> {
  static constexpr std::uint32_t offset_basis = 2166136261u;
  static constexpr std::uint32_t prime = 16777619u;
};

template <>
struct fnv1a_params<64> {
  static constexpr std::uint64_t offset_basis = 14695981039346656037ull;
  static constexpr std::uint64_t prime = 1099511628211ull;
};

// Type-based wrapper that selects params based on sizeof(T)
template <typename SizeT>
struct fnv1a_constants : fnv1a_params<sizeof(SizeT) * 8> {};

}  // namespace detail

/**
 * Compute FNV-1a hash at compile-time.
 *
 * FNV-1a is a non-cryptographic hash function known for:
 * - Simple implementation
 * - Good distribution
 * - Fast computation
 * - Works well for hash tables
 *
 * @param str The string view to hash
 * @return The hash value
 *
 * Example:
 *   constexpr auto h = fnv1a("hello");
 *   static_assert(h == fnv1a("hello"));  // Same input -> same hash
 */
template <typename CharT = char, typename SizeT = std::size_t>
[[nodiscard]] constexpr SizeT
fnv1a(std::basic_string_view<CharT> str) noexcept {
  using constants = detail::fnv1a_constants<SizeT>;

  SizeT hash = constants::offset_basis;
  for (CharT c : str) {
    // Cast to unsigned byte for consistent hashing
    auto byte = static_cast<unsigned char>(c);
    hash ^= static_cast<SizeT>(byte);
    hash *= constants::prime;
  }
  return hash;
}

/**
 * Overload for C-style string with known length.
 */
template <typename CharT = char, typename SizeT = std::size_t>
[[nodiscard]] constexpr SizeT fnv1a(const CharT* str,
                                    std::size_t len) noexcept {
  return fnv1a<CharT, SizeT>(std::basic_string_view<CharT>{str, len});
}

/**
 * Overload for null-terminated C-style strings.
 * Note: Requires iterating to find length, prefer string_view overload.
 */
template <typename CharT = char, typename SizeT = std::size_t>
[[nodiscard]] constexpr SizeT fnv1a(const CharT* str) noexcept {
  using constants = detail::fnv1a_constants<SizeT>;

  SizeT hash = constants::offset_basis;
  while (*str) {
    auto byte = static_cast<unsigned char>(*str);
    hash ^= static_cast<SizeT>(byte);
    hash *= constants::prime;
    ++str;
  }
  return hash;
}

/**
 * Overload for fixed_string.
 */
template <detail::char_type CharT, std::size_t N, std::endian E,
          typename SizeT = std::size_t>
[[nodiscard]] constexpr SizeT
fnv1a(const fixed_string<CharT, N, E>& str) noexcept {
  return fnv1a<CharT, SizeT>(std::basic_string_view<CharT>{str.data.data(), N});
}

// ============================================================================
// Hash template variable - hash_v<"string">
// ============================================================================

/**
 * Compile-time hash value for a fixed_string.
 *
 * Example:
 *   constexpr auto h = hash_v<"hello">;
 *   static_assert(h != hash_v<"world">);
 */
template <fixed_string S>
inline constexpr std::size_t hash_v = fnv1a(S.view());

// ============================================================================
// User-defined literals
// ============================================================================

inline namespace literals {
inline namespace hash_literals {

/**
 * User-defined literal for compile-time string hashing.
 *
 * Example:
 *   using namespace comptime::literals;
 *
 *   constexpr auto h = "hello"_hash;
 *
 *   // Use in switch statements:
 *   switch (fnv1a(runtime_string)) {
 *     case "GET"_hash:  handle_get();  break;
 *     case "POST"_hash: handle_post(); break;
 *     case "PUT"_hash:  handle_put();  break;
 *   }
 */
[[nodiscard]] consteval std::size_t operator""_hash(const char* str,
                                                    std::size_t len) noexcept {
  return fnv1a(std::string_view{str, len});
}

/**
 * Wide string literal variant.
 */
[[nodiscard]] consteval std::size_t operator""_hash(const wchar_t* str,
                                                    std::size_t len) noexcept {
  return fnv1a(std::wstring_view{str, len});
}

/**
 * char8_t string literal variant (UTF-8).
 */
[[nodiscard]] consteval std::size_t operator""_hash(const char8_t* str,
                                                    std::size_t len) noexcept {
  return fnv1a(std::u8string_view{str, len});
}

/**
 * char16_t string literal variant (UTF-16).
 */
[[nodiscard]] consteval std::size_t operator""_hash(const char16_t* str,
                                                    std::size_t len) noexcept {
  return fnv1a(std::u16string_view{str, len});
}

/**
 * char32_t string literal variant (UTF-32).
 */
[[nodiscard]] consteval std::size_t operator""_hash(const char32_t* str,
                                                    std::size_t len) noexcept {
  return fnv1a(std::u32string_view{str, len});
}

/**
 * 32-bit hash variant for when you need smaller hash values.
 */
[[nodiscard]] consteval std::uint32_t operator""_hash32(const char* str,
                                                        std::size_t len) noexcept {
  return fnv1a<char, std::uint32_t>(std::string_view{str, len});
}

/**
 * 64-bit hash variant for explicit 64-bit hashing.
 */
[[nodiscard]] consteval std::uint64_t operator""_hash64(const char* str,
                                                        std::size_t len) noexcept {
  return fnv1a<char, std::uint64_t>(std::string_view{str, len});
}

}  // namespace hash_literals
}  // namespace literals

// ============================================================================
// Constexpr hash functor - for use in compile-time containers
// ============================================================================

/**
 * A constexpr hash functor compatible with std::hash interface.
 *
 * Can be used as a drop-in replacement for std::hash in constexpr contexts.
 *
 * Example:
 *   constexpr constexpr_hash<std::string_view> hasher;
 *   constexpr auto h = hasher("hello");
 */
template <typename T>
struct constexpr_hash {
  using argument_type = T;
  using result_type = std::size_t;

  [[nodiscard]] constexpr std::size_t operator()(const T& str) const noexcept {
    return fnv1a(str);
  }
};

// ============================================================================
// Hash combine utility
// ============================================================================

/**
 * Combine two hash values into one.
 *
 * Useful for hashing composite types or multiple values.
 *
 * Example:
 *   constexpr auto h1 = fnv1a("key");
 *   constexpr auto h2 = fnv1a("value");
 *   constexpr auto combined = hash_combine(h1, h2);
 */
[[nodiscard]] constexpr std::size_t hash_combine(std::size_t h1,
                                                 std::size_t h2) noexcept {
  // Boost-style hash combine
  return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

/**
 * Variadic hash combine.
 */
template <typename... Hashes>
[[nodiscard]] constexpr std::size_t hash_combine(std::size_t h1,
                                                 std::size_t h2,
                                                 Hashes... rest) noexcept {
  return hash_combine(hash_combine(h1, h2), rest...);
}

}  // namespace ConstexprCore

#endif  // CONSTEXPRCORE_CONSTEXPR_HASH_H
