cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# vcpkg toolchain bootstrap (must happen before project())
# ---------------------------------------------------------------------------
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(
    Graphyne
    VERSION 0.2.0
    DESCRIPTION "A lightweight game engine with a focus on low-level architecture and performance"
    HOMEPAGE_URL "https://github.com/Multyp/graphyne"
    LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(GRAPHYNE_BUILD_EXAMPLES "Build example applications" ON)
option(GRAPHYNE_BUILD_TESTS    "Build unit tests"           ON)
option(GRAPHYNE_USE_ASAN       "Enable Address Sanitizer"   OFF)
option(GRAPHYNE_USE_CLANG_TIDY "Enable clang-tidy"          OFF)
option(GRAPHYNE_COVERAGE       "Enable code coverage (Debug, GCC/Clang)" OFF)
option(GRAPHYNE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"   OFF)

# ---------------------------------------------------------------------------
# CMake modules
# ---------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GraphyneUtils)
include(GraphyneCoverage)

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(
        /W4
        /MP
        /permissive-
        /EHsc
        /Zc:__cplusplus
        /utf-8
    )
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(GRAPHYNE_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wno-unused-parameter
    )
    if(GRAPHYNE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

# Address Sanitizer
if(GRAPHYNE_USE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Clang-Tidy
if(GRAPHYNE_USE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
        message(STATUS "Clang-Tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "Clang-Tidy requested but not found")
    endif()
endif()

# Coverage (applies global flags to all targets)
if(GRAPHYNE_COVERAGE)
    graphyne_enable_coverage_flags()
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------
set(GRAPHYNE_SOURCES
    project/src/core/engine.cpp
    project/src/core/clock.cpp
    project/src/core/version.cpp

    project/src/memory/arena.cpp
    project/src/memory/pool.cpp

    project/src/platform/platform.cpp
    project/src/platform/window.cpp
    project/src/platform/input.cpp

    project/src/rhi/device.cpp
    project/src/rhi/swapchain.cpp
    project/src/rhi/command.cpp
    project/src/rhi/sync.cpp

    project/src/graphics/renderer.cpp

    project/src/utils/logger.cpp
)

add_library(graphyne ${GRAPHYNE_SOURCES})
add_library(graphyne::graphyne ALIAS graphyne)

set_target_properties(graphyne PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "d"
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
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        spdlog::spdlog
        glm::glm
        fmt::fmt
)

target_compile_features(graphyne PUBLIC cxx_std_20)

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------
if(GRAPHYNE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(GRAPHYNE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Coverage target
# ---------------------------------------------------------------------------
if(GRAPHYNE_COVERAGE AND GRAPHYNE_BUILD_TESTS)
    graphyne_add_coverage_target(
        NAME coverage
        TEST_TARGET graphyne_tests
        EXCLUDE
            "/usr/*"
            "*/_deps/*"
            "*/tests/*"
            "*/examples/*"
    )
endif()

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS graphyne
    EXPORT graphyne-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY project/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

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

message(STATUS "Graphyne configuration:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build examples:    ${GRAPHYNE_BUILD_EXAMPLES}")
message(STATUS "  Build tests:       ${GRAPHYNE_BUILD_TESTS}")
message(STATUS "  Coverage:          ${GRAPHYNE_COVERAGE}")
message(STATUS "  ASan:              ${GRAPHYNE_USE_ASAN}")
message(STATUS "  Clang-Tidy:        ${GRAPHYNE_USE_CLANG_TIDY}")
message(STATUS "  Werror:            ${GRAPHYNE_WARNINGS_AS_ERRORS}")
