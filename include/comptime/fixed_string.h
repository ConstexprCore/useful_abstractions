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
// fixed_string - Unified compile-time string type
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
 * @tparam CharT  Character type (char, char8_t, char16_t, char32_t, wchar_t)
 * @tparam N      Number of characters (excluding null terminator)
 * @tparam Endian Byte order for multi-byte characters (default: native)
 */
template <detail::char_type CharT, std::size_t N,
          std::endian Endian = std::endian::native>
struct fixed_string {
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

  std::array<CharT, N> data{};

  constexpr fixed_string() = default;

  // Construct from string literal (array includes null terminator)
  constexpr fixed_string(const CharT (&str)[N + 1]) {
    std::copy_n(str, N, data.data());
  }

  // Construct from another array (for internal use, e.g., from literals)
  constexpr explicit fixed_string(const std::array<CharT, N>& arr) : data(arr) {}

  // Construct from range
  template <typename Range>
    requires std::ranges::input_range<Range> &&
             std::convertible_to<std::ranges::range_value_t<Range>, CharT>
  constexpr explicit fixed_string(Range&& range) {
    auto it = std::ranges::begin(range);
    for (std::size_t i = 0; i < N && it != std::ranges::end(range); ++i, ++it) {
      data[i] = static_cast<CharT>(*it);
    }
  }

  [[nodiscard]] static constexpr std::size_t size() noexcept { return N; }
  [[nodiscard]] static constexpr std::size_t length() noexcept { return N; }
  [[nodiscard]] static constexpr bool empty() noexcept { return N == 0; }

  // String view conversion (only for char types that support string_view)
  [[nodiscard]] constexpr std::basic_string_view<CharT> view() const noexcept
    requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t> ||
             std::same_as<CharT, char8_t> || std::same_as<CharT, char16_t> ||
             std::same_as<CharT, char32_t>
  {
    return {data.data(), N};
  }

  [[nodiscard]] constexpr operator std::basic_string_view<CharT>() const noexcept
    requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t> ||
             std::same_as<CharT, char8_t> || std::same_as<CharT, char16_t> ||
             std::same_as<CharT, char32_t>
  {
    return view();
  }

  [[nodiscard]] constexpr CharT* begin() noexcept { return data.data(); }
  [[nodiscard]] constexpr const CharT* begin() const noexcept { return data.data(); }
  [[nodiscard]] constexpr const CharT* cbegin() const noexcept { return data.data(); }

  [[nodiscard]] constexpr CharT* end() noexcept { return data.data() + N; }
  [[nodiscard]] constexpr const CharT* end() const noexcept { return data.data() + N; }
  [[nodiscard]] constexpr const CharT* cend() const noexcept { return data.data() + N; }

  [[nodiscard]] constexpr CharT& operator[](std::size_t i) noexcept {
    return data[i];
  }
  [[nodiscard]] constexpr const CharT& operator[](std::size_t i) const noexcept {
    return data[i];
  }

  [[nodiscard]] constexpr CharT& front() noexcept { return data[0]; }
  [[nodiscard]] constexpr const CharT& front() const noexcept { return data[0]; }

  [[nodiscard]] constexpr CharT& back() noexcept { return data[N - 1]; }
  [[nodiscard]] constexpr const CharT& back() const noexcept { return data[N - 1]; }

  [[nodiscard]] constexpr std::span<CharT, N> span() noexcept {
    return std::span<CharT, N>(data.data(), N);
  }
  [[nodiscard]] constexpr std::span<const CharT, N> span() const noexcept {
    return std::span<const CharT, N>(data.data(), N);
  }

  // Shrink to a smaller size (compile-time only)
  template <std::size_t NewSize>
    requires(NewSize <= N)
  [[nodiscard]] constexpr auto shrink() const noexcept {
    fixed_string<CharT, NewSize, Endian> result{};
    std::copy_n(data.data(), NewSize, result.data.data());
    return result;
  }

