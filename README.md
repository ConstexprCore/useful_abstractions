# Useful Abstractions

A modern C++ header-only library providing useful abstractions and utilities, with a focus on `constexpr` functionality for compile-time computations.

## Features

- **Constexpr Core**: Collection of constexpr utilities for string manipulation, reinterpret operations, and other compile-time computations
- **Header-only**: No need for separate compilation, just include the headers
- **Zero dependencies**: Only depends on the C++ standard library

## Requirements

- C++23 compatible compiler (GCC 15+, Clang 19+, ...)
- CMake 3.20+ for building tests

## Building

```bash
# Clone the repository
git clone https://github.com/ConstexprCore/useful_abstractions.git
cd useful_abstractions


# Configure with CMake
cmake -B build

# Build
cmake --build build

# Run tests
ctest --test-dir build
```

## Usage

Simply include the headers you need:

```cpp
#include <constexprcore/fixed_string.h>
#include <constexprcore/constexpr_hash.h>
#include <constexprcore/type_name.h>
```

### Example

```cpp
#include <constexprcore/fixed_string.h>
#include <constexprcore/constexpr_hash.h>
#include <constexprcore/type_name.h>
#include <iostream>

int main() {
    // Compile-time string handling with fixed_string
    constexpr ConstexprCore::fixed_string fs{"Hello, ConstexprCore!"};
    static_assert(fs.size() == 21);
    std::cout << "String: " << fs.view() << std::endl;

    // Compile-time hashing with FNV1a
    constexpr auto hash = ConstexprCore::fnv1a(std::string_view{"test"});
    std::cout << "Hash of 'test': " << hash << std::endl;

    // Compile-time type names
    constexpr auto int_name = ConstexprCore::type_name<int>();
    std::cout << "Type name of int: " << int_name << std::endl;

    return EXIT_SUCCESS;
}
```

### Template Usage

`fixed_string` can be used directly as a template parameter, enabling compile-time string-based metaprogramming:

```cpp
#include <constexprcore/fixed_string.h>

// fixed_string as template parameter
template<ConstexprCore::fixed_string Name>
struct NamedType {
    static constexpr auto type_name = Name;
    
    void print() const {
        std::cout << "Type: " << type_name.view() << std::endl;
    }
};

// Usage
using PersonType = NamedType<"Person">;
using AnimalType = NamedType<"Animal">;

int main() {
    PersonType p;
    p.print();  // Output: "Type: Person"
    
    AnimalType a;
    a.print();  // Output: "Type: Animal"
    
    return 0;
}
```


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
