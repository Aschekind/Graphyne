# Examples

function(graphyne_add_example name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE graphyne::graphyne)
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endfunction()

graphyne_add_example(simple_window simple_window.cpp)
