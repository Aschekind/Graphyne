/**
 * @file ecs.hpp
 * @brief Entity-Component-System architecture for Graphyne
 */
#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>

#include "graphyne/core/memory.hpp"
#include "graphyne/events/event_system.hpp"

namespace graphyne::ecs
{

// Forward declarations
class World;
class System;
class Entity;
class ComponentRegistry;
struct Component;

// Type definitions
using EntityID = uint32_t;
using ComponentTypeID = uint32_t;
using SystemTypeID = uint32_t;

// Constants for maximum number of components and systems
constexpr size_t MAX_COMPONENTS = 64;
constexpr size_t MAX_SYSTEMS = 32;

// ComponentMask using bitset to track which components an entity has
using ComponentMask = std::bitset<MAX_COMPONENTS>;

/**
 * @class ComponentRegistry
 * @brief Registry for component types
 */
class ComponentRegistry
{
public:
    /**
     * @brief Register a component type
     * @tparam T Component type
     * @return ID for the component type
     */
    template <typename T>
    static ComponentTypeID registerComponentType()
    {
        static_assert(std::is_base_of<Component, T>::value, "Type must derive from Component");

        ComponentTypeID id = getNextComponentTypeID();
        assert(id < MAX_COMPONENTS && "Too many component types registered");

        // Register component size and alignment for pool allocation
        getComponentSizes()[id] = sizeof(T);

        return id;
    }

    /**
     * @brief Get the size of a component type
     * @param id Component type ID
     * @return Size of the component type
     */
    static size_t getComponentSize(ComponentTypeID id)
    {
        assert(id < MAX_COMPONENTS);
        return getComponentSizes()[id];
    }

private:
    /**
     * @brief Get the next component type ID
     * @return Next available component type ID
     */
    static ComponentTypeID getNextComponentTypeID()
    {
        static ComponentTypeID nextID = 0;
        return nextID++;
    }

    /**
     * @brief Get array of component sizes
     * @return Reference to the array of component sizes
     */
    static std::array<size_t, MAX_COMPONENTS>& getComponentSizes()
    {
        static std::array<size_t, MAX_COMPONENTS> componentSizes = {};
        return componentSizes;
    }
};

/**
 * @brief Helper to get unique component type ID
 * @tparam T Component type
 * @return Unique ID for component type
 */
template <typename T>
ComponentTypeID getComponentTypeID()
{
    static ComponentTypeID typeID = ComponentRegistry::registerComponentType<T>();
    return typeID;
}

/**
 * @brief Helper to get unique system type ID
 * @tparam T System type
 * @return Unique ID for system type
 */
template <typename T>
SystemTypeID getSystemTypeID()
{
    static SystemTypeID typeID = T::getSystemTypeID();
    return typeID;
}

/**
 * @class Component
 * @brief Base class for all components
 */
struct Component
{
    Entity* entity = nullptr;

    Component() = default;
    virtual ~Component() = default;

    // Prevent copying components
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    // Allow moving components
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;
};

/**
 * @class ComponentPool
 * @brief Pool of components of a specific type
 */
class ComponentPool
{
public:
    ComponentPool(size_t componentSize, size_t initialCapacity = 100)
        : m_componentSize(componentSize), m_capacity(initialCapacity)
    {
        m_data = static_cast<uint8_t*>(graphyne::core::MemoryManager::getInstance().allocate(
            m_componentSize * m_capacity, 16, graphyne::core::AllocationType::General));
    }

    ~ComponentPool()
    {
        if (m_data)
        {
            graphyne::core::MemoryManager::getInstance().free(m_data, graphyne::core::AllocationType::General);
        }
    }

    // Prevent copying component pools
    ComponentPool(const ComponentPool&) = delete;
    ComponentPool& operator=(const ComponentPool&) = delete;

