/**
 * @file event_system.hpp
 * @brief Event system for communication between subsystems
 */
#pragma once

#include <any>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "graphyne/core/memory.hpp"

namespace graphyne::events
{

/**
 * @class Event
 * @brief Base class for all event types
 */
class Event
{
public:
    virtual ~Event() = default;

    /**
     * @brief Get the type name of the event
     * @return String representation of event type
     */
    virtual std::string getTypeName() const = 0;

    /**
     * @brief Check if event has been handled
     * @return True if event has been handled
     */
    bool isHandled() const
    {
        return m_handled;
    }

    /**
     * @brief Mark event as handled
     * @param handled Whether the event is handled
     */
    void setHandled(bool handled = true)
    {
        m_handled = handled;
    }

private:
    bool m_handled = false;
};

/**
 * @brief Templated event implementation
 * @tparam T Type of event data
 */
template <typename T>
class EventImpl : public Event
{
public:
    /**
     * @brief Constructor
     * @param data Event data
     */
    explicit EventImpl(const T& data) : m_data(data) {}

    /**
     * @brief Get event data
     * @return Event data
     */
    const T& getData() const
    {
        return m_data;
    }

    /**
     * @brief Get the type name of the event
     * @return String representation of event type
     */
    std::string getTypeName() const override
    {
        return typeid(T).name();
    }

private:
    T m_data;
};

/**
 * @brief Event listener callback type
 */
using EventCallback = std::function<void(Event&)>;

/**
 * @class EventDispatcher
 * @brief Helper class for dispatching events to appropriate handlers
 */
class EventDispatcher
{
public:
    /**
     * @brief Constructor
     * @param event Event to dispatch
     */
    explicit EventDispatcher(Event& event) : m_event(event) {}

    /**
     * @brief Dispatch event to a handler if types match
     * @tparam T Event type to check against
     * @param func Function to call if types match
     * @return True if event was dispatched
     */
    template <typename T>
    bool dispatch(const std::function<void(T&)>& func)
    {
        if (m_event.getTypeName() == typeid(T).name())
        {
            if (!m_event.isHandled())
            {
                func(static_cast<T&>(m_event));
                return true;
            }
        }
        return false;
    }

private:
    Event& m_event;
};

/**
 * @class EventSystem
 * @brief System for publishing and subscribing to events
 */
class EventSystem
{
public:
    /**
     * @brief Get singleton instance of the event system
     * @return Reference to the event system instance
     */
    static EventSystem& getInstance();

    /**
     * @brief Subscribe to events of a specific type
     * @tparam T Event type to subscribe to
     * @param callback Function to call when event is published
     * @return Subscription ID for unsubscribing
     */
    template <typename T>
    size_t subscribe(const std::function<void(T&)>& callback)
    {
        auto wrappedCallback = [callback](Event& event) { callback(static_cast<T&>(event)); };

        std::lock_guard<std::mutex> lock(m_mutex);
        size_t id = getNextSubscriptionId();

        std::type_index typeIndex(typeid(T));
        m_subscribers[typeIndex].push_back({id, wrappedCallback, std::nullopt});
        return id;
    }

    /**
     * @brief Subscribe to events of a specific type with timeout
     * @tparam T Event type to subscribe to
     * @param callback Function to call when event is published
     * @param timeout Time after which subscription expires
     * @return Subscription ID for unsubscribing
     */
    template <typename T>
    size_t subscribeWithTimeout(const std::function<void(T&)>& callback, std::chrono::seconds timeout)
    {
        auto wrappedCallback = [callback](Event& event) { callback(static_cast<T&>(event)); };

        std::lock_guard<std::mutex> lock(m_mutex);
        size_t id = getNextSubscriptionId();

        std::type_index typeIndex(typeid(T));
        auto expiresAt = std::chrono::steady_clock::now() + timeout;
        m_subscribers[typeIndex].push_back({id, wrappedCallback, expiresAt});
        return id;
    }

    /**
     * @brief Subscribe to all events
     * @param callback Function to call when any event is published
     * @return Subscription ID for unsubscribing
     */
    size_t subscribeToAll(const EventCallback& callback);

    /**
     * @brief Subscribe to all events with timeout
     * @param callback Function to call when any event is published
     * @param timeout Time after which subscription expires
     * @return Subscription ID for unsubscribing
     */
    size_t subscribeToAllWithTimeout(const EventCallback& callback, std::chrono::seconds timeout)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t id = getNextSubscriptionId();
        auto expiresAt = std::chrono::steady_clock::now() + timeout;
        m_globalSubscribers.push_back({id, callback, expiresAt});
        return id;
    }

