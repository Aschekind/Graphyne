# Unit tests for Graphyne
#
# Uses GoogleTest. All tests are linked into a single executable
# `graphyne_tests` so coverage runs in one process.

find_package(GTest REQUIRED)
include(GoogleTest)

set(GRAPHYNE_TEST_SOURCES
    core/test_handle.cpp
    core/test_result.cpp
    core/test_slot_map.cpp
    core/test_clock.cpp

    memory/test_arena.cpp
    memory/test_pool.cpp

    utils/test_logger.cpp
)

add_executable(graphyne_tests ${GRAPHYNE_TEST_SOURCES})

target_link_libraries(graphyne_tests
    PRIVATE
        graphyne::graphyne
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(graphyne_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/project/src
)

set_target_properties(graphyne_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

gtest_discover_tests(graphyne_tests
    DISCOVERY_TIMEOUT 30
    PROPERTIES LABELS "unit"
)
