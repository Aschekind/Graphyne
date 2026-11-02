# Roadmap

The orchestrator picks work from **Now**, top to bottom. One item per branch and PR; the PR that starts or
finishes an item updates this file. Decisions go to `adr/`.

Goal: a lightweight engine for learning and experimenting with low-level engine design (ECS, rendering,
multithreading). Stack: SDL3 + raw Vulkan 1.3 (ADR 0003).

## Now

1. **Migrate SDL2 → SDL3** (ADR 0003). `vcpkg.json` (`sdl3`, Linux features wayland and x11 only, as for
   SDL2), CMake targets, `platform/window.cpp` and `platform.cpp` (events, key codes, init, bool returns),
   the Vulkan surface and instance-extension calls (`rhi/device.cpp`, `platform/window.cpp`), README.
   Done when `simple_window` opens and clears on Windows, tests pass, and CI is green on both platforms.

## Next

2. **Textures and sprites** on SDL3 + Vulkan, with VMA and glslang tools (ADR 0003). Rebuild from
   `origin/feat/texture-rendering` rather than rebasing it (8 conflicting files, SDL2-based); reuse its
   camera, sprite data and image-decoding parts. Image decoding needs `stb` or similar: ask the owner and
   write an ADR. Batch sprites instead of one draw per sprite.
3. **ECS and scenes**: the main experiment. Pure C++, fully unit-tested, independent of rendering.

## Backlog

- Remove the tombstone files (`core/memory.h`, `graphics/vulkan_renderer.h`, their comment-only,
  uncompiled `.cpp`); the headers are still installed.
- Delete or use `cmake/GraphyneUtils.cmake` and `cmake/GraphyneDependencies.cmake` (unused).
- Project homepage in `CMakeLists.txt` and `vcpkg.json` points to `Multyp/graphyne`; origin is
  `Berlin-Team/Graphyne` (owner to confirm which is right).
- `core/app.h` doc comments name `gn::Frame`; the real type is `graphics::Frame`.
- Test presets for `asan`, `lint`, `msvc`; consider clang-tidy and ASan in CI.
- README layout section is out of date.
- Add the squash commit of the style-config PR to `.git-blame-ignore-revs` (the hash exists only after
  the merge).
- Names that break the naming rules: `Ok()` in `core/result.h`, the colour constants in
  `graphics/color.h` (no `k` prefix), the `r_`/`g_`/`b_`/`a_` parameters there. Rename, or record them
  as exceptions in ADR 0002.
- `scripts/setup.sh` still defaults to system packages; make vcpkg its default to match the vcpkg-only
  policy.
- If CI checks become required in branch protection, docs-only PRs (skipped by `paths-ignore`) can't
  merge: add an always-green fallback job then.

## Done

- 2026-10-28: agent setup specific to Graphyne (`AGENTS.md`, agents, skills, this roadmap).
- 2026-10-28: style configs follow the code (ADR 0002); one-off clang-format pass; CI pins
  clang-format 19.1.5. Also fixed the GCC/Clang build (nested `Config` default arguments replaced by
  overloads) and the Windows CI job (`setup-vulkan-sdk` v1.2.1, off the disabled `actions/cache@v2`).
- 2026-10-28: toolchain refresh. vcpkg baseline 2026.10.16 everywhere (Linux CI moved from apt to vcpkg),
  CMake ≥ 3.21, ubuntu-24.04, no Vulkan SDK step, vcpkg binary cache, CI trimmed for the minutes budget.
- 2026-10-29: stack review. SDL3 for windowing, raw Vulkan 1.3 kept, VMA and glslang tools approved
  (ADR 0003).
