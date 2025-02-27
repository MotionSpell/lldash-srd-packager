cmake_minimum_required(VERSION 3.15)

project(bin2dash VERSION 1.0 LANGUAGES CXX)


# Define the source directory for bin2dash
set(BIN2DASH_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEBUG "Enable debug mode" OFF)

if(DEBUG)
    add_compile_definitions(DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# Change some compiler options that are dependent on the target platform.
# xxxjack some are actually dependent on the toolchain...
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_compile_options(-Wall -Wextra -Werror -fvisibility=default -fvisibility-inlines-hidden -Wno-deprecated-declarations)
    set(BIN2DASH_LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${BIN2DASH_SRC}/apps/bin2dash/plugin.version")

    # Debug-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3)
    endif()

    # Release-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-w -DNDEBUG)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    add_compile_options(-Wall -Wextra -Werror -fvisibility=default -fvisibility-inlines-hidden -Wno-deprecated-declarations)
    set(BIN2DASH_LINK_FLAGS "")
    # Debug-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3)
    endif()

    # Release-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-w -DNDEBUG)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_compile_options(-Wall -Wextra -Werror -fvisibility=default -fvisibility-inlines-hidden -Wno-deprecated-declarations)
    set(BIN2DASH_LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${BIN2DASH_SRC}/apps/bin2dash/plugin.version")
    # Debug-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3)
    endif()

    # Release-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-w -DNDEBUG)
    endif()
else()
    message(FATAL_ERROR " Unknown CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
endif()

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
    LINK_FLAGS "${BIN2DASH_LINK_FLAGS}"
)

signals_install_plugin(bin2dash ".so")

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

signals_install_app(bin2dash_app)
