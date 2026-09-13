// constexpr_hash: FNV-1a hashing in constant expressions, and string switches.
#include <ConstexprCore/constexpr_hash.h>

#include <cstdio>
#include <string_view>

using namespace ConstexprCore;
using namespace ConstexprCore::literals;   // "..."_hash

// The same function hashes at compile time and at run time.
static_assert(fnv1a("hello") == fnv1a(std::string_view{"hello"}));
static_assert(fnv1a("hello") != fnv1a("world"));
static_assert(hash_v<"hello"> == fnv1a("hello"));       // fixed_string form

// The classic use: switch on a string. The case labels are computed by the
// compiler; only the runtime input is hashed at run time.
constexpr int status_for(std::string_view method) {
    switch (fnv1a(method)) {
        case "GET"_hash:    return 200;
        case "POST"_hash:   return 201;
        case "DELETE"_hash: return 204;
        default:            return 405;
    }
}
static_assert(status_for("GET") == 200);
static_assert(status_for("POST") == 201);
static_assert(status_for("BREW") == 405);

// Combine hashes of several fields into one, e.g. for a composite key.
constexpr std::size_t point_hash = hash_combine(fnv1a("x"), fnv1a("y"));
static_assert(point_hash != fnv1a("x") && point_hash != fnv1a("y"));

// A functor with the std::hash interface, usable in constant expressions.
constexpr constexpr_hash<std::string_view> hasher;
static_assert(hasher("key") == fnv1a("key"));

int main(int argc, char** argv) {
    std::string_view method = argc > 1 ? argv[1] : "GET";
    std::printf("%.*s -> %d\n", int(method.size()), method.data(), status_for(method));
    std::printf("fnv1a(\"hello\") = %zu\n", fnv1a("hello"));
    return status_for("GET") == 200 ? 0 : 1;
}
