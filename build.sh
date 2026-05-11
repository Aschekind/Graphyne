#!/usr/bin/env bash
set -euo pipefail

echo "===== Building Graphyne Engine ====="

BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-build}"
COVERAGE="${COVERAGE:-OFF}"

cmake -S . -B "${BUILD_DIR}" \
    -G "${CMAKE_GENERATOR:-Ninja}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DGRAPHYNE_BUILD_EXAMPLES=ON \
    -DGRAPHYNE_BUILD_TESTS=ON \
    -DGRAPHYNE_COVERAGE="${COVERAGE}"

cmake --build "${BUILD_DIR}" --parallel

echo "Running tests..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

if [[ "${COVERAGE}" == "ON" ]]; then
    cmake --build "${BUILD_DIR}" --target coverage
    echo "Coverage report: ${BUILD_DIR}/coverage/html/index.html"
fi

echo "===== Build process complete ====="
echo "Binaries: ${BUILD_DIR}/bin"
