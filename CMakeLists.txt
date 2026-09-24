cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_COMPILER g++-16)
project(stencil_lab CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

add_compile_options(-Wall -Wextra -Werror=return-type -Wno-interference-size)
# RelWithDebInfo is the profiling config: -O2 -g plus frame pointers so perf
# (and CLion's profiler) can walk the stack
add_compile_options($<$<CONFIG:RelWithDebInfo>:-fno-omit-frame-pointer>)

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB includes CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/include/*.h)

add_executable(stencil ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_include_directories(stencil PRIVATE include)
install(TARGETS stencil)

enable_testing()
file(GLOB test_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/test/*.cpp)
foreach(test_source IN LISTS test_sources)
    cmake_path(GET test_source STEM test_name)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PRIVATE include)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
