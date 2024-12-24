cmake_minimum_required(VERSION 3.15)

project(bin2dash VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEBUG "Enable debug mode" OFF)

if(DEBUG)
    add_compile_definitions(DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# Define the source directory for bin2dash
set(BIN2DASH_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add bin2dash shared library
add_library(bin2dash SHARED
    ${BIN2DASH_SRC}/apps/bin2dash/bin2dash.cpp
)

target_include_directories(bin2dash
    PRIVATE ${BIN2DASH_SRC}/apps/bin2dash
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(bin2dash
    PRIVATE signals::media
    PRIVATE signals::modules
    PRIVATE signals::pipeline
    PRIVATE signals::utils
)

set_target_properties(bin2dash PROPERTIES
    OUTPUT_NAME "bin2dash"
    PREFIX ""
    SUFFIX ".so"
    LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${BIN2DASH_SRC}/apps/bin2dash/plugin.version"
)

# Add bin2dash_app executable
add_executable(bin2dash_app
    ${BIN2DASH_SRC}/apps/bin2dash_app/main.cpp
)

target_include_directories(bin2dash_app
    PRIVATE ${BIN2DASH_SRC}/apps/bin2dash_app
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(bin2dash_app
    PRIVATE signals::appcommon
    PRIVATE signals::utils
    PRIVATE bin2dash
)

set_target_properties(bin2dash_app PROPERTIES
    OUTPUT_NAME "bin2dash_app"
    SUFFIX ".exe"
)

# Install bin2dash shared library
install(TARGETS bin2dash
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install bin2dash_app executable
install(TARGETS bin2dash_app
    RUNTIME DESTINATION bin
)


