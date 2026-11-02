# Roadmap

The orchestrator picks work from **Now**, top to bottom. One item per branch and PR; the PR that starts or
finishes an item updates this file. Decisions go to `adr/`.

Goal: a lightweight engine for learning and experimenting with low-level engine design (ECS, rendering,
multithreading). Stack: SDL3 + raw Vulkan 1.3 (ADR 0003).

## Now

1. **ECS and scenes**: the main experiment. Pure C++, fully unit-tested, independent of rendering.
   Design first (archetypes vs sparse sets), with the owner, before code.

## Next

- Nothing scheduled; pick from the backlog with the owner.

## Backlog

- `graphics::Frame` still exposes Vulkan handles to `App::on_render`; hide them behind Graphyne types.
- Shaders: `graphyne_add_shaders` writes to `${CMAKE_BINARY_DIR}/bin`, which is the parent's tree when
  Graphyne is used through `add_subdirectory`; make the output dir follow the consuming executables, and
  make the runtime shader directory configurable.
- `SpriteBatch::finish` allocates scratch vectors on every layer sort; reuse members.
- `TextureManager::load` caches by the raw path string (`a.png` and `./a.png` decode twice).
- `SpriteRenderer` bakes the swapchain format at init; rebuild the pipeline if a recreate changes it.

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
- 2026-10-29: SDL2 → SDL3 (ADR 0003). Swapchain size and resize events now in pixels (HiDPI).
- 2026-11-02: one render-finished semaphore per swapchain image; `simple_window` runs without validation
  errors.
- 2026-11-02: textures and sprites (ADR 0003, 0004). VMA, glslang tools and shader compile step; RHI
  buffers, images, samplers, shaders, pipeline builder, descriptors; `Camera2D`, stb_image loader,
  `TextureManager`, batched `SpriteBatch` + `SpriteRenderer`; `simple_window` draws 400 sprites in 3 draws.
