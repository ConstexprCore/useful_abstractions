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
      + [fixed_string as a template parameter](#fixed-string-as-a-template-parameter)
      + [Hashing, type names and escaping](#hashing-type-names-and-escaping)
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

Requires a C++23 compiler (GCC 14+, Clang 18+, recent MSVC or AppleClang).

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

### fixed_string as a template parameter

```cpp
#include <ConstexprCore/fixed_string.h>
#include <iostream>

template <ConstexprCore::fixed_string Name>
struct NamedType {
    static constexpr auto name = Name;
    void print() const { std::cout << "Type: " << name.view() << "\n"; }
};

int main() {
    NamedType<"Person">{}.print();   // Type: Person
    NamedType<"Animal">{}.print();   // Type: Animal
}
```

### Hashing, type names and escaping

```cpp
#include <ConstexprCore/useful_abstractions.h>
#include <iostream>

int main() {
    // Compile-time string handling.
    constexpr ConstexprCore::fixed_string fs{"Hello, ConstexprCore!"};
    static_assert(fs.size() == 21);

    // Compile-time hashing.
    constexpr auto hash = ConstexprCore::fnv1a(std::string_view{"test"});

    // Compile-time type names, no RTTI.
    constexpr auto int_name = ConstexprCore::type_name<int>();
    static_assert(int_name == "int");

    // JSON escaping, computed before main() runs.
    constexpr auto escaped = ConstexprCore::json_escape<"tab\there">();

    std::cout << fs.view() << " " << hash << " " << int_name
              << " " << escaped.view() << "\n";
}
```

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
