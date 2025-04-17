#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p build

# Configure with coverage enabled
cmake -B build -DGRAPHYNE_BUILD_TESTS=ON -DCOVERAGE=ON

# Build the project
cmake --build build

# Run tests with detailed output
echo
echo "Running tests with detailed output:"
echo "================================"
./build/bin/graphyne_tests --gtest_color=yes

# Generate coverage report if requested
if [ "$1" == "--coverage" ]; then
    cmake --build build --target coverage
    echo "Coverage report generated in build/coverage_report/index.html"
fi
