#!/bin/bash

echo "===== Building Graphyne Engine ====="

# Create build directory if it doesn't exist
mkdir -p build

# Navigate to build directory
cd build

# Configure with CMake
echo "Configuring project with CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug -DGRAPHYNE_BUILD_EXAMPLES=ON ..

# Build the project
echo "Building project..."
cmake --build . --config Debug

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build completed successfully!"
    echo "Executables can be found in: $(pwd)/bin"
else
    echo "Build failed with error code: $?"
fi

# Return to the original directory
cd ..

echo "===== Build process complete ====="
