#include <graphyne/core/engine.hpp>
#include <gtest/gtest.h>
#include <iostream>

using namespace graphyne;

class EngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        std::cout << "Setting up EngineTest..." << std::endl;
        // Create engine with test config
        Engine::Config config;
        config.enableValidation = false; // Disable validation for tests
        config.enableVSync = false;      // Disable VSync for tests
        engine = std::make_unique<Engine>(config);
    }

    void TearDown() override
    {
        std::cout << "Tearing down EngineTest..." << std::endl;
        if (engine)
        {
            engine->shutdown();
            engine.reset();
        }
    }

    std::unique_ptr<Engine> engine;
};

TEST_F(EngineTest, DefaultState)
{
    std::cout << "Running DefaultState test..." << std::endl;
    // Test initial state
    EXPECT_FALSE(engine->isRunning()) << "Engine should not be running initially";

    // Test initialization
    EXPECT_TRUE(engine->initialize()) << "Engine initialization should succeed";
    EXPECT_FALSE(engine->isRunning()) << "Engine should not be running after initialization";
}

TEST_F(EngineTest, ConfigConstruction)
{
    std::cout << "Running ConfigConstruction test..." << std::endl;
    // Test construction with custom config
    Engine::Config config;
    config.appName = "Test Application";
    config.windowWidth = 800;
    config.windowHeight = 600;
    config.enableValidation = false;
    config.enableVSync = false;

    auto customEngine = std::make_unique<Engine>(config);
    EXPECT_FALSE(customEngine->isRunning()) << "Newly constructed engine should not be running";
}

TEST_F(EngineTest, StartStop)
{
    std::cout << "Running StartStop test..." << std::endl;
    // Test start/stop functionality
    EXPECT_FALSE(engine->isRunning()) << "Engine should not be running initially";
    engine->stop();
    EXPECT_FALSE(engine->isRunning()) << "Engine should still not be running after stop";
}

TEST_F(EngineTest, ConfigValidation)
{
    std::cout << "Running ConfigValidation test..." << std::endl;
    Engine::Config config;

    // Test default values
    EXPECT_EQ(config.appName, "Graphyne Application") << "Default app name should be 'Graphyne Application'";
    EXPECT_EQ(config.windowWidth, 1280) << "Default window width should be 1280";
    EXPECT_EQ(config.windowHeight, 720) << "Default window height should be 720";
    EXPECT_TRUE(config.enableValidation) << "Validation should be enabled by default";
    EXPECT_TRUE(config.enableVSync) << "VSync should be enabled by default";

    // Test custom values
    config.appName = "Custom App";
    config.windowWidth = 1920;
    config.windowHeight = 1080;
    config.enableValidation = false;
    config.enableVSync = false;

    EXPECT_EQ(config.appName, "Custom App") << "Custom app name not set correctly";
    EXPECT_EQ(config.windowWidth, 1920) << "Custom window width not set correctly";
    EXPECT_EQ(config.windowHeight, 1080) << "Custom window height not set correctly";
    EXPECT_FALSE(config.enableValidation) << "Custom validation setting not set correctly";
    EXPECT_FALSE(config.enableVSync) << "Custom VSync setting not set correctly";
}
