/**
 * @file utils/logger.h
 * @brief Thin wrapper over spdlog. All engine logging goes through these macros.
 *
 * spdlog is configured once via Logger::initialize(). The macros expand to
 * `SPDLOG_INFO(...)` and friends, so they pick up source file / line / function
 * via spdlog's compile-time hook (SPDLOG_ACTIVE_LEVEL must be configured by
 * the consumer if they want to gate by level at compile time).
 */
#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>

#include <string>

namespace gn::utils {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Off,
};

class Logger {
public:
    static Logger& instance();

    /// Idempotent. Safe to call multiple times — subsequent calls are no-ops.
    /// `log_file` empty means console-only.
    bool initialize(const std::string& log_file = "",
                    LogLevel level = LogLevel::Info,
                    bool to_console = true);

    void shutdown();

    void set_level(LogLevel level);
    LogLevel level() const { return m_level; }

    bool initialized() const { return m_initialized; }

private:
    Logger() = default;
    ~Logger() = default;

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_level       = LogLevel::Info;
    bool     m_initialized = false;
};

} // namespace gn::utils

// Convenience macros mapped onto spdlog's source-location aware macros.
#define GN_TRACE(...)   SPDLOG_TRACE(__VA_ARGS__)
#define GN_DEBUG(...)   SPDLOG_DEBUG(__VA_ARGS__)
#define GN_INFO(...)    SPDLOG_INFO(__VA_ARGS__)
#define GN_WARNING(...) SPDLOG_WARN(__VA_ARGS__)
#define GN_ERROR(...)   SPDLOG_ERROR(__VA_ARGS__)
#define GN_FATAL(...)   SPDLOG_CRITICAL(__VA_ARGS__)
