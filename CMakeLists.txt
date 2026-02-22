cmake_minimum_required(VERSION 3.10)
project(FusionBox)

set(CMAKE_CXX_STANDARD 17)

# Find both libraries
find_package(SFML REQUIRED COMPONENTS Graphics Window System)
find_package(box2d REQUIRED)

add_executable(FusionBox main.cpp)

# Link both libraries
target_link_libraries(FusionBox SFML::Graphics SFML::Window SFML::System box2d)