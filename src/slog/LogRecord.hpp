#pragma once
#include "LogConfig.hpp"
#include <cstdint>

namespace slog
{

/**
 * @brief Metadata associated with every log message
 */
class LogRecordMetadata {
  public:
    LogRecordMetadata();
    void reset();

    /**
     * @brief Capture the metadata.
     *
     * This assumes that filename and function are static strings in the program
     * (i.e. those produced by __FILE__ and __FUNCTION__ macros), while tag may
     * change on the caller's thread. Therefore tag is copied, but filename and
     * function just take the pointer values.
     */
    void capture(char const* filename, char const* function, int line, int severity, char const* tag);

    /// Inspect the tag. This string is TAG_SIZE long
    char const* tag() const { return m_tag; }
    
    /// Inspect the filename where the record was recorded
    char const* filename() const { return m_filename; }

    /// Inspect the function where the record was recorded
    char const* function() const { return m_function; }

    /// Get the nanoseconds since unix epoch when the record was recorded
    uint64_t time() const { return m_time; }

    /// Get a thread id for the thread where the record was recorded.
    unsigned long thread_id() const { return m_thread_id; }

    /// Get the line where the record was recorded.
    int line() const { return m_line; }

    /// Get the attached severity. Lower numbers are more important
    int severity() const { return m_severity; }

    /// Set all fields in the metdata
    void set_data(char const* filename, char const* function, int line, int severity, char const* tag, uint64_t time,
                  unsigned long threadh_id);

  private:
    //! Associated tag metadata
    char m_tag[TAG_SIZE];

    //! filename containing the function where this message was recorded
    char const* m_filename;

    //! name of the function where this message was recorded
    char const* m_function;

    //! ns since Unix epoch
    uint64_t m_time;

    //! unique ID of the thread this message was recorded on
    unsigned long m_thread_id;

    //! program line number
    int m_line;

    //! Message importance. Lower numbers are more important
    int m_severity;
};

/**
 * @brief A LogRecord is a message string combined with the associated metadata.
 *
 * These objects are managed by LogRecordPool.
 */
class LogRecord
{
  public:
    
   /// Inspect the metadata
    LogRecordMetadata const& meta() const { return m_meta; }

    /// The maximum number of bytes in message()
    uint32_t message_max_size() const { return m_message_max_size; }

    /// The current number of bytes in message()
    uint32_t message_byte_count() const { return m_message_byte_count; }

    /// Get the message. This string is not null terminated. Use message_byte_count() to 
    /// determine the end of the string.
    char const* message() const { return m_message; }

    /// If this is a jumbo message, this pointer will give the next chunk. This LogRecord
    /// will not necessarily have valid metadata
    LogRecord const* more() const { return m_more; }

    /// Mutate the metadata
    LogRecordMetadata& meta() { return m_meta; }

    /// Set the valid message byte count()
    void message_byte_count(uint32_t count) { m_message_byte_count = count; }

    /// Mutate the message
    char* message() { return m_message; }

    /// Attach another record to this one to make a jumbo record
    LogRecord* attach(LogRecord* more_node)
    {        
        m_more = more_node;
        return m_more;
    }

  private:
    friend class LogRecordPool;
    friend class LogChannel;
    friend class PoolMemory;

    /// These are only created in LogRecordPool
    LogRecord();

    /// Clean out this record
    void reset();

    //! Metadata
    LogRecordMetadata m_meta;

    //! Total allocated space for message
    uint32_t m_message_max_size;

    //! Size of message, including possible null terminator
    uint32_t m_message_byte_count;

    //! Actual log message.
    char* m_message;

    //! If non-null, message continued here (but metadata etc. of more are undefined)
    LogRecord* m_more;

    //! Intrusive pointer for linked lists
    LogRecord* m_next;
};

} // namespace slog
