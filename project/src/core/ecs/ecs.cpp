/**
 * @file ecs.cpp
 * @brief Implementation of Entity-Component-System architecture
 */
#include "graphyne/core/ecs/ecs.hpp"
#include "graphyne/events/event_system.hpp"
#include "graphyne/utils/logger.hpp"

namespace graphyne::ecs
{

// World implementation
World::World() : m_nextEntityID(0) {}

World::~World()
{
    // Explicitly clear all systems before entities to allow systems to do cleanup
    m_systemsByUpdateOrder.clear();
    for (auto& system : m_systems)
    {
        system.reset();
    }

    // Clear all entities
    for (auto& entity : m_entities)
    {
        if (entity && entity->isAlive())
        {
            // Don't trigger events during shutdown
            entity->m_alive = false;
        }
    }
    m_entities.clear();

    // Clear component pools
    for (auto& pool : m_componentPools)
    {
        pool.reset();
    }
}

Entity& World::createEntity()
{
    EntityID id;

    // Reuse ID if there are free ones available
    if (!m_freeEntityIDs.empty())
    {
        id = m_freeEntityIDs.front();
        m_freeEntityIDs.pop();

        // Reuse existing entity slot
        m_entities[id].reset(new Entity(id, this));
    }
    else
    {
        // Create new entity
        id = m_nextEntityID++;

        // Resize entities vector if needed
        if (id >= m_entities.size())
        {
            m_entities.resize(id + 1);
        }

        m_entities[id].reset(new Entity(id, this));
    }

    // Publish entity created event
    graphyne::events::EventSystem::getInstance().publish<events::EntityCreatedEvent>(id);

    return *m_entities[id];
}

void World::destroyEntity(Entity& entity)
{
    if (!entity.isAlive())
        return;

    EntityID id = entity.getID();

    // Add to pending destroy list
    m_pendingDestroyEntities.push_back(id);
}

void World::update(float deltaTime)
{
    // Update all systems in order
    for (auto* system : m_systemsByUpdateOrder)
    {
        system->update(deltaTime);
    }

    // Process any pending entity changes
    processPendingChanges();
}

void World::processPendingChanges()
{
    // Handle entity deletions
    for (EntityID id : m_pendingDestroyEntities)
    {
        if (id >= m_entities.size() || !m_entities[id] || !m_entities[id]->isAlive())
            continue;

        Entity* entity = m_entities[id].get();

        // Remove entity from all systems
        for (auto& system : m_systems)
        {
            if (system)
            {
                auto& entityList = system->m_entities;
                entityList.erase(std::remove(entityList.begin(), entityList.end(), entity), entityList.end());
            }
        }

        // Remove all components
        for (ComponentTypeID typeID = 0; typeID < MAX_COMPONENTS; typeID++)
        {
            if (entity->m_componentMask.test(typeID))
            {
                removeComponentFromEntity(*entity, typeID);
            }
        }

        // Notify destruction
        graphyne::events::EventSystem::getInstance().publish<events::EntityDestroyedEvent>(id);

        // Free the entity
        m_entities[id].reset();
        m_freeEntityIDs.push(id);
    }

    m_pendingDestroyEntities.clear();
}

Entity* World::getEntityByID(EntityID id)
{
    if (id >= m_entities.size() || !m_entities[id] || !m_entities[id]->isAlive())
        return nullptr;

    return m_entities[id].get();
}

Component* World::getComponentForEntity(Entity& entity, ComponentTypeID componentID)
{
    if (!entity.m_componentMask.test(componentID))
        return nullptr;

    // Ensure pool exists
    ensureComponentPoolExists(componentID);

    // Get component from pool
    size_t index = entity.m_componentIndices[componentID];
    return static_cast<Component*>(m_componentPools[componentID]->get(index));
}

void World::removeComponentFromEntity(Entity& entity, ComponentTypeID componentID)
{
    if (!entity.m_componentMask.test(componentID))
        return;

    // Get component pool
    ensureComponentPoolExists(componentID);

    // Get component index
    size_t index = entity.m_componentIndices[componentID];

    // Notify about component removal
    graphyne::events::EventSystem::getInstance().publish<events::ComponentRemovedEvent>(entity.getID(), componentID);

    // Remove component from pool
    m_componentPools[componentID]->remove(index);

    // Update component mask
    entity.m_componentMask.reset(componentID);

    // If this wasn't the last component in the pool, update the entity that owned
    // the moved component
    if (index < m_componentPools[componentID]->size())
    {
        // The last component was moved to this index, find which entity owns it
        Component* movedComponent = static_cast<Component*>(m_componentPools[componentID]->get(index));
        Entity* ownerEntity = movedComponent->entity;

        // Update the owner entity's component index
        ownerEntity->m_componentIndices[componentID] = index;
    }

    // Update systems that might be interested in this entity
    updateEntitySystemList(entity);
}

void World::updateEntitySystemList(Entity& entity)
{
    for (auto& system : m_systems)
    {
        if (!system)
            continue;

        const auto& requiredComponents = system->getRequiredComponents();
        const auto& excludedComponents = system->getExcludedComponents();

        // Check if entity has all required components and none of the excluded components
        bool hasRequiredComponents = (entity.m_componentMask & requiredComponents) == requiredComponents;
        bool hasExcludedComponents = (entity.m_componentMask & excludedComponents).any();

        // Get current entity list for this system
        auto& entityList = system->m_entities;

        // Check if entity should be in this system
        bool shouldBeInSystem = hasRequiredComponents && !hasExcludedComponents && entity.isAlive();

        // Find entity in the system's entity list
        auto it = std::find(entityList.begin(), entityList.end(), &entity);
        bool isInSystem = (it != entityList.end());

        // Add or remove as needed
        if (shouldBeInSystem && !isInSystem)
        {
            entityList.push_back(&entity);
        }
        else if (!shouldBeInSystem && isInSystem)
        {
            entityList.erase(it);
        }
    }
}

void World::ensureComponentPoolExists(ComponentTypeID componentID)
{
    if (!m_componentPools[componentID])
    {
        size_t componentSize = ComponentRegistry::getComponentSize(componentID);
        m_componentPools[componentID] = std::make_unique<ComponentPool>(componentSize);
    }
}

std::vector<Entity*> World::getEntitiesWithComponents(const ComponentMask& componentMask)
{
    std::vector<Entity*> result;

    for (auto& entityPtr : m_entities)
    {
        if (entityPtr && entityPtr->isAlive() && (entityPtr->getComponentMask() & componentMask) == componentMask)
        {
            result.push_back(entityPtr.get());
        }
    }

    return result;
}

} // namespace graphyne::ecs