  // Convert to array of different type (same size)
  template <typename TargetT>
    requires(sizeof(TargetT) == sizeof(CharT))
  [[nodiscard]] constexpr std::array<TargetT, N> as_array() const noexcept {
    std::array<TargetT, N> result{};
    for (std::size_t i = 0; i < N; ++i) {
      result[i] = static_cast<TargetT>(data[i]);
    }
    return result;
  }

  // Concatenation (for same CharT and Endian)
  template <std::size_t M>
  [[nodiscard]] constexpr auto operator+(const fixed_string<CharT, M, Endian>& other) const {
    fixed_string<CharT, N + M, Endian> result{};
    std::copy_n(data.data(), N, result.data.data());
    std::copy_n(other.data.data(), M, result.data.data() + N);
    return result;
  }

  template <std::size_t M>
  [[nodiscard]] constexpr bool
  operator==(const fixed_string<CharT, M, Endian>& other) const {
    if constexpr (N != M) {
      return false;
    } else {
      return std::equal(data.begin(), data.end(), other.data.begin());
    }
  }

  template <std::size_t M>
  [[nodiscard]] constexpr auto
  operator<=>(const fixed_string<CharT, M, Endian>& other) const {
    return std::lexicographical_compare_three_way(
        data.begin(), data.end(),
        other.data.begin(), other.data.end());
  }
};

// Deduction guides
template <detail::char_type CharT, std::size_t N>
fixed_string(const CharT (&)[N]) -> fixed_string<CharT, N - 1>;

// ============================================================================
// Type traits and concepts for fixed_string
// ============================================================================

template <typename T>
struct is_fixed_string : std::false_type {};

template <detail::char_type CharT, std::size_t N, std::endian E>
struct is_fixed_string<fixed_string<CharT, N, E>> : std::true_type {};

template <typename T>
inline constexpr bool is_fixed_string_v = is_fixed_string<T>::value;

template <typename T>
concept any_fixed_string = is_fixed_string_v<std::remove_cvref_t<T>>;

template <typename T>
concept utf8_fixed_string = any_fixed_string<T> &&
    (std::same_as<typename std::remove_cvref_t<T>::value_type, char8_t> ||
     std::same_as<typename std::remove_cvref_t<T>::value_type, char>);

template <typename T>
concept utf16_fixed_string = any_fixed_string<T> &&
    std::same_as<typename std::remove_cvref_t<T>::value_type, char16_t>;

template <typename T>
concept utf32_fixed_string = any_fixed_string<T> &&
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
 * Create a fixed_string<char, N> from a string literal.
 * Usage: "hello"_fs
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_fs() noexcept {
  return fixed_string<char, Lit.size()>(Lit.storage);
}

/**
 * Create a fixed_string<char8_t, N> for UTF-8.
 * Usage: u8"hello"_u8
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u8() noexcept {
  return fixed_string<char8_t, Lit.size()>(Lit.storage);
}

/**
 * Create a fixed_string<char16_t, N> for UTF-16 (native endian).
 * Usage: u"hello"_u16
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u16() noexcept {
  return fixed_string<char16_t, Lit.size()>(Lit.storage);
}

/**
 * Create a fixed_string<char16_t, N> for UTF-16 little-endian.
 * Usage: u"hello"_u16le
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u16le() noexcept {
  return fixed_string<char16_t, Lit.size(), std::endian::little>(Lit.storage);
}

/**
 * Create a fixed_string<char16_t, N> for UTF-16 big-endian.
 * Usage: u"hello"_u16be
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u16be() noexcept {
  return fixed_string<char16_t, Lit.size(), std::endian::big>(Lit.storage);
}

/**
 * Create a fixed_string<char32_t, N> for UTF-32.
 * Usage: U"hello"_u32
 */
template <detail::string_literal Lit>
[[nodiscard]] consteval auto operator""_u32() noexcept {
  return fixed_string<char32_t, Lit.size()>(Lit.storage);
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
