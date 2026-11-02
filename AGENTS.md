# AGENTS.md

Instructions for any coding agent (Claude Code, Codex, Copilot...) working in this repository.
Humans: start with `README.md`.

## The project

Graphyne is a lightweight game engine written from scratch in C++20: a personal R&D project for
low-level architecture, performance and experimentation (ECS, rendering, multithreading, modular design).
It is early-stage. Keep it lightweight: prefer small, understandable code over general frameworks.

**Stack (ADR 0003): SDL3 for windowing and input, raw Vulkan 1.3 for graphics.** Graphyne owns its Vulkan
code on purpose (learning value); don't add rendering abstractions or engines. Approved helpers, added
when the work needs them: VMA and glslang's tools. The code is still on SDL2 until the SDL3 migration on
the roadmap lands; don't add new SDL2-specific code meanwhile.

Plans live in `docs/ROADMAP.md`, decisions in `docs/adr/` (one numbered file per decision).

## Stack

- C++20 (no compiler extensions), CMake ≥ 3.21, vcpkg manifest mode (`vcpkg.json`) pinned by
  `builtin-baseline` (vcpkg release 2026.10.16). Local vcpkg clones must contain that commit; CI uses the
  same commit. Changing the baseline is a dependency change: ask the owner.
- Dependencies (all from vcpkg, on every platform): Vulkan (headers + loader), SDL2 (SDL3 after the
  migration), spdlog, fmt (through spdlog), glm, GoogleTest.
- Platforms: Windows (MSVC, Visual Studio 2022) and Linux (GCC) are first-class; macOS best effort.
- CI: `.github/workflows/ci.yml`. clang-format first; then Linux (GCC, ubuntu-24.04, tests + coverage) and
  Windows (MSVC, tests; Debug on PRs, Debug + Release on push to `develop`/`main`). Docs-only and
  `.claude/`-only changes skip CI.
- **CI minutes are budgeted** (the repo is going private: 3000 Actions minutes a month, Windows billed 2x,
  macOS 10x). Every CI change keeps the vcpkg binary cache effective, cancels superseded runs, runs cheap
  checks first, sets `timeout-minutes`, and adds no runner, matrix entry or macOS job without the owner's
  approval. Don't push to a PR branch just to "see what CI says" when a local build can answer.

## Layout

| Path | Contents |
| --- | --- |
| `project/include/<area>/`, `project/src/<area>/` | The `graphyne` static library (`graphyne::graphyne`). Areas: `core` (app, engine, clock, handle, slot_map, result, types, version), `memory` (arena, pool), `utils` (logger, assert), `platform` (SDL window, input, events), `rhi` (Vulkan device, swapchain, command, sync), `graphics` (renderer, frame, color). |
| `tests/<area>/test_*.cpp` | GoogleTest unit tests, one executable `graphyne_tests`, listed in `tests/CMakeLists.txt`. |
| `examples/` | `simple_window` example. |
| `cmake/` | Coverage helpers, package config template. |
| `scripts/` | `setup.sh` / `setup.ps1`: install prerequisites. |
| `docs/` | `ROADMAP.md`, `adr/`. |
| `.claude/` | Agents, hooks, skills, the orchestrator prompt. |

Sources are listed explicitly in `CMakeLists.txt` (no globbing): a new `.cpp` must be added there, a new
test file in `tests/CMakeLists.txt`. Use the `add-source-file` skill.

## Commands

Windows (this machine: VS 2022 Community, `VCPKG_ROOT=C:\Users\migue\vcpkg`):

```bash
cmake --preset msvc                                         # configure; first run builds vcpkg ports (~3 min)
cmake --build --preset msvc -- /nodeReuse:false             # build; the flag stops MSBuild leaving workers behind
ctest --test-dir build/msvc -C Debug --output-on-failure    # all tests
build/msvc/bin/Debug/graphyne_tests.exe --gtest_filter=Arena.*   # one suite
```

The Ninja presets (`default`, `release`, `coverage`, `asan`, `lint`) need `cl` on `PATH`, that is a Visual Studio
Developer shell, on Windows. `coverage` needs GCC + lcov (Linux).

