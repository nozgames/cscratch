cmake_minimum_required(VERSION 3.15)
project(noz LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
include(FetchContent)

# SDL3
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        main
)

set(SDL3_STATIC ON CACHE BOOL "Build SDL as static library" FORCE)
set(SDL3_SHARED OFF CACHE BOOL "Don't build SDL as shared library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Don't build SDL tests" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Don't build SDL test library" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "Don't install SDL" FORCE)
set(SDL_LIBUDEV OFF CACHE BOOL "Disable udev support" FORCE)
set(SDL_DBUS OFF CACHE BOOL "Disable D-Bus support" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "Disable haptic/force feedback support" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "Disable sensor support" FORCE)
set(SDL3_VENDORED ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

# Box2d
FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v2.4.1
)

set(BOX2D_SHARED OFF)
set(BOX2D_STATIC ON)
set(BOX2D_BUILD_TESTBED OFF)
set(BOX2D_BUILD_UNIT_TESTS OFF)
set(BOX2D_BUILD_SAMPLES OFF)

# UTHASH
FetchContent_Declare(
  uthash
  GIT_REPOSITORY https://github.com/troydhanson/uthash.git
  GIT_TAG        v2.3.0
)

FetchContent_MakeAvailable(uthash SDL3 box2d)

add_library(noz STATIC)

target_precompile_headers(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h)

file(GLOB_RECURSE SOURCE_FILES_C "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
target_sources(noz PRIVATE ${SOURCE_FILES_C})
target_include_directories(noz PRIVATE ${enet_SOURCE_DIR}/include)
target_include_directories(noz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_include_directories(noz PRIVATE src ${uthash_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/external)

if(MSVC)
    target_compile_options(noz PRIVATE /WX)
else()
    target_compile_options(noz PRIVATE -Werror)
endif()

target_compile_definitions(noz PRIVATE _CRT_SECURE_NO_WARNINGS)

# Link dependencies for noz
target_link_libraries(noz
    PUBLIC
        box2d
        ws2_32
        winmm
        SDL3::SDL3
)
