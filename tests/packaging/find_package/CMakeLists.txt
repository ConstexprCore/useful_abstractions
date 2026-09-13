# Consumer smoke test: find_package against an install tree.
cmake_minimum_required(VERSION 3.20)
project(ua_find_package_consumer LANGUAGES CXX)

find_package(useful_abstractions REQUIRED)

add_executable(consumer ../consumer.cpp)
target_link_libraries(consumer PRIVATE ConstexprCore::useful_abstractions)
