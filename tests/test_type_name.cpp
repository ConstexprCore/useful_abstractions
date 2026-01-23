#include <doctest/doctest.h>
#include <constexprcore/type_name.h>

#include <string>
#include <vector>
#include <map>

using namespace ConstexprCore;

// ============================================================================
// type_name tests
// ============================================================================

TEST_SUITE("type_name") {
  TEST_CASE("fundamental types") {
    constexpr auto int_name = type_name<int>();
    constexpr auto float_name = type_name<float>();
    constexpr auto double_name = type_name<double>();
    constexpr auto char_name = type_name<char>();
    constexpr auto bool_name = type_name<bool>();

    // Check they're non-empty
    static_assert(!int_name.empty());
    static_assert(!float_name.empty());
    static_assert(!double_name.empty());
    static_assert(!char_name.empty());
    static_assert(!bool_name.empty());

    CHECK(!int_name.empty());
    CHECK(!float_name.empty());
    CHECK(!double_name.empty());
    CHECK(!char_name.empty());
    CHECK(!bool_name.empty());

    // Check they contain expected substrings
    CHECK(int_name.find("int") != std::string_view::npos);
    CHECK(float_name.find("float") != std::string_view::npos);
    CHECK(double_name.find("double") != std::string_view::npos);
    CHECK(char_name.find("char") != std::string_view::npos);
    CHECK(bool_name.find("bool") != std::string_view::npos);
  }

  TEST_CASE("different types have different names") {
    static_assert(type_name<int>() != type_name<float>());
    static_assert(type_name<int>() != type_name<double>());
    static_assert(type_name<int>() != type_name<long>());

    CHECK(type_name<int>() != type_name<float>());
    CHECK(type_name<int>() != type_name<double>());
  }

  TEST_CASE("same type has same name") {
    static_assert(type_name<int>() == type_name<int>());
    static_assert(type_name<double>() == type_name<double>());

    CHECK(type_name<int>() == type_name<int>());
    CHECK(type_name<double>() == type_name<double>());
  }

  TEST_CASE("pointer types") {
    constexpr auto ptr_name = type_name<int*>();
    constexpr auto ptr_ptr_name = type_name<int**>();

    static_assert(!ptr_name.empty());
    static_assert(ptr_name != type_name<int>());
    static_assert(ptr_name != ptr_ptr_name);

    CHECK(!ptr_name.empty());
    CHECK(ptr_name != type_name<int>());
  }

  TEST_CASE("reference types") {
    constexpr auto ref_name = type_name<int&>();
    constexpr auto rref_name = type_name<int&&>();

    static_assert(!ref_name.empty());
    static_assert(!rref_name.empty());
    static_assert(ref_name != rref_name);

    CHECK(!ref_name.empty());
    CHECK(!rref_name.empty());
  }

  TEST_CASE("const and volatile") {
    constexpr auto const_name = type_name<const int>();
    constexpr auto volatile_name = type_name<volatile int>();
    constexpr auto cv_name = type_name<const volatile int>();

    static_assert(!const_name.empty());
    static_assert(!volatile_name.empty());
    static_assert(!cv_name.empty());

    CHECK(!const_name.empty());
    CHECK(!volatile_name.empty());
    CHECK(!cv_name.empty());
  }

  TEST_CASE("template types") {
    constexpr auto vec_name = type_name<std::vector<int>>();
    constexpr auto map_name = type_name<std::map<int, std::string>>();

    static_assert(!vec_name.empty());
    static_assert(!map_name.empty());

    // Should contain "vector" or "map"
    CHECK(vec_name.find("vector") != std::string_view::npos);
    CHECK(map_name.find("map") != std::string_view::npos);
  }

  TEST_CASE("user-defined types") {
    struct MyStruct {};
    class MyClass {};

    constexpr auto struct_name = type_name<MyStruct>();
    constexpr auto class_name = type_name<MyClass>();

    static_assert(!struct_name.empty());
    static_assert(!class_name.empty());
    static_assert(struct_name != class_name);

    CHECK(!struct_name.empty());
    CHECK(!class_name.empty());
  }
}

// ============================================================================
// type_name_array tests
// ============================================================================

TEST_SUITE("type_name_array") {
  TEST_CASE("returns array with type name") {
    constexpr auto arr = type_name_array<int>();

    static_assert(arr.size() > 0);
    CHECK(arr.size() > 0);
  }

  TEST_CASE("array matches string_view") {
    constexpr auto name = type_name<int>();
    constexpr auto arr = type_name_array<int>();

    static_assert(arr.size() == name.size());

    bool matches = true;
    for (std::size_t i = 0; i < name.size(); ++i) {
      if (arr[i] != name[i]) {
        matches = false;
        break;
      }
    }
    CHECK(matches);
  }
}

// ============================================================================
// type_id tests
// ============================================================================

