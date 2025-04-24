#include "graphyne/controls/input_system.hpp"
#include <SDL2/SDL.h>
#include <algorithm>

namespace graphyne::input
{

InputAction::InputAction(const std::string& name) : m_name(name) {}

InputAction& InputAction::bindKey(SDL_Keycode keycode)
{
    Binding binding;
    binding.type = Binding::Type::Keyboard;
    binding.keycode = keycode;
    m_bindings.push_back(binding);
    return *this;
}

InputAction& InputAction::bindMouseButton(MouseButton button)
{
    Binding binding;
    binding.type = Binding::Type::MouseButton;
    binding.mouseButton = button;
    m_bindings.push_back(binding);
    return *this;
}

InputAction& InputAction::bindGamepadButton(int gamepadIndex, int buttonIndex)
{
    Binding binding;
    binding.type = Binding::Type::GamepadButton;
    binding.gamepadButton.gamepadIndex = gamepadIndex;
    binding.gamepadButton.buttonIndex = buttonIndex;
    m_bindings.push_back(binding);
    return *this;
}

InputAction& InputAction::bindGamepadAxis(int gamepadIndex, int axisIndex, float threshold, bool aboveThreshold)
{
    Binding binding;
    binding.type = Binding::Type::GamepadAxis;
    binding.gamepadAxis.gamepadIndex = gamepadIndex;
    binding.gamepadAxis.axisIndex = axisIndex;
    binding.gamepadAxis.threshold = threshold;
    binding.gamepadAxis.aboveThreshold = aboveThreshold;
    m_bindings.push_back(binding);
    return *this;
}

bool InputAction::isActive(KeyState state) const
{
    for (const auto& binding : m_bindings)
    {
        switch (binding.type)
        {
            case Binding::Type::Keyboard: {
                const auto& inputSystem = InputSystem::getInstance();
                if (inputSystem.getKeyState(binding.keycode) == state)
                    return true;
                break;
            }
            case Binding::Type::MouseButton: {
                const auto& inputSystem = InputSystem::getInstance();
                const auto& mouseState = inputSystem.getMouseState();
                auto it = mouseState.buttons.find(binding.mouseButton);
                if (it != mouseState.buttons.end() && it->second == state)
                    return true;
                break;
            }
            case Binding::Type::GamepadButton: {
                const auto& inputSystem = InputSystem::getInstance();
                const auto& gamepadState = inputSystem.getGamepadState(binding.gamepadButton.gamepadIndex);
                if (gamepadState.connected)
                {
                    auto it = gamepadState.buttons.find(binding.gamepadButton.buttonIndex);
                    if (it != gamepadState.buttons.end() && it->second == state)
                        return true;
                }
                break;
            }
            case Binding::Type::GamepadAxis: {
                const auto& inputSystem = InputSystem::getInstance();
                const auto& gamepadState = inputSystem.getGamepadState(binding.gamepadAxis.gamepadIndex);
                if (gamepadState.connected)
                {
                    auto it = gamepadState.axes.find(binding.gamepadAxis.axisIndex);
                    if (it != gamepadState.axes.end())
                    {
                        bool above = it->second > binding.gamepadAxis.threshold;
                        if (above == binding.gamepadAxis.aboveThreshold)
                            return true;
                    }
                }
                break;
            }
        }
    }
    return false;
}

InputSystem& InputSystem::getInstance()
{
    static InputSystem instance;
    return instance;
}

bool InputSystem::initialize()
{
    if (m_initialized)
        return true;

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        return false;

    // Initialize mouse state
    m_mouseState.buttons[MouseButton::Left] = KeyState::Released;
    m_mouseState.buttons[MouseButton::Middle] = KeyState::Released;
    m_mouseState.buttons[MouseButton::Right] = KeyState::Released;
    m_mouseState.buttons[MouseButton::X1] = KeyState::Released;
    m_mouseState.buttons[MouseButton::X2] = KeyState::Released;

    m_initialized = true;
    return true;
}

void InputSystem::shutdown()
{
    if (!m_initialized)
        return;

    for (auto& [index, state] : m_gamepadStates)
    {
        if (state.connected)
        {
            SDL_GameControllerClose(SDL_GameControllerFromInstanceID(index));
        }
    }

    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    m_initialized = false;
}

void InputSystem::processEvent(const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_KEYDOWN: {
            auto& state = m_keyStates[event.key.keysym.sym];
            if (state != KeyState::Held)
                state = KeyState::JustPressed;
            break;
        }
        case SDL_KEYUP: {
            m_keyStates[event.key.keysym.sym] = KeyState::JustReleased;
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            MouseButton button;
            switch (event.button.button)
            {
                case SDL_BUTTON_LEFT:
                    button = MouseButton::Left;
                    break;
                case SDL_BUTTON_MIDDLE:
                    button = MouseButton::Middle;
                    break;
                case SDL_BUTTON_RIGHT:
                    button = MouseButton::Right;
                    break;
                case SDL_BUTTON_X1:
                    button = MouseButton::X1;
                    break;
                case SDL_BUTTON_X2:
                    button = MouseButton::X2;
                    break;
                default:
                    return;
            }
            m_mouseState.buttons[button] = KeyState::JustPressed;
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            MouseButton button;
            switch (event.button.button)
            {
                case SDL_BUTTON_LEFT:
                    button = MouseButton::Left;
                    break;
                case SDL_BUTTON_MIDDLE:
                    button = MouseButton::Middle;
                    break;
                case SDL_BUTTON_RIGHT:
                    button = MouseButton::Right;
                    break;
                case SDL_BUTTON_X1:
                    button = MouseButton::X1;
                    break;
                case SDL_BUTTON_X2:
                    button = MouseButton::X2;
                    break;
                default:
                    return;
            }
            m_mouseState.buttons[button] = KeyState::JustReleased;
            break;
        }
        case SDL_MOUSEMOTION: {
            m_mouseState.x = event.motion.x;
            m_mouseState.y = event.motion.y;
            m_mouseState.deltaX = event.motion.xrel;
            m_mouseState.deltaY = event.motion.yrel;
            break;
        }
        case SDL_MOUSEWHEEL: {
            m_mouseState.scrollX = event.wheel.x;
            m_mouseState.scrollY = event.wheel.y;
            break;
        }
        case SDL_CONTROLLERDEVICEADDED: {
            int index = event.cdevice.which;
            SDL_GameController* controller = SDL_GameControllerOpen(index);
            if (controller)
            {
                int instanceID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
                auto& state = m_gamepadStates[instanceID];
                state.connected = true;
                state.name = SDL_GameControllerName(controller);
            }
            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED: {
            int instanceID = event.cdevice.which;
            auto it = m_gamepadStates.find(instanceID);
            if (it != m_gamepadStates.end())
            {
                it->second.connected = false;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN: {
            int instanceID = event.cbutton.which;
            auto& state = m_gamepadStates[instanceID];
            if (state.connected)
            {
                auto& buttonState = state.buttons[event.cbutton.button];
                if (buttonState != KeyState::Held)
                    buttonState = KeyState::JustPressed;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP: {
            int instanceID = event.cbutton.which;
            auto& state = m_gamepadStates[instanceID];
            if (state.connected)
            {
                state.buttons[event.cbutton.button] = KeyState::JustReleased;
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION: {
            int instanceID = event.caxis.which;
            auto& state = m_gamepadStates[instanceID];
            if (state.connected)
            {
                state.axes[event.caxis.axis] = event.caxis.value / 32767.0f;
            }
            break;
        }
    }
}

void InputSystem::update()
{
    // Update keyboard states
    for (auto& [key, state] : m_keyStates)
    {
        if (state == KeyState::JustPressed)
            state = KeyState::Held;
        else if (state == KeyState::JustReleased)
            state = KeyState::Released;
    }

    // Update mouse button states
    for (auto& [button, state] : m_mouseState.buttons)
    {
        if (state == KeyState::JustPressed)
            state = KeyState::Held;
        else if (state == KeyState::JustReleased)
            state = KeyState::Released;
    }

    // Update gamepad button states
    for (auto& [index, state] : m_gamepadStates)
    {
        if (state.connected)
        {
            for (auto& [button, buttonState] : state.buttons)
            {
                if (buttonState == KeyState::JustPressed)
                    buttonState = KeyState::Held;
                else if (buttonState == KeyState::JustReleased)
                    buttonState = KeyState::Released;
            }
        }
    }

    // Reset mouse delta and scroll
    m_mouseState.deltaX = 0;
    m_mouseState.deltaY = 0;
    m_mouseState.scrollX = 0;
    m_mouseState.scrollY = 0;

    // Process action callbacks
    for (const auto& callback : m_actionCallbacks)
    {
        auto* action = getAction(callback.actionName);
        if (action && action->isActive(callback.triggerState))
        {
            callback.callback();
        }
    }
}

KeyState InputSystem::getKeyState(SDL_Keycode keycode) const
{
    auto it = m_keyStates.find(keycode);
    return it != m_keyStates.end() ? it->second : KeyState::Released;
}

const MouseState& InputSystem::getMouseState() const
{
    return m_mouseState;
}

const GamepadState& InputSystem::getGamepadState(int index) const
{
    static GamepadState defaultState;
    auto it = m_gamepadStates.find(index);
    return it != m_gamepadStates.end() ? it->second : defaultState;
}

InputAction& InputSystem::createAction(const std::string& name)
{
    return m_actions[name] = InputAction(name);
}

InputAction* InputSystem::getAction(const std::string& name)
{
    auto it = m_actions.find(name);
    return it != m_actions.end() ? &it->second : nullptr;
}

bool InputSystem::addActionCallback(const std::string& actionName,
                                    std::function<void()> callback,
                                    KeyState triggerState)
{
    if (!getAction(actionName))
        return false;

    m_actionCallbacks.push_back({actionName, std::move(callback), triggerState});
    return true;
}

void InputSystem::clearBindings()
{
    m_actions.clear();
    m_actionCallbacks.clear();
    m_keyStates.clear();
    m_mouseState.buttons.clear();
    m_gamepadStates.clear();
}

} // namespace graphyne::input
