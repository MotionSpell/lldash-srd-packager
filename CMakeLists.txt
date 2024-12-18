cmake_minimum_required(VERSION 3.15)

project(EncodingEncapsulation VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEBUG "Enable debug mode" OFF)

if(DEBUG)
    add_compile_definitions(DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# Define the source directory for the EncodingEncapsulation
set(ENCAPS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Include the signals library as an external project
set(SIGNALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/signals)

# Add the signals build system 
add_subdirectory(${SIGNALS_DIR} signals_build)

# Add bin2dash executable
add_executable(bin2dash
    ${ENCAPS_SRC}/apps/bin2dash/bin2dash.cpp
)

target_include_directories(bin2dash
    PRIVATE ${SIGNALS_DIR}/include
    PRIVATE ${ENCAPS_SRC}/apps/bin2dash
)

target_link_libraries(bin2dash
    PRIVATE signals
)

# Add bin2dash_app executable
add_executable(bin2dash_app
    ${ENCAPS_SRC}/apps/bin2dash_app/main.cpp
)

target_include_directories(bin2dash_app
    PRIVATE ${SIGNALS_DIR}/include
)

target_link_libraries(bin2dash_app
    PRIVATE signals
)


