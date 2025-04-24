/**
 * @file simple_window.cpp
 * @brief Simple example showing window creation with Graphyne engine
 */
/**
 * @file enhanced_simple_window.cpp
 * @brief Enhanced example showcasing various Graphyne engine features
 */

#include "graphyne/controls/input_system.hpp"
#include "graphyne/core/ecs/ecs.hpp"
#include "graphyne/core/engine.hpp"
#include "graphyne/core/memory.hpp"
#include "graphyne/events/event_system.hpp"
#include "graphyne/utils/logger.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

// Define some example components
struct TransformComponent : public graphyne::ecs::Component
{
    float x, y, z;
    float rotationX, rotationY, rotationZ;
    float scaleX, scaleY, scaleZ;

    TransformComponent(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f)
        : x(posX),
          y(posY),
          z(posZ),
          rotationX(0.0f),
          rotationY(0.0f),
          rotationZ(0.0f),
          scaleX(1.0f),
          scaleY(1.0f),
          scaleZ(1.0f)
    {
    }
};

struct VelocityComponent : public graphyne::ecs::Component
{
    float vx, vy, vz;

    VelocityComponent(float velocityX = 0.0f, float velocityY = 0.0f, float velocityZ = 0.0f)
        : vx(velocityX), vy(velocityY), vz(velocityZ)
    {
    }
};

struct RenderableComponent : public graphyne::ecs::Component
{
    std::string name;
    uint32_t color;

    RenderableComponent(const std::string& objName = "default", uint32_t objColor = 0xFFFFFFFF)
        : name(objName), color(objColor)
    {
    }
};

struct PlayerControlledComponent : public graphyne::ecs::Component
{
    float speed;

    PlayerControlledComponent(float moveSpeed = 5.0f) : speed(moveSpeed) {}
};

// Define some example systems
class PhysicsSystem : public graphyne::ecs::System
{
public:
    PhysicsSystem(graphyne::ecs::World* world) : graphyne::ecs::System(world)
    {
        // Require components for this system
        requireComponent<TransformComponent>();
        requireComponent<VelocityComponent>();
    }

    void update(float deltaTime) override
    {
        // Simple physics movement
        for (auto entity : m_entities)
        {
            auto& transform = entity->getComponent<TransformComponent>();
            auto& velocity = entity->getComponent<VelocityComponent>();

            transform.x += velocity.vx * deltaTime;
            transform.y += velocity.vy * deltaTime;
            transform.z += velocity.vz * deltaTime;

            // Log position occasionally to show entity movement
            if (rand() % 100 < 5) // 5% chance to log per frame
            {
                GN_DEBUG("Entity {} position: ({}, {}, {})", entity->getID(), transform.x, transform.y, transform.z);
            }
        }
    }
};

class RenderSystem : public graphyne::ecs::System
{
public:
    RenderSystem(graphyne::ecs::World* world) : graphyne::ecs::System(world)
    {
        // Require components for this system
        requireComponent<TransformComponent>();
        requireComponent<RenderableComponent>();
    }

    void update(float deltaTime) override
    {
        // Simulate rendering
        static float renderTime = 0.0f;
        renderTime += deltaTime;

        // Only log render actions occasionally to not flood the console
        if (renderTime >= 1.0f)
        {
            GN_INFO("Rendering {} entities", m_entities.size());
            renderTime = 0.0f;

            // For demonstration purposes, render first few entities
            int countToShow = std::min(static_cast<int>(m_entities.size()), 3);
            for (int i = 0; i < countToShow; i++)
            {
                auto& transform = m_entities[i]->getComponent<TransformComponent>();
                auto& renderable = m_entities[i]->getComponent<RenderableComponent>();

                GN_DEBUG("Rendering '{}' at ({:.2f}, {:.2f}, {:.2f}) with color 0x{:08X}",
                         renderable.name,
                         transform.x,
                         transform.y,
                         transform.z,
                         renderable.color);
            }
        }
    }
};

class PlayerControlSystem : public graphyne::ecs::System
{
public:
    PlayerControlSystem(graphyne::ecs::World* world) : graphyne::ecs::System(world)
    {
        // Require components for this system
        requireComponent<TransformComponent>();
        requireComponent<VelocityComponent>();
        requireComponent<PlayerControlledComponent>();
    }

