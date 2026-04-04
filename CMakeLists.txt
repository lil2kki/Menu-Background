cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
    add_definitions(-DGLES_SILENCE_DEPRECATION)
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(main)

add_library(${PROJECT_NAME} SHARED src/main.cpp)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})

CPMAddPackage("gh:matcool/gd-imgui-cocos#geode")
target_link_libraries(${PROJECT_NAME} imgui-cocos)

# fell in love with a imgui
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wnontrivial-memcall HAVE_NONTRIVIAL_MEMCALL)
if (HAVE_NONTRIVIAL_MEMCALL)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-nontrivial-memcall)
endif()
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    target_link_libraries(${PROJECT_NAME} "-framework CoreGraphics")
endif()
