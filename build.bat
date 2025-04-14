@echo off
setlocal

echo ===== Building Graphyne Engine with vcpkg =====

:: Set vcpkg variables if VCPKG_ROOT is defined
if defined VCPKG_ROOT (
    echo Using vcpkg from: %VCPKG_ROOT%
) else (
    echo VCPKG_ROOT environment variable is not set.
    echo Please set it to your vcpkg installation directory.
    echo For example: set VCPKG_ROOT=C:\vcpkg
    exit /b 1
)

:: Create build directory if it doesn't exist
if not exist "build" mkdir build

:: Navigate to build directory
cd build

:: Configure with CMake using vcpkg toolchain
echo Configuring project with CMake...
cmake -DCMAKE_BUILD_TYPE=Debug ^
      -DGRAPHYNE_BUILD_EXAMPLES=ON ^
      -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
      -DVCPKG_TARGET_TRIPLET=x64-windows ^
      ..

:: Build the graphyne library first
echo Building graphyne library...
cmake --build . --config Debug --target graphyne

:: Then build all targets (including examples)
echo Building all targets...
cmake --build . --config Debug

:: Check if build was successful
if %ERRORLEVEL% == 0 (
    echo Build completed successfully!
    echo Executables can be found in: %CD%\bin\Debug
) else (
    echo Build failed with error code: %ERRORLEVEL%
    echo Checking if graphyne library was built correctly...
    if exist "Debug\graphyne.lib" (
        echo graphyne.lib found at: %CD%\Debug\graphyne.lib
    ) else (
        echo graphyne.lib was not found in the expected location!
        echo Checking alternative locations...
        dir /s graphyne.lib
    )
)

:: Return to the original directory
cd ..

echo ===== Build process complete =====

:: Pause to see the output
pause
