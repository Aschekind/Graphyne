cmake_minimum_required(VERSION 3.20)

# Enable testing
enable_testing()

# Find GTest package
find_package(GTest REQUIRED)

# Add coverage flags if coverage is enabled
if(COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
endif()

# Create test executable
add_executable(graphyne_tests
    main.cpp
    core/engine_test.cpp
)

# Set include directories
target_include_directories(graphyne_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/project/include
)

# Link with GTest and the main library
target_link_libraries(graphyne_tests
    PRIVATE
        GTest::GTest
        GTest::Main
        graphyne
)

# Add test
add_test(NAME graphyne_tests COMMAND graphyne_tests)

# Add coverage target if coverage is enabled
if(COVERAGE)
    find_program(LCOV_EXE NAMES lcov)
    find_program(GENHTML_EXE NAMES genhtml)

    if(LCOV_EXE AND GENHTML_EXE)
        add_custom_target(coverage
            COMMAND ${LCOV_EXE} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_EXE} --remove coverage.info '*/tests/*' '*/external/*' '*/vcpkg_installed/*' --output-file coverage.info
            COMMAND ${GENHTML_EXE} coverage.info --output-directory coverage_report
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating coverage report"
        )
    endif()
endif()
