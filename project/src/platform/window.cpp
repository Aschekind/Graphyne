#include "platform/window.h"

#include "utils/logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdio>
#include <utility>

namespace gn::platform {

namespace {

Key sdl_scancode_to_key(SDL_Scancode sc) {
    using K = Key;
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z) {
        return static_cast<K>(static_cast<u16>(K::A) + (sc - SDL_SCANCODE_A));
    }
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9) {
        return static_cast<K>(static_cast<u16>(K::Num1) + (sc - SDL_SCANCODE_1));
    }
    if (sc == SDL_SCANCODE_0) return K::Num0;
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12) {
        return static_cast<K>(static_cast<u16>(K::F1) + (sc - SDL_SCANCODE_F1));
    }
    switch (sc) {
        case SDL_SCANCODE_SPACE: return K::Space;
        case SDL_SCANCODE_TAB: return K::Tab;
        case SDL_SCANCODE_RETURN: return K::Enter;
        case SDL_SCANCODE_ESCAPE: return K::Escape;
        case SDL_SCANCODE_BACKSPACE: return K::Backspace;
        case SDL_SCANCODE_DELETE: return K::Delete;
        case SDL_SCANCODE_LEFT: return K::Left;
        case SDL_SCANCODE_RIGHT: return K::Right;
        case SDL_SCANCODE_UP: return K::Up;
        case SDL_SCANCODE_DOWN: return K::Down;
        case SDL_SCANCODE_LSHIFT: return K::LeftShift;
        case SDL_SCANCODE_RSHIFT: return K::RightShift;
        case SDL_SCANCODE_LCTRL: return K::LeftCtrl;
        case SDL_SCANCODE_RCTRL: return K::RightCtrl;
        case SDL_SCANCODE_LALT: return K::LeftAlt;
        case SDL_SCANCODE_RALT: return K::RightAlt;
        default: return K::Unknown;
    }
}

MouseButton sdl_button_to_mouse(u8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT: return MouseButton::Left;
        case SDL_BUTTON_RIGHT: return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_X1: return MouseButton::X1;
        case SDL_BUTTON_X2: return MouseButton::X2;
        default: return MouseButton::Left;
    }
}

} // namespace

Window::~Window() {
    shutdown();
}

Result<void> Window::initialize(const Config& cfg) {
    if (m_window) return Ok();

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;
    if (cfg.resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (cfg.high_dpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

    m_window = SDL_CreateWindow(cfg.title.c_str(), static_cast<int>(cfg.width), static_cast<int>(cfg.height), flags);
    if (!m_window) {
        return Err{Error{std::string("SDL_CreateWindow failed: ") + SDL_GetError()}};
    }

    int w = 0, h = 0;
    if (!SDL_GetWindowSizeInPixels(m_window, &w, &h)) {
        std::string msg = std::string("SDL_GetWindowSizeInPixels failed: ") + SDL_GetError();
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return Err{Error{std::move(msg)}};
    }
    // SDL3 leaves text input off until started; keep TextInput events flowing.
    if (!SDL_StartTextInput(m_window)) {
        GN_WARNING("SDL_StartTextInput failed: {}", SDL_GetError());
    }
    m_extent.width  = static_cast<u32>(w);
    m_extent.height = static_cast<u32>(h);
    m_should_close  = false;
    m_minimized     = false;
    GN_INFO("Window '{}' created at {}x{}", cfg.title, m_extent.width, m_extent.height);
    return Ok();
}

void Window::shutdown() {
    if (m_window) {
        SDL_StopTextInput(m_window);
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

void Window::pump_events(const EventHandler& handler) {
    SDL_Event sdl;
    Event     ev{};
    while (SDL_PollEvent(&sdl)) {
        switch (sdl.type) {
            case SDL_EVENT_QUIT:
                m_should_close = true;
                ev.type        = EventType::WindowClose;
                if (handler) handler(ev);
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                m_should_close = true;
                ev.type        = EventType::WindowClose;
                if (handler) handler(ev);
                break;

            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                m_extent.width   = static_cast<u32>(sdl.window.data1);
                m_extent.height  = static_cast<u32>(sdl.window.data2);
                m_minimized      = (m_extent.width == 0 || m_extent.height == 0);
                ev.type          = EventType::WindowResize;
                ev.resize.width  = m_extent.width;
                ev.resize.height = m_extent.height;
                if (handler) handler(ev);
                break;

            case SDL_EVENT_WINDOW_MINIMIZED: m_minimized = true; break;
            case SDL_EVENT_WINDOW_RESTORED: m_minimized = false; break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                ev.type = EventType::WindowFocusGained;
                if (handler) handler(ev);
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                ev.type = EventType::WindowFocusLost;
                if (handler) handler(ev);
                break;

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                ev.type       = (sdl.type == SDL_EVENT_KEY_DOWN) ? EventType::KeyDown : EventType::KeyUp;
                ev.key.key    = sdl_scancode_to_key(sdl.key.scancode);
                ev.key.mods   = static_cast<u16>(sdl.key.mod);
                ev.key.repeat = sdl.key.repeat != 0;
                if (handler) handler(ev);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                ev.type = (sdl.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? EventType::MouseDown : EventType::MouseUp;
                ev.mouse_button.button = sdl_button_to_mouse(sdl.button.button);
                ev.mouse_button.x      = static_cast<i32>(sdl.button.x);
                ev.mouse_button.y      = static_cast<i32>(sdl.button.y);
                if (handler) handler(ev);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                ev.type          = EventType::MouseMove;
                ev.mouse_move.x  = static_cast<i32>(sdl.motion.x);
                ev.mouse_move.y  = static_cast<i32>(sdl.motion.y);
                ev.mouse_move.dx = static_cast<i32>(sdl.motion.xrel);
                ev.mouse_move.dy = static_cast<i32>(sdl.motion.yrel);
                if (handler) handler(ev);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                ev.type      = EventType::MouseScroll;
                ev.scroll.dx = sdl.wheel.x;
                ev.scroll.dy = sdl.wheel.y;
                if (handler) handler(ev);
                break;

            case SDL_EVENT_TEXT_INPUT:
                ev.type = EventType::TextInput;
                std::snprintf(ev.text, sizeof(ev.text), "%s", sdl.text.text);
                if (handler) handler(ev);
                break;

            default: break;
        }
        ev = {};
    }
}

std::vector<const char*> Window::required_instance_extensions() const {
    Uint32             count = 0;
    const char* const* exts  = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!exts) return {};
    return {exts, exts + count};
}

} // namespace gn::platform
