# 0003. SDL3 for windowing, raw Vulkan for graphics

- Status: Accepted
- Date: 2026-10-29

## Context

Graphyne was started on SDL2 + raw Vulkan 1.3 in 2024 without a firm direction. Its goal is learning and
experimenting with low-level engine design, with code that stays small and understandable. CI minutes are
budgeted, so heavy dependencies cost money on every cold build.

- SDL2 is in maintenance mode; upstream points to SDL3
  (<https://www.phoronix.com/news/SDL2-To-Maintenance-Mode>, <https://wiki.libsdl.org/SDL3/SourceCode>).
  SDL3 3.4.18 is in vcpkg at our baseline. Graphyne's SDL code is about 230 lines in `platform/`, plus the
  surface call in `rhi/device.cpp`.
- The raw Vulkan layer works: about 1,100 lines on `develop` (Vulkan 1.3, dynamic rendering,
  synchronization2, validation layers), plus about 1,300 lines of sprite code on `feat/texture-rendering`.

Options considered for graphics:

| Option | Code kept | Learning value | Cost |
| --- | --- | --- | --- |
| Raw Vulkan 1.3 | All | Highest | Barriers, memory and descriptors written by hand |
| SDL3 GPU API | Platform only | Medium (barriers, swapchain hidden) | Rewrite `rhi/` and the renderer; shaders through SDL_shadercross (no vcpkg port at our baseline) |
| NVRHI over Vulkan | Some | Medium-high | Rewrite `rhi/`; design tied to its abstractions |
| WebGPU (Dawn) | Platform only | Medium | Heavy build (abseil, Python at build time) |
| bgfx, Diligent | Platform only | Low | Large dependencies that hide the API |
| OpenGL 4.6 | Platform only | Low | No future on macOS |

GLFW was considered for windowing: smaller, but no audio, so a second library would follow.

## Decision

- **Windowing and input: SDL3**, replacing SDL2.
- **Graphics: raw Vulkan**, targeting Vulkan 1.3 (dynamic rendering, synchronization2), with validation
  layers in Debug.
- **Allowed helpers**, to be added when the work needs them: VulkanMemoryAllocator (VMA) for buffer and
  image memory, and glslang's tools from vcpkg for compiling GLSL to SPIR-V offline at build time (no
  dependency on a locally installed Vulkan SDK). Both arrive with the textures and sprites work.
- Not adopted: vk-bootstrap (it would replace setup code that already works), volk (not needed now).

## Consequences

- The SDL3 migration is small and touches only the platform layer and one call in `rhi/`; it is the next
  roadmap item.
- Graphyne keeps owning its Vulkan code, so each feature needs more code, and the code teaches more.
- Engine areas stay free of SDL and Vulkan; the Vulkan handles that `graphics/frame.h` exposes to `App`
  users should move behind Graphyne types as rendering grows.
- `feat/texture-rendering` is not rebased as is (conflicts in 8 files, SDL2-based, and it adds `stb`
  without an ADR). The sprites item rebuilds it on SDL3, VMA and glslang, reusing its non-Vulkan parts
  (camera, sprite data, image decoding). Adding `stb` for image decoding needs the owner's approval then.
