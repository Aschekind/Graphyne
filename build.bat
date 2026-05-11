@echo off
setlocal enabledelayedexpansion

echo ===== Building Graphyne Engine =====

if not defined VCPKG_ROOT (
    if exist "%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake" (
        set "VCPKG_ROOT=%USERPROFILE%\vcpkg"
        echo VCPKG_ROOT is not set. Using default vcpkg install at: %USERPROFILE%\vcpkg
    ) else (
        echo VCPKG_ROOT is not set and no default vcpkg install was found.
        echo Set it to your vcpkg installation, for example:
        echo   set VCPKG_ROOT=C:\vcpkg
        exit /b 1
    )
)
echo Using vcpkg from: %VCPKG_ROOT%

set "CMAKE_EXE=cmake"
where cmake >nul 2>nul
if errorlevel 1 (
    if exist "%ProgramFiles%\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
    ) else if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=%ProgramFiles(x86)%\CMake\bin\cmake.exe"
    ) else (
        echo cmake is not available on PATH and no standard installation was found.
        echo Install CMake or add it to PATH, then try again.
        exit /b 1
    )
)

set "CTEST_EXE=ctest"
where ctest >nul 2>nul
if errorlevel 1 (
    if exist "%ProgramFiles%\CMake\bin\ctest.exe" (
        set "CTEST_EXE=%ProgramFiles%\CMake\bin\ctest.exe"
    ) else if exist "%ProgramFiles(x86)%\CMake\bin\ctest.exe" (
        set "CTEST_EXE=%ProgramFiles(x86)%\CMake\bin\ctest.exe"
    ) else (
        echo ctest is not available on PATH and no standard installation was found.
        echo Install CMake or add it to PATH, then try again.
        exit /b 1
    )
)

if not defined BUILD_TYPE  set "BUILD_TYPE=Debug"
if not defined BUILD_DIR   set "BUILD_DIR=build"

"%CMAKE_EXE%" -S . -B %BUILD_DIR% ^
      -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
      -DGRAPHYNE_BUILD_EXAMPLES=ON ^
      -DGRAPHYNE_BUILD_TESTS=ON ^
      -DVCPKG_TARGET_TRIPLET=x64-windows
if errorlevel 1 goto :error

"%CMAKE_EXE%" --build %BUILD_DIR% --config %BUILD_TYPE% --parallel
if errorlevel 1 goto :error

echo Running tests...
"%CTEST_EXE%" --test-dir %BUILD_DIR% -C %BUILD_TYPE% --output-on-failure
if errorlevel 1 goto :error

echo ===== Build complete =====
echo Binaries: %BUILD_DIR%\bin
exit /b 0

:error
echo Build failed with error code %errorlevel%.
exit /b %errorlevel%
