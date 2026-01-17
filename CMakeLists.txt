cmake_minimum_required(VERSION 3.20)
project(useful_abstractions
    VERSION 0.1.0
    DESCRIPTION "Compile-time C++ abstractions for immediate functions"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library target (header-only)
add_library(useful_abstractions INTERFACE)
target_include_directories(useful_abstractions INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Enable testing
option(UA_BUILD_TESTS "Build tests" ON)

if(UA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
