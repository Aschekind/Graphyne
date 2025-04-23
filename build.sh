#!/bin/bash

echo "===== Building Graphyne Engine ====="

# Check for required dependencies
echo "Checking for required dependencies..."
if ! command -v cmake &> /dev/null; then
    echo "CMake not found. Please install CMake."
    exit 1
fi

if ! command -v vulkaninfo &> /dev/null; then
    echo "Vulkan SDK not found. Please install Vulkan SDK."
    echo "You can get it from: https://vulkan.lunarg.com/sdk/home"
    exit 1
fi

# Check for vcpkg
if [ -z "$VCPKG_ROOT" ]; then
    echo "[ERROR] VCPKG_ROOT environment variable is not set."
    echo "Please set it to your vcpkg installation directory."
    echo "For example: export VCPKG_ROOT=\$HOME/vcpkg"
    exit 1
fi

# Install dependencies using manifest
echo "Installing dependencies with vcpkg..."
"$VCPKG_ROOT/vcpkg" install --triplet x64-linux

# Create build directory if it doesn't exist
mkdir -p build

# Navigate to build directory
cd build

# Configure with CMake
echo "Configuring project with CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DGRAPHYNE_BUILD_EXAMPLES=ON \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
      -DVCPKG_TARGET_TRIPLET=x64-linux \
      -DBUILD_TESTING=ON \
      -DGTEST_ROOT="$VCPKG_ROOT/installed/x64-linux" \
      ..

# Check if CMake configuration succeeded
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed."
    exit 1
fi

# Build the Graphyne library first
echo "Building Graphyne library..."
cmake --build . --config Debug --target graphyne

# Check if the library was built
if [ ! -f "libgraphyne.a" ] && [ ! -f "libgraphyne.so" ]; then
    echo "[ERROR] Graphyne library not found after building!"
    echo "Searching for library in subdirectories..."
    find . -name "libgraphyne.*"
    exit 1
fi

# Build all targets
echo "Building all targets..."
cmake --build . --config Debug -j $(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "[SUCCESS] Build completed successfully!"
    echo "Executables can be found in: $(pwd)/bin"
else
    echo "[ERROR] Build failed with error code: $?"
    exit 1
fi

# Run tests if they exist
if [ -f "test/graphyne_test" ]; then
    echo "Running tests..."
    ./test/graphyne_test
fi

# Return to the original directory
cd ..

echo "===== Build process complete ====="