    // Allow moving component pools
    ComponentPool(ComponentPool&& other) noexcept
        : m_data(other.m_data),
          m_componentSize(other.m_componentSize),
          m_size(other.m_size),
          m_capacity(other.m_capacity)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    ComponentPool& operator=(ComponentPool&& other) noexcept
    {
        if (this != &other)
        {
            if (m_data)
            {
                graphyne::core::MemoryManager::getInstance().free(m_data, graphyne::core::AllocationType::General);
            }

            m_data = other.m_data;
            m_componentSize = other.m_componentSize;
            m_size = other.m_size;
            m_capacity = other.m_capacity;

            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    /**
     * @brief Get a component by index
     * @param index Index of the component
     * @return Pointer to the component
     */
    void* get(size_t index)
    {
        assert(index < m_size);
        return m_data + (index * m_componentSize);
    }

    /**
     * @brief Create a new component
     * @tparam T Component type
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to the new component
     */
    template <typename T, typename... Args>
    T* create(Args&&... args)
    {
        if (m_size >= m_capacity)
        {
            grow();
        }

        T* component = new (m_data + (m_size * m_componentSize)) T(std::forward<Args>(args)...);
        m_size++;
        return component;
    }

    /**
     * @brief Remove a component by index
     * @param index Index of the component to remove
     */
    void remove(size_t index)
    {
        assert(index < m_size);

        // Call destructor
        Component* component = static_cast<Component*>(get(index));
        component->~Component();

        // If it's not the last element, move the last element to this position
        if (index < m_size - 1)
        {
            std::memcpy(m_data + (index * m_componentSize), m_data + ((m_size - 1) * m_componentSize), m_componentSize);
        }

        m_size--;
    }

    /**
     * @brief Get the current size of the pool
     * @return Number of components in the pool
     */
    size_t size() const
    {
        return m_size;
    }

    /**
     * @brief Get the current capacity of the pool
     * @return Capacity of the pool
     */
    size_t capacity() const
    {
        return m_capacity;
    }

private:
    /**
     * @brief Grow the pool when full
     */
    void grow()
    {
        size_t newCapacity = m_capacity * 2;
        uint8_t* newData = static_cast<uint8_t*>(graphyne::core::MemoryManager::getInstance().allocate(
            m_componentSize * newCapacity, 16, graphyne::core::AllocationType::General));

        // Copy old data to new buffer
        std::memcpy(newData, m_data, m_componentSize * m_size);

        // Free old data
        graphyne::core::MemoryManager::getInstance().free(m_data, graphyne::core::AllocationType::General);

        m_data = newData;
        m_capacity = newCapacity;
    }

    uint8_t* m_data = nullptr;
    size_t m_componentSize = 0;
    size_t m_size = 0;
    size_t m_capacity = 0;
};

/**
 * @class Entity
 * @brief Represents a game entity with components
 */
class Entity
{
public:
    /**
     * @brief Constructor
     * @param id Entity ID
     * @param world World that owns this entity
     */
    Entity(EntityID id, World* world) : m_id(id), m_world(world), m_alive(true) {}

    /**
     * @brief Get the entity ID
     * @return Entity ID
     */
    EntityID getID() const
    {
        return m_id;
    }

    /**
     * @brief Check if entity is alive
     * @return True if entity is alive
     */
    bool isAlive() const
    {
        return m_alive;
    }

    /**
     * @brief Destroy this entity
     */
    void destroy();

    /**
     * @brief Add a component to this entity
     * @tparam T Component type
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Reference to the added component
     */
    template <typename T, typename... Args>
    T& addComponent(Args&&... args);

    /**
     * @brief Check if entity has a component
     * @tparam T Component type
     * @return True if entity has the component
     */
    template <typename T>
    bool hasComponent() const;

    /**
     * @brief Get a component from this entity
     * @tparam T Component type
     * @return Reference to the component
     */
    template <typename T>
    T& getComponent();

    /**
     * @brief Get a component from this entity (const version)
     * @tparam T Component type
     * @return Const reference to the component
     */
    template <typename T>
    const T& getComponent() const;

    /**
     * @brief Remove a component from this entity
     * @tparam T Component type
     */
    template <typename T>
    void removeComponent();

    /**
     * @brief Get component mask
     * @return Component mask for this entity
     */
    const ComponentMask& getComponentMask() const
    {
        return m_componentMask;
    }

private:
    EntityID m_id;
    World* m_world;
    bool m_alive;
    ComponentMask m_componentMask;

    // Maps component type ID to component index in the pool
    std::array<size_t, MAX_COMPONENTS> m_componentIndices;

    // Reference to the world is needed for many operations
    friend class World;
};

/**
 * @class System
 * @brief Base class for all systems
 */
class System
{
public:
    /**
     * @brief Constructor
     * @param world World that this system operates on
     */
    explicit System(World* world) : m_world(world) {}

    /**
     * @brief Destructor
     */
    virtual ~System() = default;

    /**
     * @brief Initialize the system
     */
    virtual void initialize() {}

    /**
     * @brief Update the system
     * @param deltaTime Time since last update
     */
    virtual void update(float deltaTime) = 0;

    /**
     * @brief Get the required component mask for entities
     * @return Component mask specifying required components
     */
    const ComponentMask& getRequiredComponents() const
    {
        return m_requiredComponents;
    }

    /**
     * @brief Get the excluded component mask for entities
     * @return Component mask specifying excluded components
     */
    const ComponentMask& getExcludedComponents() const
    {
        return m_excludedComponents;
    }

    /**
     * @brief Get system type ID (implemented by derived classes)
     * @return System type ID
     */
    static SystemTypeID getSystemTypeID();

protected:
    /**
     * @brief Require a component type for entities
     * @tparam T Component type
     */
    template <typename T>
    void requireComponent()
    {
        m_requiredComponents.set(getComponentTypeID<T>());
    }

    /**
     * @brief Exclude a component type for entities
     * @tparam T Component type
     */
    template <typename T>
    void excludeComponent()
    {
        m_excludedComponents.set(getComponentTypeID<T>());
    }

    World* m_world;
    ComponentMask m_requiredComponents;
    ComponentMask m_excludedComponents;
    std::vector<Entity*> m_entities;

    friend class World;
};

/**
 * @class SystemRegistry
 * @brief Registry for system types
 */
class SystemRegistry
{
public:
    /**
     * @brief Get the next system type ID
     * @return Next available system type ID
     */
    static SystemTypeID getNextSystemTypeID()
    {
        static SystemTypeID nextID = 0;
        return nextID++;
    }
};

/**
 * @brief Implementation for System::getSystemTypeID
 * @return System type ID
 */
inline SystemTypeID System::getSystemTypeID()
{
    static SystemTypeID typeID = SystemRegistry::getNextSystemTypeID();
    return typeID;
}

/**
 * @class World
 * @brief Manages entities, components, and systems
 */
class World
{
public:
    /**
     * @brief Constructor
     */
    World();

    /**
     * @brief Destructor
     */
    ~World();

    /**
     * @brief Create a new entity
     * @return Reference to the created entity
     */
    Entity& createEntity();

    /**
     * @brief Destroy an entity
     * @param entity Entity to destroy
     */
    void destroyEntity(Entity& entity);

    /**
     * @brief Register a system
     * @tparam T System type
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Reference to the created system
     */
    template <typename T, typename... Args>
    T& registerSystem(Args&&... args)
    {
        static_assert(std::is_base_of<System, T>::value, "System must derive from System");

        SystemTypeID typeID = getSystemTypeID<T>();
        assert(typeID < MAX_SYSTEMS && "Too many systems registered");

        // Create system
        auto system = std::make_unique<T>(this, std::forward<Args>(args)...);
        T& systemRef = *system;

        // Initialize system
        system->initialize();

        // Store system
        m_systems[typeID] = std::move(system);
        m_systemsByUpdateOrder.push_back(m_systems[typeID].get());

        return systemRef;
    }

    /**
     * @brief Get a system
     * @tparam T System type
     * @return Reference to the system
     */
    template <typename T>
    T& getSystem()
    {
        SystemTypeID typeID = getSystemTypeID<T>();
        assert(m_systems[typeID] && "System not registered");
        return *static_cast<T*>(m_systems[typeID].get());
    }

    /**
     * @brief Update all systems
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);

    /**
     * @brief Set system update order
     * @param systemUpdateOrder Vector of system pointers in desired update order
     */
    void setSystemUpdateOrder(const std::vector<System*>& systemUpdateOrder)
    {
        m_systemsByUpdateOrder = systemUpdateOrder;
    }

    /**
     * @brief Get all entities with specific components
     * @param componentMask Bit mask of required components
     * @return Vector of entity pointers
     */
    std::vector<Entity*> getEntitiesWithComponents(const ComponentMask& componentMask);

    /**
     * @brief Process pending entity changes
     * Called at the end of each update to handle entity creation/destruction
     */
    void processPendingChanges();

    /**
     * @brief Get entity by ID
     * @param id Entity ID
     * @return Pointer to entity, or nullptr if not found
     */
    Entity* getEntityByID(EntityID id);

    /**
     * @brief Get component from entity (for internal use)
     * @param entity Entity to get component from
     * @param componentID Component type ID
     * @return Pointer to component
     */
    Component* getComponentForEntity(Entity& entity, ComponentTypeID componentID);

    /**
     * @brief Add component to entity (for internal use)
     * @tparam T Component type
     * @tparam Args Constructor argument types
     * @param entity Entity to add component to
     * @param args Constructor arguments
     * @return Reference to added component
     */
    template <typename T, typename... Args>
    T& addComponentToEntity(Entity& entity, Args&&... args)
    {
        ComponentTypeID componentID = getComponentTypeID<T>();

        // Check if entity already has this component
        assert(!entity.m_componentMask.test(componentID) && "Entity already has this component");

        // Create component pool if it doesn't exist
        if (!m_componentPools[componentID])
        {
            m_componentPools[componentID] = std::make_unique<ComponentPool>(sizeof(T));
        }

        // Add component to pool
        T* component =
            static_cast<ComponentPool*>(m_componentPools[componentID].get())->create<T>(std::forward<Args>(args)...);
        component->entity = &entity;

        // Set component index for this entity
        entity.m_componentIndices[componentID] = m_componentPools[componentID]->size() - 1;

        // Update entity's component mask
        entity.m_componentMask.set(componentID);

        // Update systems that might be interested in this entity
        updateEntitySystemList(entity);

        return *component;
    }

    /**
     * @brief Remove component from entity (for internal use)
     * @param entity Entity to remove component from
     * @param componentID Component type ID
     */
    void removeComponentFromEntity(Entity& entity, ComponentTypeID componentID);

private:
    /**
     * @brief Update the list of systems an entity belongs to
     * @param entity Entity to update
     */
    void updateEntitySystemList(Entity& entity);

    /**
     * @brief Create component pools as needed
     * @param componentID Component type ID
     */
    void ensureComponentPoolExists(ComponentTypeID componentID);

    // Entity management
    EntityID m_nextEntityID;
    core::Vector<std::unique_ptr<Entity>, core::AllocationType::General> m_entities;
    std::queue<EntityID> m_freeEntityIDs;
    std::vector<EntityID> m_pendingDestroyEntities;

    // Component pools
    std::array<std::unique_ptr<ComponentPool>, MAX_COMPONENTS> m_componentPools;

    // Systems
    std::array<std::unique_ptr<System>, MAX_SYSTEMS> m_systems;
    std::vector<System*> m_systemsByUpdateOrder;
};

// Implementation for Entity methods that depend on World

template <typename T, typename... Args>
T& Entity::addComponent(Args&&... args)
{
    return m_world->addComponentToEntity<T>(*this, std::forward<Args>(args)...);
}

template <typename T>
bool Entity::hasComponent() const
{
    return m_componentMask.test(getComponentTypeID<T>());
}

template <typename T>
T& Entity::getComponent()
{
    assert(hasComponent<T>() && "Entity does not have this component");
    return *static_cast<T*>(m_world->getComponentForEntity(*this, getComponentTypeID<T>()));
}

template <typename T>
const T& Entity::getComponent() const
{
    assert(hasComponent<T>() && "Entity does not have this component");
    return *static_cast<T*>(m_world->getComponentForEntity(*const_cast<Entity*>(this), getComponentTypeID<T>()));
}

template <typename T>
void Entity::removeComponent()
{
    m_world->removeComponentFromEntity(*this, getComponentTypeID<T>());
}

inline void Entity::destroy()
{
    m_alive = false;
    m_world->destroyEntity(*this);
}

// Event types for ECS
namespace events
{
struct EntityCreatedData
{
    EntityID entityID;
};

struct EntityCreatedEvent : public graphyne::events::EventImpl<EntityCreatedData>
{
    explicit EntityCreatedEvent(EntityID id) : EventImpl(EntityCreatedData{id}) {}
};

struct EntityDestroyedData
{
    EntityID entityID;
};

struct EntityDestroyedEvent : public graphyne::events::EventImpl<EntityDestroyedData>
{
    explicit EntityDestroyedEvent(EntityID id) : EventImpl(EntityDestroyedData{id}) {}
};

struct ComponentAddedData
{
    EntityID entityID;
    ComponentTypeID componentTypeID;
    const char* componentTypeName;
};

struct ComponentAddedEvent : public graphyne::events::EventImpl<ComponentAddedData>
{
    ComponentAddedEvent(EntityID entityID, ComponentTypeID componentTypeID, const char* componentTypeName)
        : EventImpl(ComponentAddedData{entityID, componentTypeID, componentTypeName})
    {
    }
};

struct ComponentRemovedData
{
    EntityID entityID;
    ComponentTypeID componentTypeID;
};

struct ComponentRemovedEvent : public graphyne::events::EventImpl<ComponentRemovedData>
{
    ComponentRemovedEvent(EntityID entityID, ComponentTypeID componentTypeID)
        : EventImpl(ComponentRemovedData{entityID, componentTypeID})
    {
    }
};
} // namespace events

} // namespace graphyne::ecs
