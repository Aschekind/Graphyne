cmake_minimum_required(VERSION 3.20)

# Set project information
project(
    Graphyne
    VERSION 0.1.0
    DESCRIPTION "A lightweight 3D game engine with a focus on low-level architecture and performance"
    HOMEPAGE_URL "https://github.com/Multyp/graphyne"
    LANGUAGES C CXX
)

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set global output directories for all targets
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable vcpkg integration if it exists
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(SANITIZE_FLAGS -fsanitize=address -g -O1)
    add_compile_options(${SANITIZE_FLAGS})
    add_link_options(${SANITIZE_FLAGS})
endif()

# Set build type to Debug by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# Options
option(GRAPHYNE_BUILD_EXAMPLES "Build example applications" ON)
option(GRAPHYNE_BUILD_TESTS "Build tests" ON)
option(GRAPHYNE_USE_ASAN "Enable Address Sanitizer" OFF)
option(GRAPHYNE_USE_CLANG_TIDY "Enable clang-tidy" OFF)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GraphyneUtils)
include(GraphyneDependencies)

# Add compiler flags
if(MSVC)
    # MSVC-specific flags
    add_compile_options(
        /W4             # Warning level 4
        /MP             # Multi-processor compilation
        /Zi             # Debug information format
        /permissive-    # Standards conformance
        /EHsc           # Exception handling model
        /Zc:__cplusplus # Correct __cplusplus macro
        /utf-8          # UTF-8 source files
    )
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX        # Prevent Windows.h from defining min/max macros
        WIN32_LEAN_AND_MEAN
    )
else()
    # GCC/Clang flags
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Wno-unused-parameter
    )
endif()

# Address Sanitizer setup
if(GRAPHYNE_USE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Clang-Tidy setup
if(GRAPHYNE_USE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
        message(STATUS "Clang-Tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "Clang-Tidy not found")
    endif()
endif()

# Find dependencies using vcpkg
find_package(Vulkan REQUIRED)

# Set SDL2 paths for custom installation
set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2/cmake")
set(SDL2_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2")
set(SDL2_INCLUDE_DIRS "${SDL2_PATH}/include")
set(SDL2_LIBRARIES "${SDL2_PATH}/lib/x64/SDL2.lib" "${SDL2_PATH}/lib/x64/SDL2main.lib")

find_package(SDL2 CONFIG REQUIRED)
set(SPDLOG_HEADER_ONLY ON CACHE BOOL "" FORCE)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
set(FMT_HEADER_ONLY ON CACHE BOOL "" FORCE)
find_package(fmt CONFIG REQUIRED)


# Define the main library
add_library(graphyne
    project/src/core/engine.cpp
    project/src/core/memory.cpp
    project/src/core/vulkan_resource_manager.cpp
    project/src/core/ecs/ecs.cpp
    project/src/events/event_system.cpp
    project/src/graphics/renderer.cpp
    project/src/graphics/vulkan_renderer.cpp
    project/src/utils/logger.cpp
    project/src/platform/window.cpp
    project/src/controls/input_system.cpp
)

# Explicitly set the output directory for the graphyne library
set_target_properties(graphyne PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "d"  # Add 'd' suffix for debug builds
)

target_include_directories(graphyne
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/project/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/project/src
)

target_link_libraries(graphyne
    PUBLIC
        Vulkan::Vulkan
        SDL2::SDL2
        SDL2::SDL2main
        spdlog::spdlog_header_only
        glm::glm
        fmt::fmt-header-only
)

# Add examples if enabled
if(GRAPHYNE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Add tests if enabled
if(GRAPHYNE_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS graphyne
    EXPORT graphyne-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/graphyne-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT graphyne-targets
    FILE graphyne-targets.cmake
    NAMESPACE graphyne::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/graphyne
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/graphyne-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/graphyne-config.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/graphyne-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/graphyne-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/graphyne
)

message(STATUS "Graphyne configuration complete")
