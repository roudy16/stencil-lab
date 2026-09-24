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

# CXX-only so the fetched SDL3 (C) builds with its own flags
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall$<SEMICOLON>-Wextra$<SEMICOLON>-Werror=return-type$<SEMICOLON>-Wno-interference-size>)
# RelWithDebInfo is the profiling config: -O2 -g plus frame pointers so perf
# (and CLion's profiler) can walk the stack
add_compile_options($<$<CONFIG:RelWithDebInfo>:-fno-omit-frame-pointer>)

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB includes CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/include/*.h)

# SDL3 is not packaged for Ubuntu/Pop!_OS 24.04, so the build fetches a pinned
# release (same pin as soundfog)
include(FetchContent)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.14
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(stencil ${sources})
target_include_directories(stencil PRIVATE include)
target_link_libraries(stencil PRIVATE SDL3::SDL3)
install(TARGETS stencil)

enable_testing()
file(GLOB test_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/test/*.cpp)
foreach(test_source IN LISTS test_sources)
    cmake_path(GET test_source STEM test_name)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PRIVATE include)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
