#ifndef COMPTIME_TYPE_NAME_H
#define COMPTIME_TYPE_NAME_H

#include <comptime/constexpr_hash.h>

#include <array>
#include <cstddef>
#include <string_view>

namespace comptime {

// ============================================================================
// Compile-time type name extraction
// ============================================================================

namespace detail {

/**
 * Extract type name from compiler's function signature.
 *
 * Different compilers embed type information differently:
 * - GCC/Clang: Uses __PRETTY_FUNCTION__
 * - MSVC: Uses __FUNCSIG__
 *
 * This function parses the signature to extract just the type name.
 */
template <typename T>
consteval std::string_view type_name_impl() noexcept {
#if defined(__clang__)
  // Clang format: "std::string_view comptime::detail::type_name_impl() [T = int]"
  constexpr std::string_view function = __PRETTY_FUNCTION__;
  constexpr std::string_view prefix = "[T = ";
  constexpr std::string_view suffix = "]";
#elif defined(__GNUC__)
  // GCC format: "consteval std::string_view comptime::detail::type_name_impl() [with T = int; ...]"
  constexpr std::string_view function = __PRETTY_FUNCTION__;
  constexpr std::string_view prefix = "[with T = ";
  constexpr std::string_view suffix = ";";  // GCC may have "; ..." after
#elif defined(_MSC_VER)
  // MSVC format: "class std::basic_string_view<char,...> __cdecl comptime::detail::type_name_impl<int>(void)"
  constexpr std::string_view function = __FUNCSIG__;
  constexpr std::string_view prefix = "type_name_impl<";
  constexpr std::string_view suffix = ">(void)";
#else
#error "Unsupported compiler for type_name"
#endif

  // Find the start of the type name
  constexpr std::size_t start = function.find(prefix);
  static_assert(start != std::string_view::npos, "Could not find type prefix");

  constexpr std::size_t type_start = start + prefix.size();

  // Find the end of the type name
  // For GCC, we need to handle both ';' and ']' since the format can vary
#if defined(__GNUC__) && !defined(__clang__)
  // GCC might use ';' or just ']' depending on context
  constexpr std::size_t semi_pos = function.find(';', type_start);
  constexpr std::size_t bracket_pos = function.find(']', type_start);
  constexpr std::size_t type_end =
      (semi_pos != std::string_view::npos && semi_pos < bracket_pos)
          ? semi_pos
          : bracket_pos;
#else
  constexpr std::size_t type_end = function.rfind(suffix);
#endif

  static_assert(type_end != std::string_view::npos, "Could not find type suffix");
  static_assert(type_end > type_start, "Invalid type name bounds");

  return function.substr(type_start, type_end - type_start);
}

}  // namespace detail

// ============================================================================
// Public API
// ============================================================================

/**
 * Get the name of a type as a compile-time string_view.
 *
 * This uses compiler intrinsics to extract human-readable type names
 * without requiring RTTI.
 *
 * @tparam T The type to get the name of
 * @return A string_view containing the type name
 *
 * Example:
 *   constexpr auto name = type_name<int>();  // "int"
 *   constexpr auto name2 = type_name<std::vector<int>>();  // "std::vector<int, ...>"
 */
template <typename T>
[[nodiscard]] consteval std::string_view type_name() noexcept {
  return detail::type_name_impl<T>();
}

/**
 * Get the type name as a fixed-size array for use as NTTP.
 *
 * Since string_view can't always be used as NTTP (depends on storage),
 * this returns the type name as a std::array that can be used anywhere.
 *
 * @tparam T The type to get the name of
 * @return A std::array containing the type name characters
 */
template <typename T>
[[nodiscard]] consteval auto type_name_array() noexcept {
  constexpr auto name = type_name<T>();
  std::array<char, name.size()> result{};
  for (std::size_t i = 0; i < name.size(); ++i) {
    result[i] = name[i];
  }
  return result;
}

/**
 * Compile-time type ID based on hash of type name.
 *
 * Provides a unique (within hash collision probability) identifier
 * for any type, usable at compile-time.
 *
 * @tparam T The type to get an ID for
 *
 * Example:
 *   static_assert(type_id<int> != type_id<float>);
 *   static_assert(type_id<int> == type_id<int>);
 */
template <typename T>
inline constexpr std::size_t type_id = fnv1a(type_name<T>());

/**
 * Check if two types have the same type_id.
 *
 * This is essentially a constexpr version of checking typeid equality.
 */
template <typename T, typename U>
inline constexpr bool same_type_id = (type_id<T> == type_id<U>);

// ============================================================================
// Type name comparison utilities
// ============================================================================

/**
 * Compare type names at compile-time.
 */
template <typename T, typename U>
[[nodiscard]] consteval bool type_names_equal() noexcept {
  return type_name<T>() == type_name<U>();
}

/**
 * Check if type name contains a substring.
 *
 * Useful for template metaprogramming based on type characteristics.
 *
 * Example:
 *   static_assert(type_name_contains<std::vector<int>>("vector"));
 */
template <typename T>
[[nodiscard]] consteval bool type_name_contains(std::string_view substr) noexcept {
  return type_name<T>().find(substr) != std::string_view::npos;
}

/**
 * Check if type name starts with a prefix.
 */
template <typename T>
[[nodiscard]] consteval bool type_name_starts_with(std::string_view prefix) noexcept {
  auto name = type_name<T>();
  return name.size() >= prefix.size() &&
         name.substr(0, prefix.size()) == prefix;
}

/**
 * Check if type name ends with a suffix.
 */
template <typename T>
[[nodiscard]] consteval bool type_name_ends_with(std::string_view suffix) noexcept {
  auto name = type_name<T>();
  return name.size() >= suffix.size() &&
         name.substr(name.size() - suffix.size()) == suffix;
}

// ============================================================================
// Type registration helper
// ============================================================================

/**
 * A type wrapper that carries its name as a compile-time constant.
 *
 * Useful for type registration patterns.
 *
 * Example:
 *   using IntType = named_type<int>;
 *   constexpr auto name = IntType::name;  // "int"
 */
template <typename T>
struct named_type {
  using type = T;
  static constexpr std::string_view name = type_name<T>();
  static constexpr std::size_t id = type_id<T>;
};

}  // namespace comptime

#endif  // COMPTIME_TYPE_NAME_H
