# Utility functions for Graphyne CMake build

# Function to enforce project architecture standards
function(graphyne_set_target_properties target)
    # Set extra properties for all targets
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )

    # IDE-specific properties
    set_target_properties(${target} PROPERTIES
        FOLDER ${PROJECT_NAME}
    )

    # MSVC-specific settings
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /MP     # Multi-processor compilation
            /bigobj # Increase object file sections
        )
    endif()
endfunction()

# Function to enable stricter warnings
function(graphyne_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4     # Warning level 4
            /WX     # Treat warnings as errors
            /wd4251 # Disable warning about dll-interface
            /wd4275 # Disable warning about non-dll-interface base class
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror         # Treat warnings as errors
            -Wno-unused-parameter
        )
    endif()
endfunction()

# Function to add include directories for a target with proper interface
function(graphyne_include_directories target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "PUBLIC;PRIVATE;INTERFACE")

    if(ARG_PUBLIC)
        target_include_directories(${target} PUBLIC ${ARG_PUBLIC})
    endif()

    if(ARG_PRIVATE)
        target_include_directories(${target} PRIVATE ${ARG_PRIVATE})
    endif()

    if(ARG_INTERFACE)
        target_include_directories(${target} INTERFACE ${ARG_INTERFACE})
    endif()
endfunction()

# Function to create proper install targets for libraries
function(graphyne_install_library target)
    include(GNUInstallDirs)

    install(TARGETS ${target}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endfunction()
