@echo off

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Configure with coverage enabled
cmake -B build -DGRAPHYNE_BUILD_TESTS=ON -DCOVERAGE=ON

REM Build the project
cmake --build build --config Debug

REM Run tests with detailed output
echo.
echo Running tests with detailed output:
echo ================================
.\build\bin\Debug\graphyne_tests.exe --gtest_color=yes

REM Generate coverage report if requested
if "%1"=="--coverage" (
    cmake --build build --target coverage --config Debug
    echo Coverage report generated in build\coverage_report\index.html
)
