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

set(BOX2D_SHARED OFF CACHE BOOL "Don't build Box2D as shared library" FORCE)
set(BOX2D_STATIC ON CACHE BOOL "Build Box2D as static library" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "Don't build Box2D testbed" FORCE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Don't build Box2D unit tests" FORCE)
set(BOX2D_BUILD_SAMPLES OFF CACHE BOOL "Don't build Box2D samples" FORCE)

FetchContent_MakeAvailable(SDL3 box2d)

add_library(noz STATIC)

target_precompile_headers(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h)

file(GLOB_RECURSE SOURCE_FILES_C "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Remove platform-specific files that don't match the current platform
if(WIN32)
    list(REMOVE_ITEM SOURCE_FILES_C "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_posix.c")
else()
    list(REMOVE_ITEM SOURCE_FILES_C "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_win.c")
endif()

target_sources(noz PRIVATE ${SOURCE_FILES_C})
target_include_directories(noz PRIVATE ${enet_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/external)
target_include_directories(noz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )

# Set warning flags only for noz library
if(MSVC)
    target_compile_options(noz PRIVATE /Wall /WX /wd4201 /wd4255 /wd4820 /wd4668 /wd5045 /wd4061 /wd4100)
else()
    target_compile_options(noz PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        -Wcast-align
        -Wcast-qual
        -Wconversion
        -Wdouble-promotion
        -Wfloat-equal
        -Wformat=2
        -Winit-self
        -Winline
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wmissing-prototypes
        -Wnested-externs
        -Wold-style-definition
        -Wpointer-arith
        -Wredundant-decls
        -Wshadow
        -Wstrict-prototypes
        -Wundef
        -Wunreachable-code
        -Wwrite-strings
        -Werror
    )
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

add_subdirectory(tools/importer)