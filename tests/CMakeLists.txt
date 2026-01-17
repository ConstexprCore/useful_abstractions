include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

# Test executable
add_executable(ua_tests
    main.cpp
    test_fixed_string.cpp
    test_constexpr_ptr.cpp
    test_constexpr_hash.cpp
    test_type_name.cpp
    test_utf_convert.cpp
    test_escape_string.cpp
)

target_link_libraries(ua_tests PRIVATE
    useful_abstractions
    doctest::doctest
)

target_compile_options(ua_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(ua_tests)
