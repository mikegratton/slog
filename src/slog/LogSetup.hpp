#pragma once
#include <memory>
#include <vector>

#include "LogRecord.hpp"
#include "LogRecordPool.hpp"
#include "LogSink.hpp"
#include "ThresholdMap.hpp"
#include "slog.hpp"
#ifndef SLOG_NO_STREAM
#include <ostream>
#endif

namespace slog {

/**
 * @brief Full control of logging
 *
 * This is only needed where you are starting/stopping the logger.
 *
 * Quick version:
 *
 * ```
 * #include <LogSetup.hpp> // Only needed in the cpp where setup occurs
 *
 * int main(int argc, char** argv) {
 *
 *    Log(WARN) << "No one will see this message.";
 *
 *    slog::start_logger(NOTE);
 *    Log(NOTE) << "This is logged using the ConsoleSink with the default setup";
 *    Log(INFO) << "This isn't logged because INFO < NOTE";
 *
 *    slog::stop_logger(); // This is not required, but will stop all messages from printing
 *    Log(WARN) << "No one will see this message.";
 *
 *    return 0;
 * }
 * ```
 */
class LogConfig;

/**
 * @brief Start a logger on the default channel with the given config.
 */
void start_logger(LogConfig const& config);

/**
 * @brief Start loggers on channels [0, configs.size()) with the
 * given configs.
 */
void start_logger(std::vector<LogConfig> const& configs);

/**
 * @brief Stop all channels, draining the queue into the sinks.
 *
 * Note: this prevents further messages from being logged.
 */
void stop_logger();

/**
 * @brief Set the log stream locale
 */
void set_locale(std::locale locale);
void set_locale_to_global();

/**
 * Configuration class for a logger channel. Set your logging threshold,
 * tags, log sink here, and record pool here. By default, the sink is a
 * NullSink that discards all messages and the pool is a shared global
 * pool that allocates when empty.
 */
class LogConfig {
   public:
    LogConfig();

    /// Set the default threshold at which to accept records
    void set_default_threshold(int thr) { threshold.set_default(thr); }

    /**
     * @brief Add a custom threshold for a tag
     *
     * Note you don't need to register all tags here, only those where you
     * want to log that tag at a different threshold than the default.
     */
    void add_tag(const char* tag, int thr) { threshold.add_tag(tag, thr); }

    /**
     * @brief Set the sink
     * The sink saves log records. There are three sinks included:
     * FileSink, ConsoleSink, and JournaldSink.
     */
    void set_sink(std::shared_ptr<LogSink> sink_) { sink = sink_; }

    /**
     * @brief Use a special record pool
     *
     * By default, all channels share a common pool. You can use special pools with
     * different-sized records if required by setting this.
     */
    void set_pool(std::shared_ptr<LogRecordPool> pool_) { pool = pool_; }

    /// Get the current sink
    std::shared_ptr<LogSink> get_sink() const { return sink; }

    /// Inspect the tag to threshold map
    ThresholdMap const& get_threshold_map() const { return threshold; }

    /// Get a shared_ptr to the log record pool
    std::shared_ptr<LogRecordPool> get_pool() const { return pool; }

   protected:
    std::shared_ptr<LogRecordPool> pool;
    std::shared_ptr<LogSink> sink;
    ThresholdMap threshold;

#ifndef SLOG_NO_STREAM
   public:
    /// Set the locale for the stream
    void set_locale(std::locale locale_)
    {
        locale = locale_;
    }

    std::locale const& get_locale() const
    {
        return locale;
    }

   protected:
    std::locale locale;
#endif
};

/// (For debugging the logger) Check that all pool records are either free or in a queue
long get_pool_missing_count();

}  // namespace slog
