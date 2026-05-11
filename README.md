![Graphyne Header](./graphyne-banner.png)

# Graphyne

**Graphyne** is a lightweight game engine project built from the ground up,
with a strong focus on low-level architecture and performance. It is currently
in its early stages and aims to explore modern rendering techniques,
multithreaded systems, and modular engine design.

> ⚙️ *Personal R&D project — a stepping stone toward a long-term goal:
> a custom, proprietary engine for creative and indie development.*

## Status

| Subsystem            | State                                  |
| -------------------- | -------------------------------------- |
| Window + input (SDL) | ✅ Working                              |
| Logging (spdlog)     | ✅ Working                              |
| Memory (arena/pool)  | ✅ Working                              |
| Vulkan RHI 1.3       | ✅ Clear-color frame                    |
| Coverage + CI        | ✅ Linux lcov, GitHub Actions           |
| Textures + sprites   | 🚧 Next milestone                       |
| ECS / scenes         | 🚧 Next milestone                       |

## Building

Requirements: CMake ≥ 3.20, a C++20 compiler, Vulkan SDK 1.3, and either vcpkg
or system packages for SDL2, fmt, spdlog, glm, and (for tests) GoogleTest.

### First-time setup

One-shot scripts handle the prerequisites:

```bash
# Linux / macOS  (apt / dnf / pacman / brew; falls back to vcpkg)
./scripts/setup.sh
./scripts/setup.sh --vcpkg            # use vcpkg manifest mode instead

# Windows (PowerShell)
pwsh -ExecutionPolicy Bypass -File scripts\setup.ps1
```

Both scripts are idempotent — re-running them only installs what's missing.

### Quick build (Linux / macOS)

```bash
./build.sh
```

### With coverage report

```bash
COVERAGE=ON ./build.sh
xdg-open build/coverage/html/index.html
```

### Windows (vcpkg)

```cmd
set VCPKG_ROOT=C:\path\to\vcpkg
build.bat
```

### CMake presets

```bash
cmake --preset default   # ninja + debug + tests
cmake --preset coverage  # adds gcov instrumentation
cmake --preset asan      # AddressSanitizer
cmake --preset release   # optimized
```

## Usage

Inherit from `gn::App`, override the hooks you need, and hand the app to
the engine:

```cpp
#include "core/app.h"
#include "core/engine.h"

class MyGame final : public gn::App {
    void on_update(gn::f32 dt) override { /* … */ }
    void on_render(gn::graphics::Frame& frame) override { /* … */ }
};

int main() {
    gn::Engine::Config cfg;
    cfg.app_name = "My Game";
    cfg.window_width = 1280;
    cfg.window_height = 720;

    gn::Engine engine(cfg);
    MyGame app;
    return engine.run(app);
}
```

See `examples/simple_window.cpp` for a runnable demo.

## Project layout

```
project/
  include/          public headers (installed)
    core/           types, result, handle, slot_map, clock, app, engine
    memory/         arena, pool
    platform/       platform (SDL), window, input, event, key_codes
    rhi/            Vulkan device, swapchain, command, sync
    graphics/       color, frame, renderer
    utils/          logger, assert
  src/              implementations
tests/              GoogleTest unit tests
examples/           sample apps
cmake/              CMake modules (utils, coverage)
.github/workflows/  CI definitions
```

## License

BSD 3-Clause with a custom clause: commercial use is allowed **only** for
game development; use of this software for creating other commercial game
engines is **not permitted**. See [LICENSE](./LICENSE).
