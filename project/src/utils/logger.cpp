#include "utils/logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>

namespace graphyne::utils
{

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

bool Logger::initialize(const std::string& logFile, LogLevel level, bool toConsole)
{
    if (m_initialized)
    {
        GN_WARNING("Logger already initialized");
        return true;
    }

    m_logLevel = level;
    m_toConsole = toConsole;

    if (!logFile.empty())
    {
        m_fileStream.open(logFile, std::ios::out | std::ios::app);
        if (!m_fileStream.is_open())
        {
            GN_ERROR("Failed to open log file: {}", logFile);
            return false;
        }
    }

    m_initialized = true;
    return true;
}

void Logger::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    if (m_fileStream.is_open())
    {
        m_fileStream.close();
    }

    m_initialized = false;
}

void Logger::setLogLevel(LogLevel level)
{
    m_logLevel = level;
}

void Logger::log(LogLevel level, const std::string& message)
{
    if (!m_initialized || level < m_logLevel)
    {
        return;
    }

    std::string formattedMessage = formatMessage(level, message);

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_toConsole)
    {
        std::cout << formattedMessage << std::endl;
    }

    if (m_fileStream.is_open())
    {
        m_fileStream << formattedMessage << std::endl;
        m_fileStream.flush();
    }
}

void Logger::trace(const std::string& message)
{
    log(LogLevel::Trace, message);
}

void Logger::debug(const std::string& message)
{
    log(LogLevel::Debug, message);
}

void Logger::info(const std::string& message)
{
    log(LogLevel::Info, message);
}

void Logger::warning(const std::string& message)
{
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message)
{
    log(LogLevel::Error, message);
}

void Logger::fatal(const std::string& message)
{
    log(LogLevel::Fatal, message);
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

std::string Logger::formatMessage(LogLevel level, const std::string& message)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    ss << " [" << levelToString(level) << "] " << message;

    return ss.str();
}

// Global logging functions
void trace(const std::string& message)
{
    Logger::getInstance().trace(message);
}

void debug(const std::string& message)
{
    Logger::getInstance().debug(message);
}

void info(const std::string& message)
{
    Logger::getInstance().info(message);
}

void warning(const std::string& message)
{
    Logger::getInstance().warning(message);
}

void error(const std::string& message)
{
    Logger::getInstance().error(message);
}

void fatal(const std::string& message)
{
    Logger::getInstance().fatal(message);
}

} // namespace graphyne::utils
