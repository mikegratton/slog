#pragma once
#include "LogConfig.hpp"

namespace slog {

/**
 * @brief Metadata associated with every log message
 */
struct LogRecordMetadata {
    LogRecordMetadata();
    void reset();

    /**
     * @brief Capture the metadata.
     *
     * This assumes that filename and function are
     * static strings in the program (i.e. those produced by __FILE__ and
     * __FUNCTION__ macros), while tag may change on the caller's thread.
     * Therefore tag is copied, but filename and function just take the
     * pointer values.
     */
    void capture(char const* filename, char const* function, int line, int severity, const char* tag);

    //! Associated tag metadata
    char tag[TAG_SIZE];

    //! filename containing the function where this message was recorded
    char const* filename;

    //! name of the function where this message was recorded
    char const* function;

    //! ns since Unix epoch
    unsigned long time;

    //! unique ID of the thread this message was recorded on
    unsigned long thread_id;

    //! program line number
    int line;

    //! Message importance. Lower numbers are more important
    int severity;
};

/**
 * @brief A LogRecord is a message string combined with the associated metadata.
 *
 * These objects are managed by LogRecordPool.
 */
struct LogRecord {
   public:
    //! Metadata
    LogRecordMetadata meta;

    //! maximum size of the message string (including null terminator)
    long message_max_size;

    //! Actual log message.
    char* message;

    //! If non-null, message continued here (but metadata etc. of more are undefined)
    LogRecord const* more;

   protected:
    friend class LogRecordPool;
    LogRecord(char* message_, long max_message_size_);

    /// Clean out this record
    void reset();
};

}  // namespace slog
