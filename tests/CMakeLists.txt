include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

# Test executable
add_executable(ua_tests
    main.cpp
    test_fixed_string.cpp
    test_constexpr_ptr.cpp
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
