# Useful Abstractions

[![CI](https://github.com/ConstexprCore/useful_abstractions/actions/workflows/ci.yml/badge.svg)](https://github.com/ConstexprCore/useful_abstractions/actions/workflows/ci.yml)

A header-only C++23 library of compile-time abstractions: strings you can pass
as template parameters, hashing, type names, UTF conversion and escaping — all
usable inside `constexpr` and `consteval` code, and all with **no dependencies
beyond the standard library**.

   * [What is in the box](#what-is-in-the-box)
   * [Installing it](#installing-it)
      + [Drop in one header (no build system, no dependencies)](#drop-in-one-header-no-build-system-no-dependencies)
   * [Using it as a CMake dependency](#using-it-as-a-cmake-dependency)
      + [FetchContent](#fetchcontent)
      + [find_package](#find-package)
      + [Verifying all three](#verifying-all-three)
   * [Usage](#usage)
      + [fixed_string — a string as a template parameter](#fixed_string--a-string-as-a-template-parameter)
      + [constexpr_hash — hashing and `switch` on strings](#constexpr_hash--hashing-and-switch-on-strings)
      + [type_name — type names and ids without RTTI](#type_name--type-names-and-ids-without-rtti)
      + [escape_string — JSON, URL and HTML escaping by the compiler](#escape_string--json-url-and-html-escaping-by-the-compiler)
      + [utf_convert — UTF-8 validation and conversion at compile time](#utf_convert--utf-8-validation-and-conversion-at-compile-time)
      + [constexpr_ptr — viewing memory as another type in constant evaluation](#constexpr_ptr--viewing-memory-as-another-type-in-constant-evaluation)
      + [constexpr_check — clear compile errors from `consteval` code](#constexpr_check--clear-compile-errors-from-consteval-code)
   * [Building](#building)
   * [Contributing](#contributing)
   * [License](#license)

## What is in the box

Seven components. Include the umbrella header for everything, or just the one
you need — each header stands on its own.

| Header | What it gives you |
|---|---|
| `fixed_string.h` | A string literal usable as a non-type template parameter, with the usual comparisons and `view()` |
| `constexpr_hash.h` | FNV-1a and friends over `string_view` / `fixed_string`, at compile time |
| `type_name.h` | `type_name<T>()` and a stable `type_id<T>`, without RTTI |
| `escape_string.h` | JSON escaping and percent-encoding computed in a constant expression |
| `utf_convert.h` | UTF-8 validation with precise error codes, and UTF-8 → UTF-16 / UTF-32 conversion of string literals at compile time |
| `constexpr_ptr.h` | `reinterpret_ptr` / `writable_ptr`: type-punning views that work in constant evaluation, where `reinterpret_cast` cannot |
| `constexpr_check.h` | `static_require` and `error_result`: turn a failed check inside a `consteval` function into a compiler error with your message |

```cpp
#include <ConstexprCore/useful_abstractions.h>   // everything
#include <ConstexprCore/fixed_string.h>          // or just one component
```

Requires a C++23 compiler: GCC 14+, Clang 19+ or a recent AppleClang. (Clang 18
rejects the always-throwing `consteval` helper in `constexpr_check.h`.)

## Installing it

Three ways, in increasing order of build-system involvement.

### Drop in one header (no build system, no dependencies)

`singleheader/useful_abstractions.h` is a self-contained amalgamation of the
whole library. Grab it and compile:

```bash
curl -LO https://github.com/ConstexprCore/useful_abstractions/releases/download/v0.1.0/useful_abstractions.h
```

```cpp
#include "useful_abstractions.h"

using namespace ConstexprCore;
static_assert(type_name<double>() == "double");
```

```bash
c++ -std=c++23 -O2 main.cpp -o main
```

To regenerate the file from a checkout:

```bash
python3 singleheader/amalgamate.py          # writes singleheader/useful_abstractions.h
python3 singleheader/amalgamate.py --test   # ...and compiles the demo against it
python3 singleheader/amalgamate.py --check  # CI: fail if the checked-in copy is stale
```

The script walks the include graph from `include/ConstexprCore/useful_abstractions.h`
and splices each header in where it is included, leaving system includes alone.
A CMake build also produces `build/singleheader/useful_abstractions.h` via the
`useful_abstractions_singleheader` target.

## Using it as a CMake dependency

### FetchContent

Tests switch themselves off when the library is not the top-level project, so
there is nothing else to configure.

```cmake
include(FetchContent)
FetchContent_Declare(
    useful_abstractions
    GIT_REPOSITORY https://github.com/ConstexprCore/useful_abstractions.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(useful_abstractions)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ConstexprCore::useful_abstractions)
target_compile_features(my_app PRIVATE cxx_std_23)
```

`add_subdirectory(useful_abstractions)` on a vendored checkout works the same way.

### find_package

Install the project once:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

then consume it from anywhere:

```cmake
find_package(useful_abstractions REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ConstexprCore::useful_abstractions)
```

The install ships the individual headers *and* the amalgamation, with the
amalgamation placed at the umbrella path, so this works either way:

```cpp
#include <ConstexprCore/useful_abstractions.h>   // self-contained after install
#include <ConstexprCore/fixed_string.h>          // individual components too
```

Add `-DCMAKE_PREFIX_PATH=<prefix>` when installing somewhere CMake does not
search by default, and `-DUA_INSTALL=OFF` to suppress the install rules when
embedding the project in a larger build.

### Verifying all three

`tools/test_packaging.py` builds and runs a real consumer for each path:

```bash
python3 tools/test_packaging.py
python3 tools/test_packaging.py --only dropin
```

The consumer projects live in `tests/packaging/`.

## Usage

One complete program per component. Every one of these is compiled and run as
part of the test suite (`examples/*.cpp`), and the README text is copied from
those files verbatim, so what you read here is what was tested.

### fixed_string — a string as a template parameter

The piece everything else builds on: a string literal that can be a non-type
template parameter, built up and compared at compile time, and read back as a
`string_view`.

<!-- example: examples/fixed_string.cpp -->
```cpp
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
```
<!-- /example -->

### constexpr_hash — hashing and `switch` on strings

FNV-1a that runs in constant expressions, so `case "GET"_hash:` is a
compile-time constant and dispatching on a runtime string is one hash and a
jump table.

<!-- example: examples/constexpr_hash.cpp -->
```cpp
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
```
<!-- /example -->

### type_name — type names and ids without RTTI

The name of a type as a `string_view`, and a stable integer id derived from
it, both usable in `static_assert` and as registry keys.

<!-- example: examples/type_name.cpp -->
```cpp
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
```
<!-- /example -->

### escape_string — JSON, URL and HTML escaping by the compiler

Escaped literals are computed once at compile time and sit in read-only data;
sizes are known without materialising the result.

<!-- example: examples/escape_string.cpp -->
```cpp
// escape_string: JSON, URL and HTML escaping done by the compiler.
#include <ConstexprCore/escape_string.h>

#include <cstdio>
#include <string>

using namespace ConstexprCore;

// Escaping a literal at compile time means the escaped bytes live in the
// binary's read-only data: nothing to compute, nothing to allocate.
constexpr auto json  = json_escape<"say \"hi\"\n">();
static_assert(json.view() == R"(say \"hi\"\n)");

constexpr auto quoted = json_quoted<"say \"hi\"">();    // adds the quotes too
static_assert(quoted.view() == R"("say \"hi\"")");

constexpr auto url = percent_encode<"Hello World!">();
static_assert(url.view() == "Hello%20World%21");

constexpr auto html = html_escape<"<b>&</b>">();
static_assert(html.view() == "&lt;b&gt;&amp;&lt;/b&gt;");

// Sizes are available without materialising the result.
static_assert(json_escape_size<"say \"hi\"\n">() == json.size());

// Composing at compile time: a URL with an encoded query.
constexpr auto query = "https://example.com/search?q="_fs + percent_encode<"c++ & you">();
static_assert(query.view() == "https://example.com/search?q=c%2b%2b%20%26%20you");

int main() {
    std::printf("json   : %s\n", std::string(json.view()).c_str());
    std::printf("quoted : %s\n", std::string(quoted.view()).c_str());
    std::printf("url    : %s\n", std::string(url.view()).c_str());
    std::printf("html   : %s\n", std::string(html.view()).c_str());
    std::printf("query  : %s\n", std::string(query.view()).c_str());
    return url.view() == "Hello%20World%21" ? 0 : 1;
}
```
<!-- /example -->

### utf_convert — UTF-8 validation and conversion at compile time

Validation that reports *why* and *where* input is invalid, and conversion of
UTF-8 literals to UTF-16 or UTF-32 with the result sized exactly.

<!-- example: examples/utf_convert.cpp -->
```cpp
// utf_convert: validate UTF-8 and convert it to UTF-16 / UTF-32 at compile time.
#include <ConstexprCore/utf_convert.h>

#include <cstdio>

using namespace ConstexprCore;

// "€" is E2 82 AC in UTF-8. Written as bytes so the source file's own
// encoding does not matter.
constexpr auto euro = validate_utf8<"Price: \xE2\x82\xAC">();
static_assert(euro.ok());

// Invalid input is reported with a reason and a byte position, not a bool.
constexpr auto bad = validate_utf8<"ok\xFF">();
static_assert(!bad.ok());
static_assert(bad.error == utf8_error::invalid_lead_byte);
static_assert(bad.position == 2);

constexpr auto cut = validate_utf8<"\xE2\x82">();          // sequence cut short
static_assert(cut.error == utf8_error::truncated_sequence);

// Conversion to UTF-16 and UTF-32. The result is a fixed_string of the
// target character type, sized exactly.
constexpr auto utf16 = utf8_to_utf16<"Price: \xE2\x82\xAC">();
static_assert(utf16.view() == u"Price: €");
static_assert(utf16.size() == 8);

constexpr auto utf32 = utf8_to_utf32<"\xE2\x82\xAC">();
static_assert(utf32.size() == 1 && utf32[0] == U'€');

// Counting code points rather than bytes.
static_assert(utf8_code_point_count<"Price: \xE2\x82\xAC">() == 8);
static_assert(sizeof("Price: \xE2\x82\xAC") - 1 == 10);       // bytes

// The _checked variants refuse to compile on invalid input:
//   constexpr auto oops = utf8_to_utf16_checked<"\xFF">();   // error

int main() {
    std::printf("valid           : %s\n", euro.ok() ? "yes" : "no");
    std::printf("bad at position : %zu\n", bad.position);
    std::printf("utf16 units     : %zu\n", utf16.size());
    std::printf("code points     : %zu\n", utf8_code_point_count<"Price: \xE2\x82\xAC">());
    return euro.ok() && !bad.ok() ? 0 : 1;
}
```
<!-- /example -->

### constexpr_ptr — viewing memory as another type in constant evaluation

`reinterpret_cast` is forbidden during constant evaluation; `reinterpret_ptr`
and `writable_ptr` give the same byte-level view legally, via `bit_cast` per
element, and behave as random-access iterators.

<!-- example: examples/constexpr_ptr.cpp -->
```cpp
// constexpr_ptr: view memory as another type where reinterpret_cast cannot go.
#include <ConstexprCore/constexpr_ptr.h>

#include <cstdint>
#include <cstdio>

using namespace ConstexprCore;

// reinterpret_cast is not allowed during constant evaluation. reinterpret_ptr
// gives the same "read these chars as bytes" view, legally, via bit_cast on
// each element.
constexpr std::uint8_t checksum(const char* data, std::size_t n) {
    reinterpret_ptr<std::uint8_t, char> bytes{data};
    std::uint8_t sum = 0;
    for (std::size_t i = 0; i < n; ++i) sum = static_cast<std::uint8_t>(sum + bytes[i]);
    return sum;
}

constexpr char payload[] = {'\x01', '\x02', '\xFF'};
static_assert(checksum(payload, 3) == 2);                  // 1 + 2 + 255, mod 256
static_assert(*reinterpret_ptr<std::uint8_t, char>{payload + 2} == 255);

// It is a random-access iterator, so it works with algorithms and ranges.
constexpr bool all_nonzero() {
    auto first = make_reinterpret_ptr<std::uint8_t>(payload);
    for (auto it = first; it != first + 3; ++it) if (*it == 0) return false;
    return true;
}
static_assert(all_nonzero());

// The writable counterpart, for building byte buffers in constexpr code.
constexpr std::uint32_t little_endian_word() {
    char buffer[4] = {};
    writable_ptr<std::uint8_t, char> out{buffer};
    out[0] = 0x78; out[1] = 0x56; out[2] = 0x34; out[3] = 0x12;
    std::uint32_t word = 0;
    reinterpret_ptr<std::uint8_t, char> in{buffer};
    for (int i = 3; i >= 0; --i) word = (word << 8) | in[i];
    return word;
}
static_assert(little_endian_word() == 0x12345678);

int main() {
    std::printf("checksum : %u\n", checksum(payload, 3));
    std::printf("word     : 0x%08x\n", little_endian_word());
    return checksum(payload, 3) == 2 ? 0 : 1;
}
```
<!-- /example -->

### constexpr_check — clear compile errors from `consteval` code

Turn a failed check inside a `consteval` function into a compiler error that
carries your message, and report validation failures with a position.

<!-- example: examples/constexpr_check.cpp -->
```cpp
// constexpr_check: turn a failed check inside consteval code into a clear
// compiler error, with your message.
#include <ConstexprCore/constexpr_check.h>
#include <ConstexprCore/fixed_string.h>

#include <cstdio>

using namespace ConstexprCore;

// A validated value type: constructing it with bad input does not compile.
consteval unsigned short port(unsigned value) {
    static_require(value != 0, "port 0 is reserved");
    static_require(value < 65536, "port must fit in 16 bits");
    return static_cast<unsigned short>(value);
}
constexpr auto https = port(443);
// constexpr auto nope = port(70000);   // error: "port must fit in 16 bits"

// Validating text, with the position of the problem reported back.
enum class ident_error { none, empty, bad_start, bad_char };

template <fixed_string Str>
consteval error_result<ident_error> check_identifier() {
    if (Str.size() == 0) return {ident_error::empty, 0};
    auto ok_start = [](char c) { return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
    auto ok_rest  = [&](char c) { return ok_start(c) || (c >= '0' && c <= '9'); };
    if (!ok_start(Str[0])) return {ident_error::bad_start, 0};
    for (std::size_t i = 1; i < Str.size(); ++i)
        if (!ok_rest(Str[i])) return {ident_error::bad_char, i};
    return {ident_error::none, 0};
}

static_assert(check_identifier<"user_id">().ok());
constexpr auto bad = check_identifier<"user-id">();
static_assert(bad.error == ident_error::bad_char && bad.position == 4);

// require_ok turns that result into a compile error when it must be valid.
template <fixed_string Str>
consteval auto identifier() {
    require_ok(check_identifier<Str>(), "not a valid identifier");
    return Str;
}
constexpr auto name = identifier<"user_id">();
// constexpr auto oops = identifier<"user-id">();   // error: "not a valid identifier"

int main() {
    std::printf("port  : %u\n", https);
    std::printf("ident : %.*s\n", int(name.size()), name.view().data());   // not NUL-terminated
    std::printf("bad   : position %zu\n", bad.position);
    return https == 443 ? 0 : 1;
}
```
<!-- /example -->

## Building

Requires a C++23 compiler and CMake 3.20+.

```bash
cmake -B build
cmake --build build
ctest --test-dir build
```

`UA_BUILD_TESTS` and `UA_INSTALL` default to `ON` for a top-level build and
`OFF` when the project is consumed from another CMake project.

Releases are cut by the **Release** workflow from the Actions tab
(`gh workflow run release.yml -f bump=minor`, or an explicit `version`): it
bumps the version, regenerates the amalgamation, verifies the header builds
against the standard library alone, tags, and attaches
`useful_abstractions.h` to the GitHub release.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contact

For questions or suggestions, please open an issue on GitHub.