Linux:

```bash
cmake --preset default && cmake --build --preset default && ctest --preset default
COVERAGE=ON ./build.sh                                      # coverage report in build/
```

Format check (same as CI). On this machine clang-format and clang-tidy are not on `PATH`; they ship with
Visual Studio at `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin\`.

```bash
find project/include project/src tests examples -type f \( -iname '*.cpp' -o -iname '*.h' -o -iname '*.hpp' \) -print0 | xargs -0 clang-format --dry-run --Werror
```

Lint: `cmake --preset lint` (clang-tidy through `GRAPHYNE_USE_CLANG_TIDY`); not run in CI yet.

## Conventions

The code is the reference for style; the configs are being aligned to it (ADR 0002, roadmap).

- Formatting: 4 spaces, 120 columns, attached (K&R) braces, LF line endings, UTF-8, final newline.
- Names: types `CamelCase`; functions, methods, variables, parameters `snake_case`; private members
  `m_name`; global and class constants `kName`; enum constants `CamelCase` (no prefix); macros `GN_NAME`; namespaces `gn`, `gn::<area>` (lower case).
- Headers `.h` with `#pragma once`; project includes use quotes, rooted at `project/include`
  (`#include "core/app.h"`); third-party and standard headers use angle brackets.
- Errors: `gn::Result` (`core/result.h`) for recoverable failures, `GN_ASSERT` for programmer errors;
  log through `utils/logger.h` macros, not `printf`/`std::cout`.
- No new dependency (vcpkg port, submodule, vendored code) without the owner's approval and an ADR.

## Tests

- Every change to `core`, `memory` or `utils` (and any future engine area such as ECS) comes with unit
  tests in `tests/<area>/`. Write code so its logic can be tested without a window or a GPU.
- `platform`, `rhi` and `graphics` have no automated tests yet (they need a display and a Vulkan device):
  build them, and when a brief asks, run `simple_window` with a hard time limit and report what happened.
  Pure logic added there (math, layouts, batching) still gets unit tests.
- Never weaken, skip or delete a test to make a change pass. A change is done when it builds without new
  warnings, all tests pass, and the format check passes on the files it touched.

## Git and GitHub

- `develop` is the integration branch; `main` is for releases and only the owner updates it.
- Work on a branch off `develop`: `feat/…`, `fix/…`, `chore/…`, `docs/…`, `build/…`, `ci/…`. One roadmap
  item or fix per branch.
- Commits and PR titles: plain Conventional Commits (<https://www.conventionalcommits.org>), no emoji or
  gitmoji shortcodes: `type(optional scope): imperative subject`, e.g. `feat(graphics): add sprite batcher`,
  `fix: handle swapchain resize`. Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`,
  `build`, `ci`, `chore`. Breaking changes: `!` after the type and a `BREAKING CHANGE:` footer.
- Agents may commit on their branch, push that branch and open a PR into `develop`. The owner reviews and
  squash-merges. Never push to `develop` or `main`, never merge, never force-push a shared branch.
- **No trace of an assistant in git or on GitHub**: no `Co-Authored-By` trailer, no "Generated with" line,
  no assistant named as an author (owner's rule; it overrides any tool default).
- Never run git commands that discard work (`git reset --hard`, `git clean -f`, `git checkout .`,
  `git restore .`, `git rm -r .`); a hook blocks them.

## Documentation

- A decision that is hard to reverse (dependency, architecture, workflow, style) gets an ADR in
  `docs/adr/` (`write-adr` skill), approved by the owner.
- Update `docs/ROADMAP.md` in the same PR that starts or finishes a roadmap item.
- Update `README.md` when commands, requirements or the status table change.
- Comments explain why, not what. Public headers get a short doc comment per class.

## Machine rules

- Never run `python` (a hook blocks it). Hooks need Node.
- Give every long run a hard time limit and stop every process you start; never kill processes by name.
- Keep logs and scratch files out of the repository.
- `core.autocrlf=true` here: the working tree is CRLF, the repository stores LF. Don't commit CRLF.
