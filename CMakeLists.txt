cmake_minimum_required(VERSION 3.15)

project(lldash-packager VERSION 1.0 LANGUAGES CXX)


# Define the source directory for lldash-packager
set(LLDASH_PACKAGER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
    set(LLDASH_PACKAGER_LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${ LLDASH_PACKAGER_SRC}/apps/lldash_packager/plugin.version")

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
    set(LLDASH_PACKAGER_LINK_FLAGS "")
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
    set(LLDASH_PACKAGER_LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${LLDASH_PACKAGER_SRC}/apps/lldash_packager/plugin.version")
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

# Add lldash-packager shared library
add_library(lldash-packager SHARED
    ${LLDASH_PACKAGER_SRC}/apps/lldash_packager/lldash_packager.cpp
)

target_include_directories(lldash-packager
    PRIVATE ${LLDASH_PACKAGER_SRC}/apps/lldash_packager
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(lldash-packager
    PRIVATE signals::media
    PRIVATE signals::modules
    PRIVATE signals::pipeline
    PRIVATE signals::utils
)

set_target_properties(lldash-packager PROPERTIES
    LINK_FLAGS "${LLDASH_PACKAGER_LINK_FLAGS}"
)

signals_install_plugin(lldash-packager ".so")

# Add lldash-packager_app executable
add_executable(lldash-packager-app
    ${LLDASH_PACKAGER_SRC}/apps/lldash_packager_app/main.cpp
)

target_include_directories(lldash-packager-app
    PRIVATE ${LLDASH_PACKAGER_SRC}/apps/lldash_packager_app
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(lldash-packager-app
    PRIVATE signals::appcommon
    PRIVATE signals::utils
    PRIVATE  lldash-packager
)

signals_install_app(lldash-packager-app)
