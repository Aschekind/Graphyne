# GraphyneCoverage.cmake
#
# Code coverage support using gcov / llvm-cov and lcov for report generation.
# Linux/macOS only; on Windows the coverage flags are silently ignored.
#
# Usage:
#   cmake -B build -DGRAPHYNE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build --target coverage
#
# The "coverage" target runs the test executable and then generates:
#   - build/coverage/coverage.info        (raw lcov tracefile)
#   - build/coverage/coverage.filtered.info (filtered tracefile)
#   - build/coverage/html/index.html       (genhtml report)

include_guard(GLOBAL)

function(graphyne_enable_coverage_flags)
    if(MSVC)
        message(WARNING "GRAPHYNE_COVERAGE is not supported on MSVC. Use --coverage with clang-cl manually if needed.")
        return()
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        message(WARNING "GRAPHYNE_COVERAGE works best with CMAKE_BUILD_TYPE=Debug (current: ${CMAKE_BUILD_TYPE}).")
    endif()

    add_compile_options(--coverage -O0 -g -fno-inline -fno-inline-small-functions -fno-default-inline)
    add_link_options(--coverage)
    message(STATUS "Coverage instrumentation enabled (--coverage)")
endfunction()


# graphyne_add_coverage_target(NAME <target> TEST_TARGET <test_exe> [EXCLUDE <pattern>...])
function(graphyne_add_coverage_target)
    set(options)
    set(oneValueArgs NAME TEST_TARGET)
    set(multiValueArgs EXCLUDE)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_NAME OR NOT ARG_TEST_TARGET)
        message(FATAL_ERROR "graphyne_add_coverage_target: NAME and TEST_TARGET are required")
    endif()

    find_program(LCOV_EXE lcov)
    find_program(GENHTML_EXE genhtml)

    if(NOT LCOV_EXE OR NOT GENHTML_EXE)
        message(WARNING "lcov / genhtml not found — coverage target will be a stub. "
                        "Install lcov (e.g. 'apt-get install lcov') to enable HTML reports.")
        add_custom_target(${ARG_NAME}
            COMMAND ${CMAKE_COMMAND} -E echo "lcov/genhtml not installed; cannot generate coverage report."
            COMMAND ${CMAKE_COMMAND} -E false
        )
        return()
    endif()

    set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(RAW_INFO "${COVERAGE_DIR}/coverage.info")
    set(FILTERED_INFO "${COVERAGE_DIR}/coverage.filtered.info")
    set(HTML_DIR "${COVERAGE_DIR}/html")

    # Build the exclude argument list for lcov
    set(EXCLUDE_ARGS "")
    foreach(pat IN LISTS ARG_EXCLUDE)
        list(APPEND EXCLUDE_ARGS "${pat}")
    endforeach()

    add_custom_target(${ARG_NAME}
        # Make sure binaries are built and instrumented
        DEPENDS ${ARG_TEST_TARGET}

        # Reset counters
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
        COMMAND ${LCOV_EXE} --directory ${CMAKE_BINARY_DIR} --zerocounters --quiet

        # Capture baseline (zero counts so files with 0 hits still appear)
        COMMAND ${LCOV_EXE} --directory ${CMAKE_BINARY_DIR}
                            --capture --initial
                            --output-file ${COVERAGE_DIR}/baseline.info
                            --rc lcov_branch_coverage=1
                            --quiet

        # Run tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}

        # Capture test counts
        COMMAND ${LCOV_EXE} --directory ${CMAKE_BINARY_DIR}
                            --capture
                            --output-file ${COVERAGE_DIR}/test.info
                            --rc lcov_branch_coverage=1
                            --quiet

        # Combine baseline + test
        COMMAND ${LCOV_EXE} --add-tracefile ${COVERAGE_DIR}/baseline.info
                            --add-tracefile ${COVERAGE_DIR}/test.info
                            --output-file ${RAW_INFO}
                            --rc lcov_branch_coverage=1
                            --quiet

        # Filter out system / external / test code
        COMMAND ${LCOV_EXE} --remove ${RAW_INFO} ${EXCLUDE_ARGS}
                            --output-file ${FILTERED_INFO}
                            --rc lcov_branch_coverage=1
                            --quiet

        # Render
        COMMAND ${GENHTML_EXE} ${FILTERED_INFO}
                               --output-directory ${HTML_DIR}
                               --branch-coverage
                               --title "Graphyne Coverage"
                               --legend
                               --demangle-cpp
                               --quiet
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report: ${HTML_DIR}/index.html"

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
        COMMENT "Generating code coverage report (target: ${ARG_NAME})"
    )
endfunction()
