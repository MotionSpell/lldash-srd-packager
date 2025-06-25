# Define the shared library
add_library(bin2dash SHARED
    bin2dash.cpp
)

# Link signals libraries
target_link_libraries(bin2dash PRIVATE
    media
    modules
    pipeline
    utils
)

# Include directories from signals
target_include_directories(bin2dash PRIVATE
    ${CMAKE_SOURCE_DIR}/signals/src
)

# Set target properties
set_target_properties(bin2dash PROPERTIES
    OUTPUT_NAME "bin2dash"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    PREFIX ""  #
    SUFFIX ".so"
)

# Add plugin version script
target_link_options(bin2dash PRIVATE
    -Wl,--no-undefined
    -Wl,--version-script=${CMAKE_SOURCE_DIR}/src/apps/bin2dash/plugin.version
)
