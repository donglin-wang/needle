cmake_minimum_required(VERSION 3.20)
project(needle CXX C)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# libsais
FetchContent_Declare(libsais
  GIT_REPOSITORY https://github.com/IlyaGrebnov/libsais.git
  GIT_TAG        v2.8.5
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(libsais)

add_executable(needle src/main.cpp)
target_link_libraries(needle PRIVATE libsais)

# GoogleTest
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()
file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE GTest::gtest_main libsais)
include(GoogleTest)
gtest_discover_tests(tests)

# Google Benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.1
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googlebenchmark)

file(GLOB_RECURSE BENCH_SOURCES benchmarks/*.cpp)
add_executable(benchmarks ${BENCH_SOURCES})
target_link_libraries(benchmarks PRIVATE benchmark::benchmark libsais)
