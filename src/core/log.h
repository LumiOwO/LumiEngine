#pragma once

#include <cassert>

#include "singleton.h"

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
        console_sink->set_pattern("[%H:%M:%S.%e] (%s:%#, %!): \n%^[%l] %v%$");
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

#define LOG_(log_name, ...) \
    SPDLOG_LOGGER_##log_name(lumi::LogWrapper::Instance().logger, __VA_ARGS__)
#define LOG_INFO(fmt, ...)    LOG_(INFO, fmt, __VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   LOG_(DEBUG, fmt, __VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG_(WARN, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...)   LOG_(ERROR, fmt, __VA_ARGS__)
#define LOG_FATAL(fmt, ...)   LOG_(CRITICAL, fmt, __VA_ARGS__)

#pragma warning(disable : 4003)
#if defined(LUMI_FORCE_ASSERT) || !defined(NDEBUG)

// !!Important:  These macros are Microsoft specific
#define LOG_ASSERT_CALL_(X, Y)                             X Y
#define LOG_ASSERT_EXPAND_(...)                            __VA_ARGS__
#define LOG_ASSERT_GET_ARG4_(a0_, a1_, a2_, a3_, a4_, ...) a4_
#define LOG_ASSERT_COUNT_FROM_0_TO_3_(...) \
    LOG_ASSERT_EXPAND_(LOG_ASSERT_GET_ARG4_(__VA_ARGS__, 3, 2, 1, 0))
#define LOG_ASSERT_COUNT_(...) \
    LOG_ASSERT_CALL_(LOG_ASSERT_COUNT_FROM_0_TO_3_, (, __VA_ARGS__))

#define LOG_ASSERT_HAS_FMT_(...) LOG_ASSERT_COUNT_(__VA_ARGS__)
#define LOG_ASSERT_MESSAGE_HAS_FMT_(exp, fmt, ...) \
    LOG_ERROR("Assertion (" #exp ") failed: " fmt, __VA_ARGS__)
#define LOG_ASSERT_MESSAGE_NO_FMT_(exp) LOG_ERROR("Assertion (" #exp ") failed")
#define LOG_ASSERT_MESSAGE_(exp, fmt, ...)                              \
    (void)((LOG_ASSERT_HAS_FMT_(fmt) &&                                 \
            (LOG_ASSERT_MESSAGE_HAS_FMT_(exp, fmt, __VA_ARGS__), 1)) || \
           (LOG_ASSERT_MESSAGE_NO_FMT_(exp), 0))

#define LOG_ASSERT(exp, fmt, ...)                                          \
    (void)((!!(exp)) || (LOG_ASSERT_MESSAGE_(exp, fmt, __VA_ARGS__), 0) || \
           (throw std::runtime_error("Assertion (" #exp ") failed"), 0))

#else
#define LOG_ASSERT(exp, fmt, ...) ((void)0)
#endif
}  // namespace lumi
