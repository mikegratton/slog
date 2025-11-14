#pragma once
#include <memory>
#include <vector>
#if SLOG_STREAM
#include <ostream>
#endif

#include "LogRecordPool.hpp"
#include "LogSink.hpp"
#include "ThresholdMap.hpp"
#include "slog.hpp"

namespace slog
{

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
void start_logger(std::vector<LogConfig> configs);

#if SLOG_STREAM
/**
 * @brief Set the log stream locale
 */
void set_locale(std::locale locale);
void set_locale_to_global();
#endif

/**
 * Configuration class for a logger channel. Set your logging threshold,
 * tags, log sink here, and record pool here. By default, the sink is a
 * NullSink that discards all messages and the pool is a shared global
 * pool that allocates when empty.
 */
class LogConfig
{
  public:
    LogConfig();

    /**
     * @brief Construct a config from a threshold and sink
     * This will use the default pool and have no special tag
     * thresholds.
     */
    LogConfig(int default_threshold, std::shared_ptr<LogSink> sink);

    /**
     * @brief Set the default threshold at which to accept records
     */
    void set_default_threshold(int thr) { threshold.set_default(thr); }

    /**
     * @brief Add a custom threshold for a tag
     *
     * Note you don't need to register all tags here, only those where you
     * want to log that tag at a different threshold than the default.
     */
    void add_tag(char const* tag, int thr) { threshold.add_tag(tag, thr); }

    /**
     * @brief Set the sink
     * The sink saves log records. There are three sinks included:
     * FileSink, ConsoleSink, and JournaldSink.
     */
    void set_sink(std::shared_ptr<LogSink> new_sink) { sink = new_sink; }

    /**
     * @brief Use a special record pool
     *
     * By default, all channels share a common pool. You can use special pools with
     * different-sized records if required by setting this.
     */
    void set_pool(std::shared_ptr<LogRecordPool> new_pool) { pool = new_pool; }

    /// Get the current sink
    std::shared_ptr<LogSink> const& get_sink() { return sink; }

    /// Inspect the tag to threshold map
    ThresholdMap const& get_threshold_map() const { return threshold; }

    /// Get a shared_ptr to the log record pool
    std::shared_ptr<LogRecordPool> const& get_pool() { return pool; }

  private:
    std::shared_ptr<LogRecordPool> pool;
    std::shared_ptr<LogSink> sink;
    ThresholdMap threshold;

#if SLOG_STREAM
  public:
    /// Set the locale for the stream
    void set_locale(std::locale new_locale) { locale = new_locale; }

    /// Get the locale stored here
    std::locale const& get_locale() const { return locale; }

  private:
    std::locale locale;
#endif
};

} // namespace slog
