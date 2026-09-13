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
add_library(ConstexprCore::useful_abstractions ALIAS useful_abstractions)
target_include_directories(useful_abstractions INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(useful_abstractions INTERFACE cxx_std_23)

# ---------------------------------------------------------------------------
# Single-header amalgamation
#
# singleheader/useful_abstractions.h is checked in so users can grab it
# straight from the repository; this target regenerates it from include/. The
# script walks the include graph itself, so new headers are picked up without
# touching this file.
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter QUIET)

set(UA_SINGLEHEADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/singleheader/useful_abstractions.h")
set(UA_SINGLEHEADER_AVAILABLE OFF)
if(Python3_Interpreter_FOUND)
    set(UA_SINGLEHEADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/singleheader")
    set(UA_SINGLEHEADER_OUTPUT "${UA_SINGLEHEADER_OUTPUT_DIR}/useful_abstractions.h")
    set(UA_AMALGAMATE "${CMAKE_CURRENT_SOURCE_DIR}/singleheader/amalgamate.py")

    file(GLOB_RECURSE UA_PUBLIC_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/ConstexprCore/*.h")

    add_custom_command(
        OUTPUT "${UA_SINGLEHEADER_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${UA_SINGLEHEADER_OUTPUT_DIR}"
        COMMAND ${Python3_EXECUTABLE} "${UA_AMALGAMATE}"
            --output "${UA_SINGLEHEADER_OUTPUT}"
            --demo "${UA_SINGLEHEADER_OUTPUT_DIR}/amalgamation_demo.cpp"
            --quiet
        DEPENDS "${UA_AMALGAMATE}" ${UA_PUBLIC_HEADERS}
        VERBATIM
        COMMENT "Generating single-header useful_abstractions.h"
    )

    add_custom_target(useful_abstractions_singleheader DEPENDS "${UA_SINGLEHEADER_OUTPUT}")
    set(UA_SINGLEHEADER_AVAILABLE ON)

    # Refresh the checked-in copy under singleheader/ from a build tree.
    add_custom_target(useful_abstractions_singleheader_update
        COMMAND ${Python3_EXECUTABLE} "${UA_AMALGAMATE}"
        COMMENT "Updating singleheader/useful_abstractions.h in the source tree"
        VERBATIM
    )
else()
    message(STATUS "Python3 interpreter not found; skipping single-header generation target")
endif()

# ---------------------------------------------------------------------------
# Install / export
#
# What gets installed is the amalgamated header, placed at the same include
# path the sources use, so <ConstexprCore/useful_abstractions.h> works the same
# whether the library was consumed from source or from an install tree. The
# individual headers are installed alongside it for people who prefer them.
# ---------------------------------------------------------------------------
option(UA_INSTALL "Generate the install target" ${PROJECT_IS_TOP_LEVEL})
if(UA_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    add_library(useful_abstractions_installed INTERFACE)
    target_compile_features(useful_abstractions_installed INTERFACE cxx_std_23)
    target_include_directories(useful_abstractions_installed INTERFACE
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    set_target_properties(useful_abstractions_installed PROPERTIES
        EXPORT_NAME useful_abstractions)

    install(TARGETS useful_abstractions_installed EXPORT useful_abstractionsTargets)

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/ConstexprCore"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.h")

    if(UA_SINGLEHEADER_AVAILABLE)
        set(UA_INSTALLED_HEADER "${UA_SINGLEHEADER_OUTPUT}")
        install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target useful_abstractions_singleheader)")
    else()
        set(UA_INSTALLED_HEADER "${UA_SINGLEHEADER_SOURCE}")
    endif()
    # The amalgamation shadows the umbrella header, so an installed copy is
    # self-contained, and is also available under its bare name.
    install(FILES "${UA_INSTALLED_HEADER}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ConstexprCore")
    install(FILES "${UA_INSTALLED_HEADER}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/useful_abstractionsConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/useful_abstractionsConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/useful_abstractionsConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/useful_abstractions")

    install(EXPORT useful_abstractionsTargets
        FILE useful_abstractionsTargets.cmake
        NAMESPACE ConstexprCore::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/useful_abstractions")

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/useful_abstractionsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/useful_abstractionsConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/useful_abstractions")
endif()

# Enable testing
option(UA_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

if(UA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
