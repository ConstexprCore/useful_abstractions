// type_name: the name of a type as a compile-time string, without RTTI.
#include <ConstexprCore/type_name.h>

#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace ConstexprCore;

// Plain types come back verbatim.
static_assert(type_name<int>() == "int");
static_assert(type_name<double>() == "double");

// Library types include their namespace; spell out expectations with
// predicates rather than exact strings, since compilers format template
// arguments differently.
static_assert(type_name_contains<std::vector<int>>("vector"));
static_assert(type_name_starts_with<std::string>("std::"));

// A stable integer id per type: a hash of the name.
static_assert(type_id<int> != type_id<float>);
static_assert(type_id<int> == type_id<int>);
static_assert(same_type_id<std::vector<int>, std::vector<int>>);

// A registry keyed by type name, filled in at compile time.
template <typename T>
struct registered {
    static constexpr std::string_view name = type_name<T>();
    static constexpr std::size_t id = type_id<T>;
};

int main() {
    // type_name() views into the compiler's function signature, so it is
    // not NUL-terminated: print it with a length, or copy it into a string.
    std::printf("%-24.*s id=%zu\n", int(registered<int>::name.size()),
                registered<int>::name.data(), registered<int>::id);
    std::printf("%-24s id=%zu\n", std::string(registered<std::map<int, int>>::name).c_str(),
                registered<std::map<int, int>>::id);
    return type_name<int>() == "int" ? 0 : 1;
}
