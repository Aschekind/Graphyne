/**
 * @file logger.h
 * @brief Simple logging utility for Graphyne
 */
#pragma once

#include <fmt/core.h>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

namespace graphyne::utils
{

/**
 * @enum LogLevel
 * @brief Severity levels for log messages
 */
enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/**
 * @class Logger
 * @brief Simple thread-safe logger with console and file output
 */
class Logger
{
public:
    /**
     * @brief Get singleton instance of the logger
     * @return Reference to the logger instance
     */
    static Logger& getInstance();

    /**
     * @brief Initialize the logger
     * @param logFile Path to the log file (empty for console-only logging)
     * @param level Minimum log level to display
     * @param toConsole Whether to output to console as well as file
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize(const std::string& logFile = "", LogLevel level = LogLevel::Info, bool toConsole = true);

    /**
     * @brief Shutdown the logger
     */
    void shutdown();

    /**
     * @brief Set the minimum log level
     * @param level New minimum log level
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Log a message with the specified level
     * @param level Severity level of the message
     * @param message Message to log
     */
    void log(LogLevel level, const std::string& message);

    /**
     * @brief Log a trace message
     * @param message Message to log
     */
    void trace(const std::string& message);

    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    void debug(const std::string& message);

    /**
     * @brief Log an info message
     * @param message Message to log
     */
    void info(const std::string& message);

    /**
     * @brief Log a warning message
     * @param message Message to log
     */
    void warning(const std::string& message);

    /**
     * @brief Log an error message
     * @param message Message to log
     */
    void error(const std::string& message);

    /**
     * @brief Log a fatal error message
     * @param message Message to log
     */
    void fatal(const std::string& message);

private:
    // Private constructor for singleton
    Logger() = default;

    // Deleted copy and move constructors and assignment operators
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Convert log level to string
    std::string levelToString(LogLevel level);

    // Format message with timestamp and level
    std::string formatMessage(LogLevel level, const std::string& message);

    LogLevel m_logLevel = LogLevel::Info;
    bool m_toConsole = true;
    std::ofstream m_fileStream;
    std::mutex m_mutex;
    bool m_initialized = false;
};

// Convenient global logging functions
void trace(const std::string& message);
void debug(const std::string& message);
void info(const std::string& message);
void warning(const std::string& message);
void error(const std::string& message);
void fatal(const std::string& message);

// Macro-based logging with source file and line information
#define GN_TRACE(...) ::graphyne::utils::trace(fmt::format(__VA_ARGS__))
#define GN_DEBUG(...) ::graphyne::utils::debug(fmt::format(__VA_ARGS__))
#define GN_INFO(...) ::graphyne::utils::info(fmt::format(__VA_ARGS__))
#define GN_WARNING(...) ::graphyne::utils::warning(fmt::format(__VA_ARGS__))
#define GN_ERROR(...) ::graphyne::utils::error(fmt::format(__VA_ARGS__))
#define GN_FATAL(...) ::graphyne::utils::fatal(fmt::format(__VA_ARGS__))

} // namespace graphyne::utils
