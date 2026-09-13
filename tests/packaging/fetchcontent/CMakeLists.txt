# Consumer smoke test: pull useful_abstractions in with FetchContent.
# UA_SOURCE is set by tools/test_packaging.py to this checkout; a real project
# would use GIT_REPOSITORY/GIT_TAG instead (see README.md).
cmake_minimum_required(VERSION 3.20)
project(ua_fetchcontent_consumer LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(useful_abstractions SOURCE_DIR "${UA_SOURCE}")
FetchContent_MakeAvailable(useful_abstractions)

add_executable(consumer ../consumer.cpp)
target_link_libraries(consumer PRIVATE ConstexprCore::useful_abstractions)
target_compile_features(consumer PRIVATE cxx_std_23)
