#include "graphyne/events/event_system.hpp"
#include <mutex>
#include <algorithm>

namespace graphyne::events
{

EventSystem& EventSystem::getInstance()
{
    static EventSystem instance;
    return instance;
}

size_t EventSystem::getNextSubscriptionId()
{
    // Thread-safe way to generate unique subscription IDs
    std::lock_guard<std::mutex> lock(m_mutex);
    return ++m_lastSubscriptionId;
}

size_t EventSystem::subscribeToAll(const EventCallback& callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t id = getNextSubscriptionId();
    m_globalSubscribers.push_back({id, callback});
    return id;
}

void EventSystem::unsubscribe(size_t subscriptionId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Search in type-specific subscribers
    for (auto& [typeIndex, callbacks] : m_subscribers)
    {
        auto it = std::find_if(callbacks.begin(), callbacks.end(),
            [subscriptionId](const SubscriptionEntry& entry) {
                return entry.id == subscriptionId;
            });

        if (it != callbacks.end())
        {
            callbacks.erase(it);
            return;
        }
    }

    // Search in global subscribers
    auto it = std::find_if(
        m_globalSubscribers.begin(), m_globalSubscribers.end(),
        [subscriptionId](const SubscriptionEntry& entry) {
            return entry.id == subscriptionId;
        });

    if (it != m_globalSubscribers.end())
    {
        m_globalSubscribers.erase(it);
    }
}

void EventSystem::publishEvent(Event& event)
{
    std::type_index typeIndex(typeid(event));

    // Use a local copy of subscribers to avoid holding the lock during callbacks
    std::vector<EventCallback> typeCallbacks;
    std::vector<EventCallback> globalCallbacks;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Copy type-specific subscribers
        if (m_subscribers.find(typeIndex) != m_subscribers.end())
        {
            for (const auto& entry : m_subscribers[typeIndex])
            {
                typeCallbacks.push_back(entry.callback);
            }
        }

        // Copy global subscribers
        for (const auto& entry : m_globalSubscribers)
        {
            globalCallbacks.push_back(entry.callback);
        }
    }

    // Notify type-specific subscribers
    for (const auto& callback : typeCallbacks)
    {
        if (!event.isHandled())
        {
            try {
                callback(event);
            } catch (const std::exception& e) {
                // Log exception but continue with other subscribers
                // In a real implementation, you would use your logger here
                // Logger::error("Exception in event callback: {}", e.what());
            }
        }
    }

    // Notify global subscribers
    for (const auto& callback : globalCallbacks)
    {
        if (!event.isHandled())
        {
            try {
                callback(event);
            } catch (const std::exception& e) {
                // Log exception but continue with other subscribers
                // Logger::error("Exception in event callback: {}", e.what());
            }
        }
    }
}

void EventSystem::clearSubscribers()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscribers.clear();
    m_globalSubscribers.clear();
}

void EventSystem::cleanupStaleSubscribers()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::steady_clock::now();

    // Check if we should perform cleanup
    if (now - m_lastCleanupTime < m_cleanupInterval)
    {
        return;
    }

    m_lastCleanupTime = now;

    // Clean up type-specific subscribers that haven't been active recently
    for (auto& [typeIndex, subscribers] : m_subscribers)
    {
        auto it = std::remove_if(subscribers.begin(), subscribers.end(),
            [this, now](const SubscriptionEntry& entry) {
                // If the subscription has an expiration time and it's passed
                return entry.expiresAt.has_value() &&
                       now > entry.expiresAt.value();
            });

        if (it != subscribers.end())
        {
            subscribers.erase(it, subscribers.end());
        }
    }

    // Clean up global subscribers
    auto it = std::remove_if(m_globalSubscribers.begin(), m_globalSubscribers.end(),
        [this, now](const SubscriptionEntry& entry) {
            return entry.expiresAt.has_value() &&
                   now > entry.expiresAt.value();
        });

    if (it != m_globalSubscribers.end())
    {
        m_globalSubscribers.erase(it, m_globalSubscribers.end());
    }
}

void EventSystem::setSubscriptionTimeout(size_t subscriptionId, std::chrono::seconds timeout)
{
    if (timeout.count() <= 0) {
        return;  // Ignore invalid timeouts
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto expiresAt = std::chrono::steady_clock::now() + timeout;

    // Set timeout for type-specific subscribers
    for (auto& [typeIndex, callbacks] : m_subscribers)
    {
        for (auto& entry : callbacks)
        {
            if (entry.id == subscriptionId)
            {
                entry.expiresAt = expiresAt;
                return;
            }
        }
    }

    // Set timeout for global subscribers
    for (auto& entry : m_globalSubscribers)
    {
        if (entry.id == subscriptionId)
        {
            entry.expiresAt = expiresAt;
            return;
        }
    }
}

bool EventSystem::isSubscriptionActive(size_t subscriptionId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check in type-specific subscribers
    for (const auto& [typeIndex, callbacks] : m_subscribers)
    {
        auto it = std::find_if(callbacks.begin(), callbacks.end(),
            [subscriptionId](const SubscriptionEntry& entry) {
                return entry.id == subscriptionId;
            });

        if (it != callbacks.end())
        {
            if (!it->expiresAt.has_value() ||
                std::chrono::steady_clock::now() <= it->expiresAt.value())
            {
                return true;
            }
            return false;
        }
    }

    // Check in global subscribers
    auto it = std::find_if(m_globalSubscribers.begin(), m_globalSubscribers.end(),
        [subscriptionId](const SubscriptionEntry& entry) {
            return entry.id == subscriptionId;
        });

    if (it != m_globalSubscribers.end())
    {
        if (!it->expiresAt.has_value() ||
            std::chrono::steady_clock::now() <= it->expiresAt.value())
        {
            return true;
        }
        return false;
    }

    return false; // Subscription not found
}

} // namespace graphyne::events
