# Examples CMakeLists.txt

# Add an executable for the simple window example
add_executable(simple_window simple_window.cpp)

target_include_directories(simple_window
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src  # This gives direct access to core/engine.h
)

# Link the example against the graphyne engine library
target_link_libraries(simple_window
    PRIVATE
        graphyne
)

# Make sure the example depends on the graphyne library
add_dependencies(simple_window graphyne)

# Set output directory (this should inherit from parent, but explicitly set to be sure)
set_target_properties(simple_window PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
