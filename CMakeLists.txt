cmake_minimum_required(VERSION 3.15)
project(bin2dash_project VERSION 1.0)

# Global configurations
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set global flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g3)
    add_link_options(-g)
else()
    add_compile_options(-O3 -DNDEBUG -w)
    add_link_options(-s)
endif()

# Include signals submodule
add_subdirectory(signals)

# Add the apps subdirectories
add_subdirectory(src/apps/bin2dash)
add_subdirectory(src/apps/bin2dash_app)
