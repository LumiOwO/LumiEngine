#pragma once

#include <cassert>

#include "ISingleton.h"

#pragma warning(push, 0)
#pragma warning(disable : 6285 6385 26812 26451 26498 26437 26495)

#ifdef LUMI_ENABLE_DEBUG_LOG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include "spdlog/async.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#pragma warning(pop)

namespace lumi {

struct LogWrapper final : public ISingleton<LogWrapper> {
    std::shared_ptr<spdlog::logger> logger;

    LogWrapper() {
        auto console_sink =
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%H:%M:%S.%e] (%s:%#) %^[%l]\n%v%$");
        console_sink->set_color(              //
            spdlog::level::level_enum::info,  //
            FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        console_sink->set_color(
            spdlog::level::level_enum::debug,
            FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        const spdlog::sinks_init_list sink_list = {console_sink};

        spdlog::init_thread_pool(8192, 1);

        logger = std::make_shared<spdlog::logger>(
            "logger", sink_list.begin(), sink_list.end());
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    ~LogWrapper() {
        logger->flush();
        spdlog::drop_all();
        spdlog::shutdown();
    }
};

#define __LOG(log_name, ...) \
    SPDLOG_LOGGER_##log_name(lumi::LogWrapper::Instance().logger, __VA_ARGS__)
#define LOG_INFO(fmt, ...)    __LOG(INFO, fmt, __VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   __LOG(DEBUG, fmt, __VA_ARGS__)
#define LOG_WARNING(fmt, ...) __LOG(WARN, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...)   __LOG(ERROR, fmt, __VA_ARGS__)
#define LOG_FATAL(fmt, ...)   __LOG(CRITICAL, fmt, __VA_ARGS__)

#pragma warning(disable : 4003)
#if defined(LUMI_FORCE_ASSERT) || !defined(NDEBUG)
#define LOG_ASSERT(exp, fmt, ...)                                              \
    (void)((!!(exp)) ||                                                        \
           (LOG_DEBUG("Assertion failed: " #exp ". "##fmt, __VA_ARGS__), 0) || \
           (assert(0), 0))
#else
#define LOG_ASSERT(exp, fmt, ...) ((void)0)
#endif
}  // namespace lumi
