#!/usr/bin/env bash
#
# One-shot setup for Linux / macOS.
#
# Installs everything Graphyne needs to build, test, and produce a coverage
# report. Tries hard to be idempotent — re-running it skips anything that
# is already installed.
#
# Modes:
#   --system   (default on Linux) install via apt / dnf / pacman / brew.
#   --vcpkg                       clone vcpkg into ~/vcpkg and use manifest mode.
#   --both                        do both (system tools, vcpkg for portability).
#
# Usage:
#   scripts/setup.sh             # auto-pick (apt on Linux, brew on macOS, vcpkg as fallback)
#   scripts/setup.sh --vcpkg
#   VCPKG_ROOT=/custom/path scripts/setup.sh --vcpkg

set -euo pipefail

# ---------------------------------------------------------------------------
# CLI parsing
# ---------------------------------------------------------------------------
MODE="auto"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --system) MODE="system"; shift ;;
        --vcpkg)  MODE="vcpkg";  shift ;;
        --both)   MODE="both";   shift ;;
        -h|--help)
            grep '^#' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *) echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
done

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
say()    { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
warn()   { printf '\033[1;33m    %s\033[0m\n' "$*"; }
ok()     { printf '\033[1;32m    %s\033[0m\n' "$*"; }
have()   { command -v "$1" >/dev/null 2>&1; }

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"

# ---------------------------------------------------------------------------
# Detect platform
# ---------------------------------------------------------------------------
OS="$(uname -s)"
case "$OS" in
    Linux*)
        PLATFORM="linux"
        if [[ -f /etc/os-release ]]; then
            . /etc/os-release
            DISTRO="${ID:-unknown}"
        else
            DISTRO="unknown"
        fi
        ;;
    Darwin*) PLATFORM="macos"; DISTRO="macos" ;;
    *) echo "Unsupported OS: $OS" >&2; exit 1 ;;
esac

if [[ "$MODE" == "auto" ]]; then
    case "$DISTRO" in
        ubuntu|debian|fedora|rhel|centos|arch|manjaro|macos) MODE="system" ;;
        *) MODE="vcpkg" ;;
    esac
fi

say "Platform: $PLATFORM ($DISTRO)   |   Mode: $MODE"

# ---------------------------------------------------------------------------
# System packages
# ---------------------------------------------------------------------------
install_system_packages() {
    case "$DISTRO" in
        ubuntu|debian)
            say "Installing apt packages"
            sudo apt-get update
            sudo apt-get install -y --no-install-recommends \
                build-essential cmake ninja-build git pkg-config \
                libvulkan-dev vulkan-validationlayers spirv-tools glslang-tools \
                libsdl2-dev \
                libfmt-dev libspdlog-dev libglm-dev \
                libgtest-dev googletest \
                lcov gcovr clang-format clang-tidy
            ;;
        fedora|rhel|centos)
            say "Installing dnf packages"
            sudo dnf install -y \
                gcc gcc-c++ make cmake ninja-build git pkgconf \
                vulkan-loader-devel vulkan-validation-layers spirv-tools glslang \
                SDL2-devel \
                fmt-devel spdlog-devel glm-devel \
                gtest-devel \
                lcov gcovr clang-tools-extra
            ;;
        arch|manjaro)
            say "Installing pacman packages"
            sudo pacman -S --needed --noconfirm \
                base-devel cmake ninja git pkgconf \
                vulkan-headers vulkan-validation-layers spirv-tools glslang \
                sdl2 \
                fmt spdlog glm \
                gtest \
                lcov gcovr clang
            ;;
        macos)
            if ! have brew; then
                warn "Homebrew not found. Install it first:"
                warn '  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
                exit 1
            fi
            say "Installing brew packages"
            brew update
            brew install cmake ninja git pkg-config \
                         sdl2 fmt spdlog glm googletest lcov llvm
            warn "macOS: install the Vulkan SDK manually from"
            warn "  https://vulkan.lunarg.com/sdk/home#mac"
            warn "and run 'source ~/VulkanSDK/<version>/setup-env.sh' before building."
            ;;
        *)
            warn "Unknown Linux distro '$DISTRO'. Skipping system packages — use --vcpkg instead."
            return 1
            ;;
    esac
    ok "System packages installed."
}

# ---------------------------------------------------------------------------
# vcpkg
# ---------------------------------------------------------------------------
install_vcpkg() {
    say "Setting up vcpkg in $VCPKG_ROOT"
    if [[ ! -d "$VCPKG_ROOT/.git" ]]; then
        if [[ -e "$VCPKG_ROOT" ]]; then
            echo "Path $VCPKG_ROOT exists but isn't a vcpkg clone. Remove it or set VCPKG_ROOT." >&2
            exit 1
        fi
        git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
    else
        ok "vcpkg already cloned"
    fi

    if [[ ! -x "$VCPKG_ROOT/vcpkg" ]]; then
        "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics
    else
        ok "vcpkg.exe already built"
    fi

    # Persist VCPKG_ROOT in the user's shell profile (idempotent).
    local profile
    case "$SHELL" in
        */zsh)  profile="$HOME/.zshrc" ;;
        */bash) profile="$HOME/.bashrc" ;;
        *)      profile="$HOME/.profile" ;;
    esac
    if [[ -f "$profile" ]] && grep -q 'VCPKG_ROOT=' "$profile"; then
        ok "VCPKG_ROOT already set in $profile"
    else
        {
            echo ""
            echo "# Added by graphyne/scripts/setup.sh"
            echo "export VCPKG_ROOT=\"$VCPKG_ROOT\""
            echo "export PATH=\"\$VCPKG_ROOT:\$PATH\""
        } >> "$profile"
        ok "Appended VCPKG_ROOT to $profile (open a new shell to pick it up)"
    fi

    say "Installing vcpkg manifest dependencies"
    pushd "$REPO_ROOT" >/dev/null
        if [[ ! -f vcpkg.json ]]; then
            echo "vcpkg.json not found in $REPO_ROOT" >&2
            exit 1
        fi
        local triplet
        case "$PLATFORM" in
            linux) triplet="x64-linux" ;;
            macos) triplet="$(uname -m | sed 's/x86_64/x64/; s/arm64/arm64/')-osx" ;;
        esac
        "$VCPKG_ROOT/vcpkg" install --triplet "$triplet"
    popd >/dev/null
    ok "Dependencies installed via vcpkg"
}

# ---------------------------------------------------------------------------
# Drive
# ---------------------------------------------------------------------------
case "$MODE" in
    system) install_system_packages ;;
    vcpkg)  install_vcpkg ;;
    both)
        install_system_packages || true
        install_vcpkg
        ;;
esac

say "Setup complete"
cat <<EOF

    You can now build the engine:

        ./build.sh                          # debug build + tests
        COVERAGE=ON ./build.sh              # debug build + tests + lcov HTML report
        BUILD_TYPE=Release ./build.sh       # optimized build

    Or use a CMake preset directly:

        cmake --preset default
        cmake --build --preset default
        ctest --preset default

EOF
