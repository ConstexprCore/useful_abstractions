#ifndef UA_CONSTEXPR_PTR_H
#define UA_CONSTEXPR_PTR_H

#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace ua {

// ============================================================================
// reinterpret_ptr - Constexpr-safe type-punning pointer wrapper (read-only)
// ============================================================================

/**
 * A pointer wrapper that enables type-punning in constexpr contexts.
 *
 * Standard C++ does not allow reinterpret_cast during constant evaluation.
 * This wrapper provides a constexpr-safe alternative by using static_cast
 * on the values (not pointers) when types have the same size.
 *
 * Use case: Reading char arrays as uint8_t, signed/unsigned conversions,
 * or any scenario where you need to view memory as a different type.
 *
 * @tparam ViewAs  The type to interpret values as when dereferencing
 * @tparam ActualT The actual pointed-to type
 *
 * Requires: sizeof(ViewAs) == sizeof(ActualT)
 *
 * Example:
 *   constexpr char data[] = {'A', 'B', 'C'};
 *   constexpr reinterpret_ptr<uint8_t, char> ptr{data};
 *   static_assert(*ptr == 65);  // 'A' viewed as uint8_t
 */
template <typename ViewAs, typename ActualT>
  requires(sizeof(ViewAs) == sizeof(ActualT))
struct reinterpret_ptr {
  using element_type = ViewAs;
  using pointer = const ActualT*;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::random_access_iterator_tag;

  const ActualT* ptr;

  constexpr explicit reinterpret_ptr(const ActualT* p) noexcept : ptr(p) {}

  // Dereferencing - returns value converted to ViewAs
  [[nodiscard]] constexpr ViewAs operator*() const noexcept {
    return static_cast<ViewAs>(*ptr);
  }

  [[nodiscard]] constexpr ViewAs operator[](difference_type n) const noexcept {
    return static_cast<ViewAs>(ptr[n]);
  }

  // Increment/Decrement
  constexpr reinterpret_ptr& operator++() noexcept {
    ++ptr;
    return *this;
  }

  constexpr reinterpret_ptr operator++(int) noexcept {
    auto old = *this;
    ++ptr;
    return old;
  }

  constexpr reinterpret_ptr& operator--() noexcept {
    --ptr;
    return *this;
  }

  constexpr reinterpret_ptr operator--(int) noexcept {
    auto old = *this;
    --ptr;
    return old;
  }

  // Arithmetic
  constexpr reinterpret_ptr& operator+=(difference_type n) noexcept {
    ptr += n;
    return *this;
  }

  constexpr reinterpret_ptr& operator-=(difference_type n) noexcept {
    ptr -= n;
    return *this;
  }

  [[nodiscard]] constexpr reinterpret_ptr
  operator+(difference_type n) const noexcept {
    return reinterpret_ptr{ptr + n};
  }

  [[nodiscard]] constexpr reinterpret_ptr
  operator-(difference_type n) const noexcept {
    return reinterpret_ptr{ptr - n};
  }

  [[nodiscard]] constexpr difference_type
  operator-(const reinterpret_ptr& other) const noexcept {
    return ptr - other.ptr;
  }

  // Comparison
  [[nodiscard]] constexpr bool
  operator==(const reinterpret_ptr& other) const noexcept {
    return ptr == other.ptr;
  }

  [[nodiscard]] constexpr auto
  operator<=>(const reinterpret_ptr& other) const noexcept {
    return ptr <=> other.ptr;
  }

  // Access to underlying pointer (as const void* for type safety)
  [[nodiscard]] constexpr const void* get() const noexcept { return ptr; }

  // Get underlying raw pointer (use with caution)
  [[nodiscard]] constexpr const ActualT* raw() const noexcept { return ptr; }

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return ptr != nullptr;
  }
};

// Non-member addition for symmetry: n + ptr
template <typename ViewAs, typename ActualT>
[[nodiscard]] constexpr reinterpret_ptr<ViewAs, ActualT>
operator+(std::ptrdiff_t n, reinterpret_ptr<ViewAs, ActualT> p) noexcept {
  return p + n;
}

// ============================================================================
// make_reinterpret_ptr - Factory function
// ============================================================================

/**
 * Create a reinterpret_ptr with deduced actual type.
 *
 * Example:
 *   constexpr char data[] = "hello";
 *   auto ptr = make_reinterpret_ptr<uint8_t>(data);
 */
template <typename ViewAs, typename ActualT>
[[nodiscard]] constexpr reinterpret_ptr<ViewAs, ActualT>
make_reinterpret_ptr(const ActualT* p) noexcept {
  return reinterpret_ptr<ViewAs, ActualT>{p};
}

// ============================================================================
// write_proxy - Helper for writable type-punning
// ============================================================================

/**
 * Proxy returned by writable_ptr's operator* to enable "*ptr = value" syntax.
 */
template <typename WriteAs, typename ActualT>
struct write_proxy {
  ActualT* ptr;

  constexpr explicit write_proxy(ActualT* p) noexcept : ptr(p) {}

