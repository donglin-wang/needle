cmake_minimum_required(VERSION 3.20)
project(needle CXX C)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

find_package(re2 11.0.0 EXACT REQUIRED)

# libsais
FetchContent_Declare(libsais
  GIT_REPOSITORY https://github.com/IlyaGrebnov/libsais.git
  GIT_TAG        v2.8.5
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(libsais)

add_executable(smoke_test_cpp src/smoke_test.cpp)
target_link_libraries(smoke_test_cpp PRIVATE re2::re2 libsais)
