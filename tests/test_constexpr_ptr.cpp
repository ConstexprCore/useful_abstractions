#include <doctest/doctest.h>
#include <comptime/constexpr_ptr.h>

#include <array>
#include <cstdint>
#include <type_traits>

using namespace comptime;

// ============================================================================
// reinterpret_ptr tests
// ============================================================================

TEST_SUITE("reinterpret_ptr") {
  TEST_CASE("construction and dereferencing") {
    static constexpr char data[] = {'A', 'B', 'C'};
    constexpr reinterpret_ptr<uint8_t, char> ptr{data};

    static_assert(*ptr == 65);  // 'A' as uint8_t

    CHECK(*ptr == 65);
  }

  TEST_CASE("signed to unsigned conversion") {
    static constexpr signed char data[] = {-1, -2, -3};
    constexpr reinterpret_ptr<uint8_t, signed char> ptr{data};

    // -1 as uint8_t is 255
    static_assert(*ptr == 255);
    CHECK(*ptr == 255);
  }

  TEST_CASE("indexing") {
    static constexpr char data[] = "hello";
    constexpr reinterpret_ptr<uint8_t, char> ptr{data};

    static_assert(ptr[0] == 'h');
    static_assert(ptr[1] == 'e');
    static_assert(ptr[4] == 'o');

    CHECK(ptr[0] == 'h');
    CHECK(ptr[1] == 'e');
    CHECK(ptr[4] == 'o');
  }

  TEST_CASE("increment and decrement") {
    static constexpr char data[] = "abc";
    reinterpret_ptr<uint8_t, char> ptr{data};

    CHECK(*ptr == 'a');

    ++ptr;
    CHECK(*ptr == 'b');

    ptr++;
    CHECK(*ptr == 'c');

    --ptr;
    CHECK(*ptr == 'b');

    ptr--;
    CHECK(*ptr == 'a');
  }

  TEST_CASE("constexpr increment") {
    constexpr auto test = []() {
      const char data[] = "xyz";
      reinterpret_ptr<uint8_t, char> ptr{data};
      ++ptr;
      return *ptr;
    };

    static_assert(test() == 'y');
    CHECK(test() == 'y');
  }

  TEST_CASE("arithmetic operators") {
    static constexpr char data[] = "hello world";
    constexpr reinterpret_ptr<uint8_t, char> ptr{data};

    constexpr auto ptr2 = ptr + 6;
    static_assert(*ptr2 == 'w');
    CHECK(*ptr2 == 'w');

    constexpr auto ptr3 = ptr2 - 6;
    static_assert(*ptr3 == 'h');
    CHECK(*ptr3 == 'h');
  }

  TEST_CASE("pointer difference") {
    static constexpr char data[] = "test";
    constexpr reinterpret_ptr<uint8_t, char> begin{data};
    constexpr reinterpret_ptr<uint8_t, char> end{data + 4};

    static_assert(end - begin == 4);
    CHECK(end - begin == 4);
  }

  TEST_CASE("compound assignment") {
    static constexpr char data[] = "hello";
    reinterpret_ptr<uint8_t, char> ptr{data};

    ptr += 2;
    CHECK(*ptr == 'l');

    ptr -= 1;
    CHECK(*ptr == 'e');
  }

  TEST_CASE("comparison operators") {
    static constexpr char data[] = "test";
    constexpr reinterpret_ptr<uint8_t, char> ptr1{data};
    constexpr reinterpret_ptr<uint8_t, char> ptr2{data};
    constexpr reinterpret_ptr<uint8_t, char> ptr3{data + 1};

    static_assert(ptr1 == ptr2);
    static_assert(ptr1 != ptr3);
    static_assert(ptr1 < ptr3);
    static_assert(ptr3 > ptr1);

    CHECK(ptr1 == ptr2);
    CHECK(ptr1 != ptr3);
    CHECK(ptr1 < ptr3);
  }

  TEST_CASE("get() and raw()") {
    static constexpr char data[] = "test";
    constexpr reinterpret_ptr<uint8_t, char> ptr{data};

    CHECK(ptr.get() == data);
    CHECK(ptr.raw() == data);
  }

  TEST_CASE("bool conversion") {
    constexpr reinterpret_ptr<uint8_t, char> null_ptr{nullptr};
    static constexpr char data[] = "x";
    constexpr reinterpret_ptr<uint8_t, char> valid_ptr{data};

    static_assert(!null_ptr);
    static_assert(static_cast<bool>(valid_ptr));

    CHECK(!null_ptr);
    CHECK(static_cast<bool>(valid_ptr));
  }

  TEST_CASE("non-member addition") {
    static constexpr char data[] = "abcd";
    constexpr reinterpret_ptr<uint8_t, char> ptr{data};

    constexpr auto ptr2 = 2 + ptr;
    static_assert(*ptr2 == 'c');
    CHECK(*ptr2 == 'c');
  }

  TEST_CASE("make_reinterpret_ptr factory") {
    static constexpr char data[] = "test";
    constexpr auto ptr = make_reinterpret_ptr<uint8_t>(data);

    static_assert(*ptr == 't');
    CHECK(*ptr == 't');
  }

  TEST_CASE("16-bit type punning") {
    static constexpr int16_t data[] = {-1, 100, -200};
    constexpr reinterpret_ptr<uint16_t, int16_t> ptr{data};

    static_assert(*ptr == 65535);  // -1 as uint16_t
    static_assert(ptr[1] == 100);

    CHECK(*ptr == 65535);
    CHECK(ptr[1] == 100);
  }

  TEST_CASE("32-bit type punning") {
    static constexpr int32_t data[] = {-1, 42};
    constexpr reinterpret_ptr<uint32_t, int32_t> ptr{data};

    static_assert(*ptr == 4294967295u);  // -1 as uint32_t
    static_assert(ptr[1] == 42);

    CHECK(*ptr == 4294967295u);
  }
}

