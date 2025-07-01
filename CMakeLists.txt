cmake_minimum_required(VERSION 3.16)
project(software-renderer)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in lib/SDL (as a submodule)
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

# Create your game executable target as usual
add_executable(software-renderer src/main.cpp src/vec2.cpp)
target_include_directories(software-renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link to the actual SDL3 library.
target_link_libraries(software-renderer PRIVATE SDL3::SDL3)
