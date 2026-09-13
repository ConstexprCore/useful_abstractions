# One program per component. Each is a complete, self-checking example: it
# static_asserts its claims and returns non-zero if a runtime check fails.
# They are registered as tests, and tools/sync_readme_examples.py copies them
# verbatim into README.md, so the documentation is exactly what was compiled.
file(GLOB UA_EXAMPLE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(source IN LISTS UA_EXAMPLE_SOURCES)
    get_filename_component(name "${source}" NAME_WLE)
    if(name MATCHES "^\\.")   # editor backups, macOS ._ resource forks
        continue()
    endif()
    add_executable(example_${name} "${source}")
    target_link_libraries(example_${name} PRIVATE ConstexprCore::useful_abstractions)
    target_compile_options(example_${name} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>)
    add_test(NAME example_${name} COMMAND example_${name})
endforeach()