  constexpr write_proxy& operator=(WriteAs value) noexcept {
    *ptr = static_cast<ActualT>(value);
    return *this;
  }

  // Allow reading the value too
  [[nodiscard]] constexpr operator WriteAs() const noexcept {
    return static_cast<WriteAs>(*ptr);
  }
};

// ============================================================================
// writable_ptr - Constexpr-safe type-punning pointer wrapper (read-write)
// ============================================================================

/**
 * A writable version of reinterpret_ptr for type-punning writes in constexpr.
 *
 * Enables writing values of type WriteAs into storage of type ActualT,
 * with proper static_cast conversion.
 *
 * @tparam WriteAs The type to write values as
 * @tparam ActualT The actual pointed-to storage type
 *
 * Requires: sizeof(WriteAs) == sizeof(ActualT)
 *
 * Example:
 *   char buffer[4] = {};
 *   writable_ptr<uint8_t, char> ptr{buffer};
 *   *ptr = 255;  // Writes to buffer[0] as char(255)
 */
template <typename WriteAs, typename ActualT>
  requires(sizeof(WriteAs) == sizeof(ActualT))
struct writable_ptr {
  using element_type = WriteAs;
  using pointer = ActualT*;
  using difference_type = std::ptrdiff_t;

  ActualT* ptr;

  constexpr explicit writable_ptr(ActualT* p) noexcept : ptr(p) {}

  // Dereferencing - returns proxy for assignment
  [[nodiscard]] constexpr write_proxy<WriteAs, ActualT>
  operator*() const noexcept {
    return write_proxy<WriteAs, ActualT>{ptr};
  }

  [[nodiscard]] constexpr write_proxy<WriteAs, ActualT>
  operator[](difference_type n) const noexcept {
    return write_proxy<WriteAs, ActualT>{ptr + n};
  }

  // Increment/Decrement
  constexpr writable_ptr& operator++() noexcept {
    ++ptr;
    return *this;
  }

  constexpr writable_ptr operator++(int) noexcept {
    auto old = *this;
    ++ptr;
    return old;
  }

  constexpr writable_ptr& operator--() noexcept {
    --ptr;
    return *this;
  }

  constexpr writable_ptr operator--(int) noexcept {
    auto old = *this;
    --ptr;
    return old;
  }

  // Arithmetic
  constexpr writable_ptr& operator+=(difference_type n) noexcept {
    ptr += n;
    return *this;
  }

  constexpr writable_ptr& operator-=(difference_type n) noexcept {
    ptr -= n;
    return *this;
  }

  [[nodiscard]] constexpr writable_ptr
  operator+(difference_type n) const noexcept {
    return writable_ptr{ptr + n};
  }

  [[nodiscard]] constexpr writable_ptr
  operator-(difference_type n) const noexcept {
    return writable_ptr{ptr - n};
  }

  [[nodiscard]] constexpr difference_type
  operator-(const writable_ptr& other) const noexcept {
    return ptr - other.ptr;
  }

  // Comparison
  [[nodiscard]] constexpr bool
  operator==(const writable_ptr& other) const noexcept {
    return ptr == other.ptr;
  }

  [[nodiscard]] constexpr auto
  operator<=>(const writable_ptr& other) const noexcept {
    return ptr <=> other.ptr;
  }

  // Access to underlying pointer
  [[nodiscard]] constexpr ActualT* get() const noexcept { return ptr; }

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return ptr != nullptr;
  }
};

// ============================================================================
// make_writable_ptr - Factory function
// ============================================================================

/**
 * Create a writable_ptr with deduced actual type.
 *
 * Example:
 *   char buffer[4] = {};
 *   auto ptr = make_writable_ptr<uint8_t>(buffer);
 *   *ptr = 255;
 */
template <typename WriteAs, typename ActualT>
[[nodiscard]] constexpr writable_ptr<WriteAs, ActualT>
make_writable_ptr(ActualT* p) noexcept {
  return writable_ptr<WriteAs, ActualT>{p};
}

// ============================================================================
// Type traits
// ============================================================================

template <typename T>
struct is_reinterpret_ptr : std::false_type {};

template <typename V, typename A>
struct is_reinterpret_ptr<reinterpret_ptr<V, A>> : std::true_type {};

template <typename T>
inline constexpr bool is_reinterpret_ptr_v = is_reinterpret_ptr<T>::value;

template <typename T>
struct is_writable_ptr : std::false_type {};

template <typename W, typename A>
struct is_writable_ptr<writable_ptr<W, A>> : std::true_type {};

template <typename T>
inline constexpr bool is_writable_ptr_v = is_writable_ptr<T>::value;

// ============================================================================
// Concepts
// ============================================================================

template <typename T>
concept any_reinterpret_ptr = is_reinterpret_ptr_v<std::remove_cvref_t<T>>;

template <typename T>
concept any_writable_ptr = is_writable_ptr_v<std::remove_cvref_t<T>>;

}  // namespace ua

#endif  // UA_CONSTEXPR_PTR_H