    void initialize() override
    {
        // Register for input events
        auto& eventSystem = graphyne::events::EventSystem::getInstance();

        // Subscribe to player input events
        m_inputSubscriptionId = eventSystem.subscribeToAll([this](graphyne::events::Event& event) {
            // Here we would process input events specifically for player control
        });
    }

    void update(float deltaTime) override
    {
        for (auto entity : m_entities)
        {
            auto& velocity = entity->getComponent<VelocityComponent>();
            auto& playerControl = entity->getComponent<PlayerControlledComponent>();

            // Get input values
            float moveX = 0.0f;
            float moveY = 0.0f;

            // In a real game, we'd get these from the input system
            // For this example, we'll just oscillate the movement for demonstration
            static float time = 0.0f;
            time += deltaTime;

            moveX = sin(time) * playerControl.speed;
            moveY = cos(time * 1.5f) * playerControl.speed;

            // Update velocity based on input
            velocity.vx = moveX;
            velocity.vy = moveY;
        }
    }

private:
    size_t m_inputSubscriptionId = 0;
};

// Custom event types for our example
namespace ExampleEvents
{
struct EntitySpawnedData
{
    graphyne::ecs::EntityID entityId;
    std::string name;
};

struct EntitySpawnedEvent : public graphyne::events::EventImpl<EntitySpawnedData>
{
    EntitySpawnedEvent(graphyne::ecs::EntityID id, const std::string& name) : EventImpl(EntitySpawnedData{id, name}) {}
};

struct GameStateChangedData
{
    std::string oldState;
    std::string newState;
};

struct GameStateChangedEvent : public graphyne::events::EventImpl<GameStateChangedData>
{
    GameStateChangedEvent(const std::string& oldState, const std::string& newState)
        : EventImpl(GameStateChangedData{oldState, newState})
    {
    }
};
} // namespace ExampleEvents

// Main application class
class SimpleGameApp
{
public:
    SimpleGameApp()
    {
        // Configure the engine
        m_engineConfig.appName = "Graphyne Enhanced Window";
        m_engineConfig.windowWidth = 1280;
        m_engineConfig.windowHeight = 720;
        m_engineConfig.enableValidation = true;
        m_engineConfig.enableVSync = true;
    }

    bool initialize()
    {
        // Create and initialize the engine
        m_engine = std::make_unique<graphyne::Engine>(m_engineConfig);
        if (!m_engine->initialize())
        {
            graphyne::utils::error("Failed to initialize Graphyne engine");
            return false;
        }

        // Initialize input system
        auto& inputSystem = graphyne::input::InputSystem::getInstance();
        if (!inputSystem.initialize())
        {
            graphyne::utils::error("Failed to initialize input system");
            return false;
        }

        // Create some input actions
        auto& moveUp = inputSystem.createAction("MoveUp").bindKey(SDLK_w);
        auto& moveDown = inputSystem.createAction("MoveDown").bindKey(SDLK_s);
        auto& moveLeft = inputSystem.createAction("MoveLeft").bindKey(SDLK_a);
        auto& moveRight = inputSystem.createAction("MoveRight").bindKey(SDLK_d);
        auto& quitAction = inputSystem.createAction("Quit").bindKey(SDLK_ESCAPE);
        auto& spawnEntity = inputSystem.createAction("SpawnEntity").bindKey(SDLK_SPACE);

        // Add callbacks for the actions
        inputSystem.addActionCallback("Quit", [this]() {
            graphyne::utils::info("Quit action triggered");
            m_engine->stop();
        });

        inputSystem.addActionCallback("SpawnEntity", [this]() {
            graphyne::utils::info("Spawning new entity");
            createRandomEntity();
        });

        // Set up event system handlers
        setupEventSystem();

        // Initialize ECS world
        m_world = std::make_unique<graphyne::ecs::World>();

        // Register systems
        m_physicsSystem = &m_world->registerSystem<PhysicsSystem>();
        m_renderSystem = &m_world->registerSystem<RenderSystem>();
        m_playerControlSystem = &m_world->registerSystem<PlayerControlSystem>();

        // Set system update order
        m_world->setSystemUpdateOrder({m_playerControlSystem, m_physicsSystem, m_renderSystem});

        // Create initial entities
        createInitialEntities();

        // Signal application ready
        auto& eventSystem = graphyne::events::EventSystem::getInstance();
        eventSystem.publish<ExampleEvents::GameStateChangedEvent>("Initializing", "Ready");

        graphyne::utils::info("SimpleGameApp initialization completed");
        return true;
    }

