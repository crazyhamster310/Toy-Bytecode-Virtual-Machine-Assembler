cmake_minimum_required(VERSION 3.24)

project(ToyBytecodeVM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type" FORCE)
endif()

add_library(vm_core STATIC
    runtime/src/vm.cpp
)
target_include_directories(vm_core PUBLIC runtime/include)

add_executable(byte-vm runtime/src/main.cpp)
target_link_libraries(byte-vm PRIVATE vm_core)

add_library(vm_project_options INTERFACE)
target_compile_options(vm_project_options INTERFACE
    -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow -Wswitch -Wswitch-enum
    -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual
)

option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior sanitizers" ON)
if(ENABLE_SANITIZERS AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    target_compile_options(vm_project_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(vm_project_options INTERFACE -fsanitize=address,undefined)
endif()

target_link_libraries(vm_core PRIVATE vm_project_options)
target_link_libraries(byte-vm PRIVATE vm_project_options)

include(CTest)

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(vm_tests
        runtime/tests/test_stack.cpp
        runtime/tests/test_memory.cpp
        runtime/tests/test_vm.cpp
    )
    target_link_libraries(vm_tests PRIVATE
        vm_project_options
        vm_core
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(vm_tests)
endif()