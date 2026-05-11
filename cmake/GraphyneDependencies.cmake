# GraphyneDependencies.cmake
#
# Dependencies are now managed exclusively via the vcpkg manifest
# (`vcpkg.json` at the repository root). The previous ad-hoc SDL2
# download helper has been retired. This file is intentionally left
# in place so that older out-of-tree CMake includes don't error.
message(STATUS "GraphyneDependencies.cmake: no-op (dependencies are managed via vcpkg manifest)")
