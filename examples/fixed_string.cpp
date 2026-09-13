// fixed_string: a string you can pass as a template parameter.
#include <ConstexprCore/fixed_string.h>

#include <cstdio>

using namespace ConstexprCore;
using namespace ConstexprCore::literals;   // "..."_fs

// A string literal as a non-type template parameter. This is what the rest
// of the library builds on: every <"literal"> below is a fixed_string.
template <fixed_string Name>
struct field {
    static constexpr std::string_view name = Name.view();
    static constexpr std::size_t length = Name.size();
};

using user_id = field<"user_id">;
static_assert(user_id::name == "user_id");
static_assert(user_id::length == 7);

// Values, not just types: build strings at compile time.
constexpr auto prefix = "api/v1/"_fs;
constexpr auto route  = prefix + "users"_fs;            // concatenation
static_assert(route.view() == "api/v1/users");
static_assert(route.size() == 12);
static_assert(route[0] == 'a' && route.back() == 's');

// Comparison works across different lengths, and orders lexicographically.
static_assert("abc"_fs == "abc"_fs);
static_assert("abc"_fs != "abcd"_fs);
static_assert("abc"_fs < "abd"_fs);

// Reading a fixed_string back out as a plain string_view.
constexpr std::string_view as_view = route;             // implicit conversion

int main() {
    // A fixed_string holds exactly its characters, with no terminator: print
    // through a string_view with a length, never with a bare %s.
    std::printf("field   : %.*s (%zu chars)\n", int(user_id::length), user_id::name.data(), user_id::length);
    std::printf("route   : %.*s\n", int(as_view.size()), as_view.data());
    for (char c : "abc"_fs) std::printf("%c ", c);       // iterable
    std::printf("\n");
    return route.view() == "api/v1/users" ? 0 : 1;
}