TEST_SUITE("type_id") {
  TEST_CASE("same type same id") {
    static_assert(type_id<int> == type_id<int>);
    static_assert(type_id<float> == type_id<float>);

    CHECK(type_id<int> == type_id<int>);
    CHECK(type_id<float> == type_id<float>);
  }

  TEST_CASE("different types different ids") {
    static_assert(type_id<int> != type_id<float>);
    static_assert(type_id<int> != type_id<double>);
    static_assert(type_id<int> != type_id<long>);
    static_assert(type_id<int> != type_id<int*>);
    static_assert(type_id<int> != type_id<int&>);

    CHECK(type_id<int> != type_id<float>);
    CHECK(type_id<int> != type_id<double>);
  }

  TEST_CASE("type_id is non-zero") {
    static_assert(type_id<int> != 0);
    static_assert(type_id<float> != 0);

    CHECK(type_id<int> != 0);
    CHECK(type_id<float> != 0);
  }

  TEST_CASE("type_id can be used as NTTP") {
    constexpr auto test = []<std::size_t ID>() { return ID; };

    constexpr auto id = test.template operator()<type_id<int>>();
    static_assert(id == type_id<int>);
    CHECK(id == type_id<int>);
  }
}

// ============================================================================
// same_type_id tests
// ============================================================================

TEST_SUITE("same_type_id") {
  TEST_CASE("same types") {
    static_assert(same_type_id<int, int>);
    static_assert(same_type_id<float, float>);

    CHECK(same_type_id<int, int>);
    CHECK(same_type_id<float, float>);
  }

  TEST_CASE("different types") {
    static_assert(!same_type_id<int, float>);
    static_assert(!same_type_id<int, long>);

    CHECK_FALSE(same_type_id<int, float>);
    CHECK_FALSE(same_type_id<int, long>);
  }
}

// ============================================================================
// type_name comparison utilities tests
// ============================================================================

TEST_SUITE("type_name comparison") {
  TEST_CASE("type_names_equal") {
    static_assert(type_names_equal<int, int>());
    static_assert(!type_names_equal<int, float>());

    CHECK(type_names_equal<int, int>());
    CHECK_FALSE(type_names_equal<int, float>());
  }

  TEST_CASE("type_name_contains") {
    static_assert(type_name_contains<std::vector<int>>("vector"));
    static_assert(!type_name_contains<int>("vector"));

    CHECK(type_name_contains<std::vector<int>>("vector"));
    CHECK_FALSE(type_name_contains<int>("vector"));
  }

  TEST_CASE("type_name_starts_with") {
    // int should start with "int"
    static_assert(type_name_starts_with<int>("int"));

    CHECK(type_name_starts_with<int>("int"));
  }

  TEST_CASE("type_name_ends_with") {
    // This is compiler-dependent, but let's test it works
    constexpr auto name = type_name<int>();
    static_assert(type_name_ends_with<int>(name.substr(name.size() - 1)));
  }
}

// ============================================================================
// named_type tests
// ============================================================================

TEST_SUITE("named_type") {
  TEST_CASE("type alias") {
    using IntNamed = named_type<int>;

    static_assert(std::is_same_v<IntNamed::type, int>);
    CHECK((std::is_same_v<IntNamed::type, int>));
  }

  TEST_CASE("name member") {
    using IntNamed = named_type<int>;

    static_assert(!IntNamed::name.empty());
    static_assert(IntNamed::name == type_name<int>());

    CHECK(!IntNamed::name.empty());
    CHECK(IntNamed::name == type_name<int>());
  }

  TEST_CASE("id member") {
    using IntNamed = named_type<int>;

    static_assert(IntNamed::id == type_id<int>);
    CHECK(IntNamed::id == type_id<int>);
  }

  TEST_CASE("different named_types") {
    using IntNamed = named_type<int>;
    using FloatNamed = named_type<float>;

    static_assert(IntNamed::name != FloatNamed::name);
    static_assert(IntNamed::id != FloatNamed::id);

    CHECK(IntNamed::name != FloatNamed::name);
    CHECK(IntNamed::id != FloatNamed::id);
  }
}

// ============================================================================
// Practical usage tests
// ============================================================================

TEST_SUITE("practical usage") {
  TEST_CASE("type dispatch without RTTI") {
    // Simulate type-based dispatch using type_id
    constexpr auto dispatch = [](std::size_t id) {
      if (id == type_id<int>) return 1;
      if (id == type_id<float>) return 2;
      if (id == type_id<double>) return 3;
      return 0;
    };

    CHECK(dispatch(type_id<int>) == 1);
    CHECK(dispatch(type_id<float>) == 2);
    CHECK(dispatch(type_id<double>) == 3);
    CHECK(dispatch(type_id<char>) == 0);
  }

  TEST_CASE("compile-time type registration") {
    // Can use type_id as compile-time key
    constexpr std::size_t int_id = type_id<int>;
    constexpr std::size_t float_id = type_id<float>;

    static_assert(int_id != float_id);
    static_assert(int_id == type_id<int>);

    CHECK(int_id != float_id);
  }

  TEST_CASE("type name for debugging") {
    struct MyCustomType {};

    constexpr auto name = type_name<MyCustomType>();

    // The name should contain "MyCustomType"
    CHECK(name.find("MyCustomType") != std::string_view::npos);
  }
}
