#ifndef COMPTIME_FIXED_STRING_H
#define COMPTIME_FIXED_STRING_H

#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <concepts>
#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace comptime {

// ============================================================================
// Concepts for character types
// ============================================================================

namespace detail {

template <typename T>
concept char_type = std::same_as<T, char> || std::same_as<T, wchar_t> ||
                    std::same_as<T, char8_t> || std::same_as<T, char16_t> ||
                    std::same_as<T, char32_t>;

}  // namespace detail

// ============================================================================
// fixed_string - Compile-time string for use as non-type template parameters
// ============================================================================

/**
 * A fixed-size string that can be used as a non-type template parameter (C++20).
 *
 * This enables passing string literals directly to templates:
 *   template <fixed_string Name>
 *   struct named_type { ... };
 *
 *   using my_type = named_type<"hello">;
 *
 * @tparam N The size of the string including the null terminator
 */
template <std::size_t N>
struct fixed_string {
  std::array<char, N> data{};

  constexpr fixed_string() = default;

  constexpr fixed_string(const char (&str)[N]) {
    std::copy_n(str, N, data.data());
  }

  [[nodiscard]] constexpr std::string_view view() const noexcept {
    return {data.data(), N - 1};  // Exclude null terminator
  }

  [[nodiscard]] static constexpr std::size_t size() noexcept { return N - 1; }
  [[nodiscard]] static constexpr std::size_t length() noexcept { return N - 1; }
  [[nodiscard]] static constexpr bool empty() noexcept { return N <= 1; }

  [[nodiscard]] constexpr operator std::string_view() const noexcept {
    return view();
  }

  [[nodiscard]] constexpr char operator[](std::size_t i) const noexcept {
    return data[i];
  }

  [[nodiscard]] constexpr const char* c_str() const noexcept { return data.data(); }

  [[nodiscard]] constexpr const char* begin() const noexcept { return data.data(); }
  [[nodiscard]] constexpr const char* end() const noexcept {
    return data.data() + N - 1;
  }

  template <std::size_t M>
  [[nodiscard]] constexpr bool operator==(const fixed_string<M>& other) const {
    if constexpr (N != M) {
      return false;
    } else {
      return std::equal(data.begin(), data.end(), other.data.begin());
    }
  }

  template <std::size_t M>
  [[nodiscard]] constexpr auto operator<=>(const fixed_string<M>& other) const {
    return view() <=> other.view();
  }

  template <std::size_t M>
  [[nodiscard]] constexpr auto operator+(const fixed_string<M>& other) const {
    fixed_string<N + M - 1> result{};
    std::copy_n(data.data(), N - 1, result.data.data());
    std::copy_n(other.data.data(), M, result.data.data() + N - 1);
    return result;
  }
};

// Deduction guide
template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

// ============================================================================
// static_string - Fixed-size string with explicit character type and endianness
// ============================================================================

/**
 * Compile-time string with configurable character type and endianness.
 *
 * Useful for UTF-8/16/32 conversions and cross-platform binary data.
 *
 * @tparam CharT    Character type (char, char8_t, char16_t, char32_t)
 * @tparam N        Number of characters (excluding any terminator)
 * @tparam Endian   Byte order for multi-byte characters
 */
template <detail::char_type CharT, std::size_t N,
          std::endian Endian = std::endian::native>
struct static_string {
  using value_type = CharT;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = CharT*;
  using const_pointer = const CharT*;
  using reference = CharT&;
  using const_reference = const CharT&;
  using iterator = CharT*;
  using const_iterator = const CharT*;

  static constexpr std::endian endianness = Endian;

  std::array<CharT, N> storage{};

  constexpr static_string() = default;

  // Construct from string literal (array includes null terminator)
  constexpr explicit static_string(const CharT (&str)[N + 1]) {
    std::copy_n(str, N, storage.data());
  }

  // Construct from range
  template <typename Range>
    requires std::ranges::input_range<Range> &&
             std::convertible_to<std::ranges::range_value_t<Range>, CharT>
  constexpr explicit static_string(Range&& range) {
    auto it = std::ranges::begin(range);
    for (std::size_t i = 0; i < N && it != std::ranges::end(range); ++i, ++it) {
      storage[i] = static_cast<CharT>(*it);
    }
  }

  [[nodiscard]] static constexpr std::size_t size() noexcept { return N; }
  [[nodiscard]] static constexpr std::size_t length() noexcept { return N; }
  [[nodiscard]] static constexpr bool empty() noexcept { return N == 0; }

  [[nodiscard]] constexpr CharT* data() noexcept { return storage.data(); }
  [[nodiscard]] constexpr const CharT* data() const noexcept { return storage.data(); }

  [[nodiscard]] constexpr CharT& operator[](std::size_t i) noexcept {
    return storage[i];
  }
  [[nodiscard]] constexpr const CharT& operator[](std::size_t i) const noexcept {
    return storage[i];
  }

  [[nodiscard]] constexpr CharT& front() noexcept { return storage[0]; }
  [[nodiscard]] constexpr const CharT& front() const noexcept {
    return storage[0];
  }

  [[nodiscard]] constexpr CharT& back() noexcept { return storage[N - 1]; }
  [[nodiscard]] constexpr const CharT& back() const noexcept {
    return storage[N - 1];
  }

  [[nodiscard]] constexpr iterator begin() noexcept { return storage.data(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return storage.data();
  }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
    return storage.data();
  }