// ============================================================================
// writable_ptr tests
// ============================================================================

TEST_SUITE("writable_ptr") {
  TEST_CASE("write through proxy") {
    char buffer[4] = {};
    writable_ptr<uint8_t, char> ptr{buffer};

    *ptr = 65;  // Write 'A'

    CHECK(buffer[0] == 'A');
  }

  TEST_CASE("write with indexing") {
    char buffer[4] = {};
    writable_ptr<uint8_t, char> ptr{buffer};

    ptr[0] = 'H';
    ptr[1] = 'i';
    ptr[2] = '!';

    CHECK(buffer[0] == 'H');
    CHECK(buffer[1] == 'i');
    CHECK(buffer[2] == '!');
  }

  TEST_CASE("read through proxy") {
    char buffer[] = "AB";
    writable_ptr<uint8_t, char> ptr{buffer};

    uint8_t val = *ptr;
    CHECK(val == 65);  // 'A'
  }

  TEST_CASE("constexpr write") {
    constexpr auto test = []() {
      char buffer[4] = {};
      writable_ptr<uint8_t, char> ptr{buffer};
      *ptr = 'X';
      ++ptr;
      *ptr = 'Y';
      return buffer[0] == 'X' && buffer[1] == 'Y';
    };

    static_assert(test());
    CHECK(test());
  }

  TEST_CASE("increment and decrement") {
    char buffer[4] = {};
    writable_ptr<uint8_t, char> ptr{buffer};

    *ptr = 'A';
    ++ptr;
    *ptr = 'B';
    ++ptr;
    *ptr = 'C';

    CHECK(buffer[0] == 'A');
    CHECK(buffer[1] == 'B');
    CHECK(buffer[2] == 'C');

    --ptr;
    *ptr = 'X';
    CHECK(buffer[1] == 'X');
  }

  TEST_CASE("arithmetic operators") {
    char buffer[10] = {};
    writable_ptr<uint8_t, char> ptr{buffer};

    auto ptr5 = ptr + 5;
    *ptr5 = 'M';
    CHECK(buffer[5] == 'M');

    auto ptr3 = ptr5 - 2;
    *ptr3 = 'N';
    CHECK(buffer[3] == 'N');
  }

  TEST_CASE("compound assignment") {
    char buffer[10] = {};
    writable_ptr<uint8_t, char> ptr{buffer};

    ptr += 3;
    *ptr = 'X';
    CHECK(buffer[3] == 'X');

    ptr -= 2;
    *ptr = 'Y';
    CHECK(buffer[1] == 'Y');
  }

  TEST_CASE("pointer difference") {
    char buffer[10] = {};
    writable_ptr<uint8_t, char> begin{buffer};
    writable_ptr<uint8_t, char> end{buffer + 10};

    CHECK(end - begin == 10);
  }

  TEST_CASE("comparison operators") {
    char buffer[10] = {};
    writable_ptr<uint8_t, char> ptr1{buffer};
    writable_ptr<uint8_t, char> ptr2{buffer};
    writable_ptr<uint8_t, char> ptr3{buffer + 5};

    CHECK(ptr1 == ptr2);
    CHECK(ptr1 != ptr3);
    CHECK(ptr1 < ptr3);
    CHECK(ptr3 > ptr1);
  }

  TEST_CASE("get() access") {
    char buffer[4] = {};
    writable_ptr<uint8_t, char> ptr{buffer};

    CHECK(ptr.get() == buffer);
  }

  TEST_CASE("bool conversion") {
    writable_ptr<uint8_t, char> null_ptr{nullptr};
    char buffer[] = "x";
    writable_ptr<uint8_t, char> valid_ptr{buffer};

    CHECK(!null_ptr);
    CHECK(static_cast<bool>(valid_ptr));
  }

  TEST_CASE("make_writable_ptr factory") {
    char buffer[4] = {};
    auto ptr = make_writable_ptr<uint8_t>(buffer);

    *ptr = 'Z';
    CHECK(buffer[0] == 'Z');
  }

  TEST_CASE("signed/unsigned write conversion") {
    signed char buffer[4] = {};
    writable_ptr<uint8_t, signed char> ptr{buffer};

    *ptr = 255;  // uint8_t max

    // 255 as signed char is -1
    CHECK(buffer[0] == -1);
  }

  TEST_CASE("16-bit writable type punning") {
    int16_t buffer[4] = {};
    writable_ptr<uint16_t, int16_t> ptr{buffer};

    *ptr = 65535;  // uint16_t max

    // 65535 as int16_t is -1
    CHECK(buffer[0] == -1);
  }
}

