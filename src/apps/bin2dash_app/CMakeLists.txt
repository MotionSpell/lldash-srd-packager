# Define the executable
add_executable(bin2dash_app
    main.cpp
)

# Link dependencies
target_link_libraries(bin2dash_app PRIVATE
    appcommon
    utils
    bin2dash
)

# Include directories from signals
target_include_directories(bin2dash_app PRIVATE
    ${CMAKE_SOURCE_DIR}/signals/src
)

# Set target properties
set_target_properties(bin2dash_app PROPERTIES
    OUTPUT_NAME "bin2dash_app"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    SUFFIX ".exe"
)

# Link bin2dash as a shared library
target_link_directories(bin2dash_app PRIVATE
    ${CMAKE_BINARY_DIR}/bin
)
