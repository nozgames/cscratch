cmake_minimum_required(VERSION 3.15)

project(cscratch)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
    
add_subdirectory(libs/noz noz)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Create executable
add_executable(cscratch ${SOURCE_FILES})
target_link_libraries(cscratch)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT cscratch)
target_precompile_headers(cscratch PUBLIC src/pch.h)


target_link_libraries(cscratch noz)
set_target_properties(cscratch PROPERTIES WIN32_EXECUTABLE TRUE)