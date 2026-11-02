# 0004. stb_image for image decoding

- Status: Accepted
- Date: 2026-11-02

## Context

Textures and sprites (roadmap) need image files decoded to RGBA8 pixels before upload to Vulkan. Graphyne
keeps dependencies few and light: they come from vcpkg and every cold CI build pays for them.
`origin/feat/texture-rendering` already decoded images with `stb_image`, through a small
`resources/image_loader` wrapper.

`stb` is a vcpkg port (version 2026-08-02 at our baseline), header-only, public domain or MIT
(<https://github.com/nothings/stb>), with nothing to build. Heavier alternatives (libpng + libjpeg-turbo,
DevIL, FreeImage) bring native builds and more formats than Graphyne needs.

## Decision

Use `stb_image` from the vcpkg `stb` port for decoding PNG and JPEG (and the other formats it reads) to
RGBA8. Only `resources/image_loader.cpp` includes it and defines `STB_IMAGE_IMPLEMENTATION`; the rest of
Graphyne sees Graphyne types. Other `stb` headers are not used unless a later decision says so.

## Consequences

- No build cost in CI; one header to keep up to date through the vcpkg baseline.
- `stb_image` is not hardened against hostile files; Graphyne loads its own assets only. Revisit if it
  ever loads untrusted images.
- No GPU-compressed formats (KTX2, BC/ASTC); a later decision can add them when needed.
