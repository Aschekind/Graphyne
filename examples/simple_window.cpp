/**
 * @file simple_window.cpp
 * @brief Simple example showing window creation with Graphyne engine
 */
#include "graphyne/controls/input_system.hpp"
#include "graphyne/core/engine.hpp"
#include "graphyne/utils/logger.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    // Configure the engine
    graphyne::Engine::Config config;
    config.appName = "Graphyne Simple Window";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.enableValidation = true;
    config.enableVSync = true;

    // Create and initialize the engine
    graphyne::Engine engine(config);
    if (!engine.initialize())
    {
        graphyne::utils::error("Failed to initialize Graphyne engine");
        return 1;
    }

    // Initialize input system
    auto& inputSystem = graphyne::input::InputSystem::getInstance();
    if (!inputSystem.initialize())
    {
        graphyne::utils::error("Failed to initialize input system");
        return 1;
    }

    // Create some input actions
    auto& moveUp = inputSystem.createAction("MoveUp").bindKey(SDLK_w);
    auto& moveDown = inputSystem.createAction("MoveDown").bindKey(SDLK_s);
    auto& moveLeft = inputSystem.createAction("MoveLeft").bindKey(SDLK_a);
    auto& moveRight = inputSystem.createAction("MoveRight").bindKey(SDLK_d);
    auto& quitAction = inputSystem.createAction("Quit").bindKey(SDLK_ESCAPE);

    // Add callbacks for the actions
    inputSystem.addActionCallback("Quit", [&engine]() {
        graphyne::utils::info("Quit action triggered");
        engine.stop();
    });

    // Run the engine
    int result = engine.run();

    // Shut down the input system
    inputSystem.shutdown();

    // Shut down the engine
    engine.shutdown();
    graphyne::utils::info("Simple Window Example Exiting");

    return result;
}
