cmake_minimum_required(VERSION 3.16)

# Project name and version
project(amp VERSION 1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2)
find_package(SDL2_mixer)
find_package(TagLib)

# Add include directory
include_directories(include)

# Add source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
  ${PROJECT_NAME}
  ${SDL2_LIBRARIES}
  SDL2_mixer::SDL2_mixer
  TagLib::TagLib
)
