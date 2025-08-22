cmake_minimum_required(VERSION 3.15)

project(cscratch)

include(FetchContent)

FetchContent_Declare(
  cglm
  GIT_REPOSITORY https://github.com/recp/cglm.git
  GIT_TAG        v0.9.4
)

FetchContent_Declare(
  uthash
  GIT_REPOSITORY https://github.com/troydhanson/uthash.git
  GIT_TAG        v2.3.0
)

FetchContent_MakeAvailable(cglm uthash)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
    
file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Create executable
add_executable(cscratch ${SOURCE_FILES})

# Link cglm
target_link_libraries(cscratch cglm)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT cscratch)

# Enable precompiled headers
target_precompile_headers(cscratch PUBLIC src/pch.h)

# Include directories
target_include_directories(cscratch PRIVATE src ${uthash_SOURCE_DIR}/src)

# Treat warnings as errors
if(MSVC)
    target_compile_options(cscratch PRIVATE /WX)
else()
    target_compile_options(cscratch PRIVATE -Werror)
endif()

