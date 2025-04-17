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

# Set up dependencies
# SDL2 is managed by vcpkg, no need for manual setup
