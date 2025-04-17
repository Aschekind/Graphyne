#include "graphyne/core/event_system.hpp"

namespace graphyne::events
{

EventSystem& EventSystem::getInstance()
{
    static EventSystem instance;
    return instance;
}

size_t EventSystem::subscribeToAll(const EventCallback& callback)
{
    m_globalSubscribers.push_back(callback);
    return reinterpret_cast<size_t>(&m_globalSubscribers.back());
}

void EventSystem::unsubscribe(size_t subscriptionId)
{
    // Search in type-specific subscribers
    for (auto& [typeIndex, callbacks] : m_subscribers)
    {
        auto it = std::find_if(callbacks.begin(), callbacks.end(), [subscriptionId](const EventCallback& callback) {
            return reinterpret_cast<size_t>(&callback) == subscriptionId;
        });

        if (it != callbacks.end())
        {
            callbacks.erase(it);
            return;
        }
    }

    // Search in global subscribers
    auto it = std::find_if(
        m_globalSubscribers.begin(), m_globalSubscribers.end(), [subscriptionId](const EventCallback& callback) {
            return reinterpret_cast<size_t>(&callback) == subscriptionId;
        });

    if (it != m_globalSubscribers.end())
    {
        m_globalSubscribers.erase(it);
    }
}

void EventSystem::publishEvent(Event& event)
{
    std::type_index typeIndex(typeid(event));

    // Notify type-specific subscribers
    if (m_subscribers.find(typeIndex) != m_subscribers.end())
    {
        for (const auto& callback : m_subscribers[typeIndex])
        {
            if (!event.isHandled())
            {
                callback(event);
            }
        }
    }

    // Notify global subscribers
    for (const auto& callback : m_globalSubscribers)
    {
        if (!event.isHandled())
        {
            callback(event);
        }
    }
}

void EventSystem::clearSubscribers()
{
    m_subscribers.clear();
    m_globalSubscribers.clear();
}

} // namespace graphyne::events
