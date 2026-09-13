// Compile with:  c++ -std=c++23 -I. amalgamation_demo.cpp -o demo
#include "useful_abstractions.h"

#include <cstdio>
#include <string>

using namespace ConstexprCore;

// fixed_string: a string literal usable as a template parameter.
template <fixed_string Name>
struct tagged {
    static constexpr std::string_view tag = Name.view();
};
static_assert(tagged<"widget">::tag == "widget");

// Hashing and type names, both in a constant expression.
static_assert(fnv1a("widget") != fnv1a("gadget"));
static_assert(type_name<double>() == "double");

// JSON escaping computed at compile time.
static constexpr auto escaped = json_escape<"tab\there">();

int main() {
    std::printf("useful_abstractions %s\n", useful_abstractions_version);
    std::printf("tag       : %s\n", std::string(tagged<"widget">::tag).c_str());
    std::printf("type_name : %s\n", std::string(type_name<double>()).c_str());
    std::printf("escaped   : %s\n", std::string(escaped.view()).c_str());
    return 0;
}
