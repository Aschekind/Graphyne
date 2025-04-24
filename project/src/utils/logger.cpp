#include "graphyne/utils/logger.hpp"
#include <chrono>
#include <ctime>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <iomanip>
#include <source_location>

namespace graphyne::utils
{

SourceLocation SourceLocation::current(const char* file, int line)
{
    return {file, line};
}

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

bool Logger::initialize(const std::string& logFile, LogLevel level, bool toConsole)
{
    if (m_initialized)
    {
        warning("Logger already initialized");
        return true;
    }

    m_logLevel = level;
    m_toConsole = toConsole;

    if (!logFile.empty())
    {
        m_fileStream.open(logFile, std::ios::out | std::ios::app);
        if (!m_fileStream.is_open())
        {
            if (m_toConsole)
            {
                fmt::print(stderr, "Failed to open log file: {}\n", logFile);
            }
            return false;
        }
    }

    m_initialized = true;
    info("Logger initialized");
    return true;
}

void Logger::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    info("Logger shutting down");

    if (m_fileStream.is_open())
    {
        m_fileStream.close();
    }

    m_initialized = false;
}

void Logger::setLogLevel(LogLevel level)
{
    m_logLevel = level;
    info(fmt::format("Log level set to {}", levelToString(level)));
}

void Logger::log(LogLevel level, const std::string& message, const SourceLocation& location)
{
    if (!m_initialized || level < m_logLevel)
    {
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::string fileName = location.file;

    // Extract just the filename from the path
    size_t lastSlash = fileName.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        fileName = fileName.substr(lastSlash + 1);
    }

    // Format the log message using fmt
    std::string formattedMessage =
        fmt::format("{:%Y-%m-%d %H:%M:%S}.{:03d} [{}] [{}:{}] {}",
                    fmt::localtime(std::chrono::system_clock::to_time_t(now)),
                    std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000,
                    levelToString(level),
                    fileName,
                    location.line,
                    message);

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_toConsole)
    {
        // Use fmt::color to add colors based on log level
        switch (level)
        {
            case LogLevel::Trace:
                fmt::print(fg(fmt::color::gray), "{}\n", formattedMessage);
                break;
            case LogLevel::Debug:
                fmt::print(fg(fmt::color::light_blue), "{}\n", formattedMessage);
                break;
            case LogLevel::Info:
                fmt::print(fg(fmt::color::white), "{}\n", formattedMessage);
                break;
            case LogLevel::Warning:
                fmt::print(fg(fmt::color::yellow), "{}\n", formattedMessage);
                break;
            case LogLevel::Error:
                fmt::print(fg(fmt::color::red), "{}\n", formattedMessage);
                break;
            case LogLevel::Fatal:
                fmt::print(fg(fmt::color::dark_red) | fmt::emphasis::bold, "{}\n", formattedMessage);
                break;
            default:
                fmt::print("{}\n", formattedMessage);
                break;
        }
    }

    if (m_fileStream.is_open())
    {
        m_fileStream << formattedMessage << std::endl;
        m_fileStream.flush();
    }
}

void Logger::trace(const std::string& message, const SourceLocation& location)
{
    log(LogLevel::Trace, message, location);
}

void Logger::debug(const std::string& message, const SourceLocation& location)
{
    log(LogLevel::Debug, message, location);
}

void Logger::info(const std::string& message, const SourceLocation& location)
{
    log(LogLevel::Info, message, location);
}

void Logger::warning(const std::string& message, const SourceLocation& location)
{
    log(LogLevel::Warning, message, location);
}

void Logger::error(const std::string& message, const SourceLocation& location)
{
    log(LogLevel::Error, message, location);
}

void Logger::fatal(const std::string& message, const SourceLocation& location)
{
    log(LogLevel::Fatal, message, location);
}

std::string Logger::levelToString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARNING";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Fatal:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

// Modified global logging functions that capture source location
void trace(const std::string& message, const SourceLocation& location)
{
    Logger::getInstance().trace(message, location);
}

void debug(const std::string& message, const SourceLocation& location)
{
    Logger::getInstance().debug(message, location);
}

void info(const std::string& message, const SourceLocation& location)
{
    Logger::getInstance().info(message, location);
}

void warning(const std::string& message, const SourceLocation& location)
{
    Logger::getInstance().warning(message, location);
}

void error(const std::string& message, const SourceLocation& location)
{
    Logger::getInstance().error(message, location);
}

void fatal(const std::string& message, const SourceLocation& location)
{
    Logger::getInstance().fatal(message, location);
}

} // namespace graphyne::utils
