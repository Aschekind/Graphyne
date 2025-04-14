/**
 * @file simple_window.cpp
 * @brief Simple example showing window creation with Graphyne engine
 */
#include "core/engine.h"
#include "utils/logger.h"

#include <iostream>

int main(int argc, char* argv[])
{
    // Initialize the logger
    graphyne::utils::Logger::getInstance().initialize("simple_window.log", graphyne::utils::LogLevel::Debug);
    graphyne::utils::info("Simple Window Example Starting");

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

    // Run the engine
    int result = engine.run();

    // Shut down the engine
    engine.shutdown();
    graphyne::utils::info("Simple Window Example Exiting");

    return result;
}
