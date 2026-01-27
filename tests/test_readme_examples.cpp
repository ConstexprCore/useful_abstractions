#include <doctest/doctest.h>
#include <constexprcore/fixed_string.h>
#include <constexprcore/constexpr_hash.h>
#include <constexprcore/type_name.h>

#include <iostream>
#include <string_view>

// Template from README example - defined at namespace scope
template<ConstexprCore::fixed_string Name>
struct NamedType {
    static constexpr auto type_name = Name;

    std::string get_name() const {
        return std::string(type_name.view());
    }
};

// ============================================================================
// README examples compilation test
// ============================================================================

TEST_SUITE("README examples") {
    TEST_CASE("Basic example compiles and runs") {
        // Compile-time string handling with fixed_string
        constexpr ConstexprCore::fixed_string fs{"Hello, ConstexprCore!"};
        static_assert(fs.size() == 21);

        // Compile-time hashing with FNV1a
        constexpr auto hash = ConstexprCore::fnv1a(std::string_view{"test"});

        // Compile-time type names
        constexpr auto int_name = ConstexprCore::type_name<int>();

        // Verify the results
        CHECK(fs.view() == "Hello, ConstexprCore!");
        CHECK(!int_name.empty());
        CHECK(int_name.find("int") != std::string_view::npos);
        CHECK(hash == ConstexprCore::fnv1a(std::string_view{"test"}));
    }

    TEST_CASE("Template usage example compiles and runs") {
        // Usage
        using PersonType = NamedType<"Person">;
        using AnimalType = NamedType<"Animal">;

        PersonType p;
        AnimalType a;

        CHECK(p.get_name() == "Person");
        CHECK(a.get_name() == "Animal");

        // Verify compile-time constants
        static_assert(PersonType::type_name.view() == std::string_view{"Person"});
        static_assert(AnimalType::type_name.view() == std::string_view{"Animal"});
    }
}