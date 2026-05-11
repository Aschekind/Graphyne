#include "utils/logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <vector>

namespace gn::utils {

namespace {

spdlog::level::level_enum to_spdlog(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return spdlog::level::trace;
        case LogLevel::Debug:   return spdlog::level::debug;
        case LogLevel::Info:    return spdlog::level::info;
        case LogLevel::Warning: return spdlog::level::warn;
        case LogLevel::Error:   return spdlog::level::err;
        case LogLevel::Fatal:   return spdlog::level::critical;
        case LogLevel::Off:     return spdlog::level::off;
    }
    return spdlog::level::info;
}

} // namespace

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

bool Logger::initialize(const std::string& log_file, LogLevel level, bool to_console) {
    if (m_initialized) {
        return true;
    }

    try {
        auto dist = std::make_shared<spdlog::sinks::dist_sink_mt>();

        if (to_console) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            dist->add_sink(console_sink);
        }

        if (!log_file.empty()) {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, /*truncate=*/false);
            dist->add_sink(file_sink);
        }

        auto logger = std::make_shared<spdlog::logger>("graphyne", dist);
        logger->set_level(to_spdlog(level));
        logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%^%l%$] [%s:%#] %v");
        logger->flush_on(spdlog::level::warn);

        spdlog::set_default_logger(logger);
        spdlog::set_level(to_spdlog(level));

        m_level       = level;
        m_initialized = true;
        return true;
    } catch (const std::exception&) {
        // We can't log this through spdlog because it might be the thing that failed.
        return false;
    }
}

void Logger::shutdown() {
    if (!m_initialized) return;
    spdlog::shutdown();
    m_initialized = false;
}

void Logger::set_level(LogLevel level) {
    m_level = level;
    if (m_initialized) {
        spdlog::set_level(to_spdlog(level));
    }
}

} // namespace gn::utils