    void run()
    {
        if (m_engine)
        {
            graphyne::utils::info("Starting SimpleGameApp main loop");
            m_engine->run();
        }
    }

    void shutdown()
    {
        graphyne::utils::info("Shutting down SimpleGameApp");

        // Clean up ECS world
        m_world.reset();

        // Shut down input system
        graphyne::input::InputSystem::getInstance().shutdown();

        // Shut down the engine
        if (m_engine)
        {
            m_engine->shutdown();
        }

        graphyne::utils::info("SimpleGameApp shutdown completed");
    }

private:
    void setupEventSystem()
    {
        auto& eventSystem = graphyne::events::EventSystem::getInstance();

        // Subscribe to entity spawned events
        eventSystem.subscribe<ExampleEvents::EntitySpawnedEvent>([](ExampleEvents::EntitySpawnedEvent& event) {
            const auto& data = event.getData();
            GN_INFO("Entity spawned: ID={}, Name='{}'", data.entityId, data.name);
        });

        // Subscribe to game state change events
        eventSystem.subscribe<ExampleEvents::GameStateChangedEvent>([](ExampleEvents::GameStateChangedEvent& event) {
            const auto& data = event.getData();
            GN_INFO("Game state changed: {} -> {}", data.oldState, data.newState);
        });

        // Subscribe to window resize events
        eventSystem.subscribe<graphyne::events::CommonEvents::WindowResizeEvent>(
            [](graphyne::events::CommonEvents::WindowResizeEvent& event) {
                const auto& data = event.getData();
                GN_INFO("Window resized: {}x{}", data.width, data.height);
            });
    }

    void createInitialEntities()
    {
        graphyne::utils::info("Creating initial entities");

        // Create player entity
        auto& playerEntity = m_world->createEntity();
        playerEntity.addComponent<TransformComponent>(0.0f, 0.0f, 0.0f);
        playerEntity.addComponent<VelocityComponent>();
        playerEntity.addComponent<RenderableComponent>("Player", 0xFF0000FF); // Red color
        playerEntity.addComponent<PlayerControlledComponent>(10.0f);

        GN_INFO("Created player entity with ID: {}", playerEntity.getID());

        // Create some other entities
        for (int i = 0; i < 5; ++i)
        {
            createRandomEntity();
        }
    }

    void createRandomEntity()
    {
        static int entityCounter = 0;
        entityCounter++;

        auto& entity = m_world->createEntity();

        // Random position
        float x = (rand() % 20) - 10.0f;
        float y = (rand() % 20) - 10.0f;
        float z = (rand() % 20) - 10.0f;

        entity.addComponent<TransformComponent>(x, y, z);

        // Random velocity
        float vx = (rand() % 10 - 5) / 5.0f;
        float vy = (rand() % 10 - 5) / 5.0f;
        float vz = (rand() % 10 - 5) / 5.0f;

        entity.addComponent<VelocityComponent>(vx, vy, vz);

        // Random color (ARGB format)
        uint32_t color = 0xFF000000 |            // Alpha
                         (rand() % 256) |        // Blue
                         ((rand() % 256) << 8) | // Green
                         ((rand() % 256) << 16); // Red

        std::string name = "Entity_" + std::to_string(entityCounter);
        entity.addComponent<RenderableComponent>(name, color);

        // Publish entity spawned event
        auto& eventSystem = graphyne::events::EventSystem::getInstance();
        eventSystem.publish<ExampleEvents::EntitySpawnedEvent>(entity.getID(), name);
    }

    std::unique_ptr<graphyne::Engine> m_engine;
    graphyne::Engine::Config m_engineConfig;
    std::unique_ptr<graphyne::ecs::World> m_world;

    // Systems
    PhysicsSystem* m_physicsSystem = nullptr;
    RenderSystem* m_renderSystem = nullptr;
    PlayerControlSystem* m_playerControlSystem = nullptr;
};

int main(int argc, char* argv[])
{
    // Seed random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    // Create and run our application
    SimpleGameApp app;

    if (app.initialize())
    {
        app.run();
    }

    app.shutdown();
    return 0;
}