// ============================================================================
// Type traits tests
// ============================================================================

TEST_SUITE("constexpr_ptr type traits") {
  TEST_CASE("is_reinterpret_ptr") {
    static_assert(is_reinterpret_ptr_v<reinterpret_ptr<uint8_t, char>>);
    static_assert(!is_reinterpret_ptr_v<writable_ptr<uint8_t, char>>);
    static_assert(!is_reinterpret_ptr_v<int*>);
    static_assert(!is_reinterpret_ptr_v<int>);
  }

  TEST_CASE("is_writable_ptr") {
    static_assert(is_writable_ptr_v<writable_ptr<uint8_t, char>>);
    static_assert(!is_writable_ptr_v<reinterpret_ptr<uint8_t, char>>);
    static_assert(!is_writable_ptr_v<int*>);
    static_assert(!is_writable_ptr_v<int>);
  }

  TEST_CASE("any_reinterpret_ptr concept") {
    static_assert(any_reinterpret_ptr<reinterpret_ptr<uint8_t, char>>);
    static_assert(any_reinterpret_ptr<const reinterpret_ptr<uint8_t, char>&>);
    static_assert(!any_reinterpret_ptr<int*>);
  }

  TEST_CASE("any_writable_ptr concept") {
    static_assert(any_writable_ptr<writable_ptr<uint8_t, char>>);
    static_assert(any_writable_ptr<const writable_ptr<uint8_t, char>&>);
    static_assert(!any_writable_ptr<int*>);
  }
}

// ============================================================================
// Iterator traits tests
// ============================================================================

TEST_SUITE("iterator traits") {
  TEST_CASE("reinterpret_ptr has iterator traits") {
    using ptr_type = reinterpret_ptr<uint8_t, char>;

    static_assert(std::same_as<ptr_type::element_type, uint8_t>);
    static_assert(std::same_as<ptr_type::difference_type, std::ptrdiff_t>);
    static_assert(
        std::same_as<ptr_type::iterator_category, std::random_access_iterator_tag>);
  }

  TEST_CASE("writable_ptr has type aliases") {
    using ptr_type = writable_ptr<uint8_t, char>;

    static_assert(std::same_as<ptr_type::element_type, uint8_t>);
    static_assert(std::same_as<ptr_type::pointer, char*>);
    static_assert(std::same_as<ptr_type::difference_type, std::ptrdiff_t>);
  }
}

// ============================================================================
// Practical usage tests
// ============================================================================

TEST_SUITE("practical usage") {
  TEST_CASE("iterate over char array as uint8_t") {
    constexpr auto sum_bytes = [](const char* data, std::size_t len) {
      uint32_t sum = 0;
      reinterpret_ptr<uint8_t, char> ptr{data};
      for (std::size_t i = 0; i < len; ++i) {
        sum += ptr[i];
      }
      return sum;
    };

    static constexpr char data[] = {1, 2, 3, 4, 5};
    static_assert(sum_bytes(data, 5) == 15);
    CHECK(sum_bytes(data, 5) == 15);
  }

  TEST_CASE("fill buffer with sequential bytes") {
    constexpr auto fill_sequential = []() {
      char buffer[5] = {};
      writable_ptr<uint8_t, char> ptr{buffer};
      for (uint8_t i = 0; i < 5; ++i) {
        *ptr++ = i + 1;
      }
      return buffer[0] == 1 && buffer[1] == 2 && buffer[2] == 3 &&
             buffer[3] == 4 && buffer[4] == 5;
    };

    static_assert(fill_sequential());
    CHECK(fill_sequential());
  }

  TEST_CASE("copy with type conversion") {
    constexpr auto copy_as_unsigned = []() {
      const signed char src[] = {-1, -2, -3, -4};
      unsigned char dst[4] = {};

      reinterpret_ptr<uint8_t, signed char> read_ptr{src};
      writable_ptr<uint8_t, unsigned char> write_ptr{dst};

      for (int i = 0; i < 4; ++i) {
        *write_ptr++ = *read_ptr++;
      }

      return dst[0] == 255 && dst[1] == 254 && dst[2] == 253 && dst[3] == 252;
    };

    static_assert(copy_as_unsigned());
    CHECK(copy_as_unsigned());
  }
}
