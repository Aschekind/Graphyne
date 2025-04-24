/**
 * @file input_system.hpp
 * @brief Input handling system for Graphyne engine
 */
#pragma once

#include <SDL2/SDL.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace graphyne::input
{

/**
 * @enum KeyState
 * @brief Represents the state of a key or button
 */
enum class KeyState
{
    Released,
    Pressed,
    Held,
    JustPressed,
    JustReleased
};

/**
 * @enum MouseButton
 * @brief Represents mouse buttons
 */
enum class MouseButton
{
    Left,
    Middle,
    Right,
    X1,
    X2
};

/**
 * @struct MouseState
 * @brief Contains mouse position and button state information
 */
struct MouseState
{
    int x = 0;
    int y = 0;
    int deltaX = 0;
    int deltaY = 0;
    int scrollX = 0;
    int scrollY = 0;
    std::unordered_map<MouseButton, KeyState> buttons;
};

/**
 * @struct GamepadState
 * @brief Contains gamepad button and axis state information
 */
struct GamepadState
{
    bool connected = false;
    std::string name;
    std::unordered_map<int, KeyState> buttons;
    std::unordered_map<int, float> axes;
};

/**
 * @class InputAction
 * @brief Maps input bindings to named actions
 */
class InputAction
{
public:
    /**
     * @brief Default constructor
     */
    InputAction() = default;

    /**
     * @brief Constructor
     * @param name Name of the action
     */
    explicit InputAction(const std::string& name);

    /**
     * @brief Add a keyboard binding to this action
     * @param keycode SDL keycode
     * @return Reference to this action for chaining
     */
    InputAction& bindKey(SDL_Keycode keycode);

    /**
     * @brief Add a mouse button binding to this action
     * @param button Mouse button
     * @return Reference to this action for chaining
     */
    InputAction& bindMouseButton(MouseButton button);

    /**
     * @brief Add a gamepad button binding to this action
     * @param gamepadIndex Gamepad index
     * @param buttonIndex Button index
     * @return Reference to this action for chaining
     */
    InputAction& bindGamepadButton(int gamepadIndex, int buttonIndex);

    /**
     * @brief Add a gamepad axis binding to this action
     * @param gamepadIndex Gamepad index
     * @param axisIndex Axis index
     * @param threshold Threshold value for activation
     * @param aboveThreshold Whether action triggers above or below threshold
     * @return Reference to this action for chaining
     */
    InputAction& bindGamepadAxis(int gamepadIndex, int axisIndex, float threshold, bool aboveThreshold);

    /**
     * @brief Check if action is active
     * @param state Optional state to check against (pressed, held, etc.)
     * @return True if action is active with specified state
     */
    bool isActive(KeyState state = KeyState::Pressed) const;

    /**
     * @brief Get the action name
     * @return Action name
     */
    const std::string& getName() const
    {
        return m_name;
    }

private:
    struct Binding
    {
        enum class Type
        {
            Keyboard,
            MouseButton,
            GamepadButton,
            GamepadAxis
        };

        Type type;
        union
        {
            SDL_Keycode keycode;
            MouseButton mouseButton;
            struct
            {
                int gamepadIndex;
                int buttonIndex;
            } gamepadButton;
            struct
            {
                int gamepadIndex;
                int axisIndex;
                float threshold;
                bool aboveThreshold;
            } gamepadAxis;
        };
    };

    std::string m_name;
    std::vector<Binding> m_bindings;
    friend class InputSystem;
};

/**
 * @class InputSystem
 * @brief Manages input handling and action mapping
 */
class InputSystem
{
public:
    /**
     * @brief Get singleton instance of the input system
     * @return Reference to the input system instance
     */
    static InputSystem& getInstance();

    /**
     * @brief Initialize the input system
     * @return True if initialization succeeded
     */
    bool initialize();

    /**
     * @brief Shutdown the input system
     */
    void shutdown();

    /**
     * @brief Process input events
     * @param event SDL event to process
     */
    void processEvent(const SDL_Event& event);

    /**
     * @brief Update input states
     * Called once per frame to update held/pressed states
     */
    void update();

    /**
     * @brief Get the state of a keyboard key
     * @param keycode SDL keycode
     * @return Key state
     */
    KeyState getKeyState(SDL_Keycode keycode) const;

    /**
     * @brief Get mouse state
     * @return Current mouse state
     */
    const MouseState& getMouseState() const;

    /**
     * @brief Get gamepad state
     * @param index Gamepad index
     * @return Current gamepad state
     */
    const GamepadState& getGamepadState(int index) const;

    /**
     * @brief Create a new input action
     * @param name Action name
     * @return Reference to the created action
     */
    InputAction& createAction(const std::string& name);

    /**
     * @brief Get an input action by name
     * @param name Action name
     * @return Pointer to the action, or nullptr if not found
     */
    InputAction* getAction(const std::string& name);

    /**
     * @brief Add a callback for an input action
     * @param actionName Name of the action
     * @param callback Function to call when action is triggered
     * @param triggerState State that triggers the callback
     * @return True if action exists and callback was added
     */
    bool addActionCallback(const std::string& actionName,
                           std::function<void()> callback,
                           KeyState triggerState = KeyState::JustPressed);

    /**
     * @brief Clear all input bindings and actions
     */
    void clearBindings();

private:
    // Private constructor for singleton
    InputSystem() = default;

    // Deleted copy and move constructors and assignment operators
    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;
    InputSystem(InputSystem&&) = delete;
    InputSystem& operator=(InputSystem&&) = delete;

    struct ActionCallback
    {
        std::string actionName;
        std::function<void()> callback;
        KeyState triggerState;
    };

    std::unordered_map<SDL_Keycode, KeyState> m_keyStates;
    MouseState m_mouseState;
    std::unordered_map<int, GamepadState> m_gamepadStates;
    std::unordered_map<std::string, InputAction> m_actions;
    std::vector<ActionCallback> m_actionCallbacks;
    bool m_initialized = false;
};

} // namespace graphyne::input