    /**
     * @brief Unsubscribe from events
     * @param subscriptionId Subscription ID returned from subscribe
     */
    void unsubscribe(size_t subscriptionId);

    /**
     * @brief Publish an event
     * @tparam T Event type
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     */
    template <typename T, typename... Args>
    void publish(Args&&... args)
    {
        T event(std::forward<Args>(args)...);
        publishEvent(event);
    }

    /**
     * @brief Publish an existing event
     * @param event Event to publish
     */
    void publishEvent(Event& event);

    /**
     * @brief Clear all subscribers
     */
    void clearSubscribers();

    /**
     * @brief Clean up stale subscribers that have timed out
     */
    void cleanupStaleSubscribers();

    /**
     * @brief Set a timeout for an existing subscription
     * @param subscriptionId ID of the subscription
     * @param timeout Time after which subscription expires
     */
    void setSubscriptionTimeout(size_t subscriptionId, std::chrono::seconds timeout);

    /**
     * @brief Check if a subscription is still active
     * @param subscriptionId ID of the subscription
     * @return True if the subscription is active
     */
    bool isSubscriptionActive(size_t subscriptionId);

private:
    // Private constructor for singleton
    EventSystem()
        : m_lastSubscriptionId(0),
          m_lastCleanupTime(std::chrono::steady_clock::now()),
          m_cleanupInterval(std::chrono::minutes(5)) // Clean up every 5 minutes by default
    {
    }

    // Deleted copy and move constructors and assignment operators
    EventSystem(const EventSystem&) = delete;
    EventSystem& operator=(const EventSystem&) = delete;
    EventSystem(EventSystem&&) = delete;
    EventSystem& operator=(EventSystem&&) = delete;

    /**
     * @brief Generate the next unique subscription ID
     * @return New subscription ID
     */
    size_t getNextSubscriptionId();

    /**
     * @struct SubscriptionEntry
     * @brief Stores subscription data with expiration information
     */
    struct SubscriptionEntry
    {
        size_t id;
        EventCallback callback;
        std::optional<std::chrono::time_point<std::chrono::steady_clock>> expiresAt;
    };

    core::UnorderedMap<std::type_index, core::Vector<SubscriptionEntry>, core::AllocationType::General> m_subscribers;
    core::Vector<SubscriptionEntry, core::AllocationType::General> m_globalSubscribers;
    std::mutex m_mutex;
    size_t m_lastSubscriptionId;
    std::chrono::time_point<std::chrono::steady_clock> m_lastCleanupTime;
    std::chrono::seconds m_cleanupInterval;
};

/**
 * @brief Common event types that might be useful across the engine
 */
namespace CommonEvents
{
/**
 * @struct WindowResizeData
 * @brief Data for window resize events
 */
struct WindowResizeData
{
    int width;
    int height;
};

/**
 * @struct WindowResizeEvent
 * @brief Triggered when window is resized
 */
struct WindowResizeEvent : public EventImpl<WindowResizeData>
{
    WindowResizeEvent(int w, int h) : EventImpl(WindowResizeData{w, h}) {}
};

/**
 * @struct WindowCloseData
 * @brief Data for window close events
 */
struct WindowCloseData
{
};

/**
 * @struct WindowCloseEvent
 * @brief Triggered when window is closed
 */
struct WindowCloseEvent : public EventImpl<WindowCloseData>
{
    WindowCloseEvent() : EventImpl(WindowCloseData{}) {}
};

/**
 * @struct AppTickData
 * @brief Data for app tick events
 */
struct AppTickData
{
    float deltaTime;
};

/**
 * @struct AppTickEvent
 * @brief Triggered on each app tick
 */
struct AppTickEvent : public EventImpl<AppTickData>
{
    explicit AppTickEvent(float dt) : EventImpl(AppTickData{dt}) {}
};

/**
 * @struct AppUpdateData
 * @brief Data for app update events
 */
struct AppUpdateData
{
    float deltaTime;
};

/**
 * @struct AppUpdateEvent
 * @brief Triggered on each app update
 */
struct AppUpdateEvent : public EventImpl<AppUpdateData>
{
    explicit AppUpdateEvent(float dt) : EventImpl(AppUpdateData{dt}) {}
};

/**
 * @struct AppRenderData
 * @brief Data for app render events
 */
struct AppRenderData
{
};

/**
 * @struct AppRenderEvent
 * @brief Triggered on each app render
 */
struct AppRenderEvent : public EventImpl<AppRenderData>
{
    AppRenderEvent() : EventImpl(AppRenderData{}) {}
};
} // namespace CommonEvents

} // namespace graphyne::events
