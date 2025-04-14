@echo off
setlocal

echo ===== Building Graphyne Engine with vcpkg =====

:: Check for VCPKG_ROOT
if defined VCPKG_ROOT (
    echo Using vcpkg from: %VCPKG_ROOT%
) else (
    echo [ERROR] VCPKG_ROOT environment variable is not set.
    echo Please set it to your vcpkg installation directory.
    echo For example: set VCPKG_ROOT=C:\vcpkg
    exit /b 1
)

:: Check for Vulkan SDK
if defined VULKAN_SDK (
    echo Vulkan SDK found at: %VULKAN_SDK%
) else (
    echo [WARNING] VULKAN_SDK environment variable not set.
    echo Vulkan SDK may not be installed correctly.
    echo You can get it from: https://vulkan.lunarg.com/sdk/home
)

:: Create build directory if it doesn't exist
if not exist "build" mkdir build

:: Navigate to build directory
cd build

:: Configure with CMake using vcpkg toolchain
echo Configuring project with CMake...
cmake -DCMAKE_BUILD_TYPE=Debug ^
      -DGRAPHYNE_BUILD_EXAMPLES=ON ^
      -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
      -DVCPKG_TARGET_TRIPLET=x64-windows ^
      ..

:: Check if CMake configuration succeeded
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

:: Build the Graphyne library
echo Building Graphyne library...
cmake --build . --config Debug --target graphyne

:: Build all targets
echo Building all targets...
cmake --build . --config Debug

:: Check if build was successful
if %ERRORLEVEL% == 0 (
    echo [SUCCESS] Build completed successfully!
    echo Executables can be found in: %CD%\bin\Debug
) else (
    echo [ERROR] Build failed with error code: %ERRORLEVEL%
    echo Checking if graphyne.lib was built...
    if exist "Debug\graphyne.lib" (
        echo graphyne.lib found at: %CD%\Debug\graphyne.lib
    ) else (
        echo [ERROR] graphyne.lib not found!
        echo Searching for graphyne.lib in subdirectories...
        dir /s /b graphyne.lib
    )
)

:: Go back to root directory
cd ..

echo ===== Build process complete =====

:: Pause to see output
pause
