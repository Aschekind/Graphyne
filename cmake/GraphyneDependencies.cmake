# Dependencies management for Graphyne

include(FetchContent)

# Set options for FetchContent
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Function to add a dependency from GitHub
function(graphyne_add_github_dependency name repo tag)
    FetchContent_Declare(
        ${name}
        GIT_REPOSITORY https://github.com/${repo}.git
        GIT_TAG ${tag}
        GIT_SHALLOW ON
    )

    FetchContent_MakeAvailable(${name})
endfunction()

# Function to handle SDL2 setup on Windows
function(graphyne_setup_sdl2)
    if(WIN32 AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2")
        message(STATUS "Setting up SDL2 for Windows...")

        # Create directories if they don't exist
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2")

        # Set SDL2 version
        set(SDL2_VERSION "2.28.5")

        # Download SDL2 development libraries for Windows
        file(DOWNLOAD
            "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-VC.zip"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-devel-${SDL2_VERSION}-VC.zip"
            SHOW_PROGRESS
        )

        # Extract the SDL2 development libraries
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-devel-${SDL2_VERSION}-VC.zip"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external"
        )

        # Move the extracted files to the SDL2 directory
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-${SDL2_VERSION}" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/external")
        file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-${SDL2_VERSION}" "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2")

        # Clean up the downloaded zip file
        file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-devel-${SDL2_VERSION}-VC.zip")

        message(STATUS "SDL2 setup complete")
    endif()
endfunction()

# Set up dependencies
graphyne_setup_sdl2()