  [[nodiscard]] constexpr iterator end() noexcept { return storage.data() + N; }
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return storage.data() + N;
  }
  [[nodiscard]] constexpr const_iterator cend() const noexcept {
    return storage.data() + N;
  }

  [[nodiscard]] constexpr std::span<CharT, N> span() noexcept {
    return std::span<CharT, N>(storage.data(), N);
  }
  [[nodiscard]] constexpr std::span<const CharT, N> span() const noexcept {
    return std::span<const CharT, N>(storage.data(), N);
  }

  // Shrink to a smaller size (compile-time only)
  template <std::size_t NewSize>
    requires(NewSize <= N)
  [[nodiscard]] constexpr auto shrink() const noexcept {
    static_string<CharT, NewSize, Endian> result{};
    std::copy_n(storage.data(), NewSize, result.storage.data());
    return result;
  }

  // Convert to array of different type (same size)
  template <typename TargetT>
    requires(sizeof(TargetT) == sizeof(CharT))
  [[nodiscard]] constexpr std::array<TargetT, N> as_array() const noexcept {
    std::array<TargetT, N> result{};
    for (std::size_t i = 0; i < N; ++i) {
      result[i] = static_cast<TargetT>(storage[i]);
    }
    return result;
  }

  template <std::size_t M>
  [[nodiscard]] constexpr bool
  operator==(const static_string<CharT, M, Endian>& other) const {
    if constexpr (N != M) {
      return false;
    } else {
      return std::equal(storage.begin(), storage.end(), other.storage.begin());
    }
  }

  template <std::size_t M>
  [[nodiscard]] constexpr auto
  operator<=>(const static_string<CharT, M, Endian>& other) const {
    return std::lexicographical_compare_three_way(
        storage.begin(), storage.end(),
        other.storage.begin(), other.storage.end());
  }
};

// ============================================================================
// Type traits and concepts for static_string
// ============================================================================

template <typename T>
struct is_static_string : std::false_type {};

template <detail::char_type CharT, std::size_t N, std::endian E>
struct is_static_string<static_string<CharT, N, E>> : std::true_type {};

template <typename T>
inline constexpr bool is_static_string_v = is_static_string<T>::value;

template <typename T>
concept any_static_string = is_static_string_v<std::remove_cvref_t<T>>;

template <typename T>
concept utf8_static_string = any_static_string<T> &&
    (std::same_as<typename std::remove_cvref_t<T>::value_type, char8_t> ||
     std::same_as<typename std::remove_cvref_t<T>::value_type, char>);

template <typename T>
concept utf16_static_string = any_static_string<T> &&
    std::same_as<typename std::remove_cvref_t<T>::value_type, char16_t>;

template <typename T>
concept utf32_static_string = any_static_string<T> &&
    std::same_as<typename std::remove_cvref_t<T>::value_type, char32_t>;

// ============================================================================
// string_literal - Helper for user-defined literal operators
// ============================================================================

namespace detail {

/**
 * Helper to capture string literals with their size for UDL operators.
 * Strips the null terminator from size calculation.
 */
template <char_type CharT, std::size_t N>
struct string_literal {
  std::array<CharT, N - 1> storage{};
  using type = CharT;

  static constexpr std::size_t size() noexcept { return N - 1; }

  constexpr string_literal(const CharT (&str)[N]) {
    static_assert(N >= 1, "String literal must have at least null terminator");
    std::copy_n(str, N - 1, storage.data());
  }
};

}  // namespace detail

// ============================================================================
// User-defined literals
// ============================================================================

inline namespace literals {
inline namespace string_literals {

/**
 * Create a static_string<char, N> from a string literal.
 * Usage: "hello"_ss
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_ss() noexcept {
  return static_string<char, Lit.size()>(Lit.storage);
}

/**
 * Create a static_string<char8_t, N> for UTF-8.
 * Usage: u8"hello"_u8
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u8() noexcept {
  return static_string<char8_t, Lit.size()>(Lit.storage);
}

/**
 * Create a static_string<char16_t, N> for UTF-16 (native endian).
 * Usage: u"hello"_u16
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u16() noexcept {
  return static_string<char16_t, Lit.size()>(Lit.storage);
}

/**
 * Create a static_string<char16_t, N> for UTF-16 little-endian.
 * Usage: u"hello"_u16le
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u16le() noexcept {
  return static_string<char16_t, Lit.size(), std::endian::little>(Lit.storage);
}

/**
 * Create a static_string<char16_t, N> for UTF-16 big-endian.
 * Usage: u"hello"_u16be
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u16be() noexcept {
  return static_string<char16_t, Lit.size(), std::endian::big>(Lit.storage);
}

/**
 * Create a static_string<char32_t, N> for UTF-32.
 * Usage: U"hello"_u32
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u32() noexcept {
  return static_string<char32_t, Lit.size()>(Lit.storage);
}

}  // namespace string_literals
}  // namespace literals

// ============================================================================
// string_constant - Type-level string wrapper
// ============================================================================

/**
 * Wraps a fixed_string as a type, exposing it as a static constexpr value.
 *
 * Useful when you need to pass strings as type parameters:
 *   using name = string_constant<"hello">;
 *   constexpr auto s = name::value;  // "hello"
 */
template <fixed_string Str>
struct string_constant {
  static constexpr std::string_view value = Str.view();
  static constexpr auto string = Str;

  [[nodiscard]] constexpr operator std::string_view() const noexcept {
    return value;
  }
};

}  // namespace comptime

#endif  // COMPTIME_FIXED_STRING_H
