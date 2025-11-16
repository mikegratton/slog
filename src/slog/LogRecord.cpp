#include "LogRecord.hpp"
#include "LogConfig.hpp"

#include <chrono>
#include <cstring>
#include <limits>
#include <thread>

namespace slog
{

constexpr int NO_LINE = -1;

LogRecordMetadata::LogRecordMetadata() { reset(); }

void LogRecordMetadata::reset()
{
    m_filename = "";
    m_function = "";
    m_line = NO_LINE;
    m_severity = std::numeric_limits<int>::max();    
    memset(m_tag, 0, TAG_SIZE);
}

void LogRecordMetadata::capture(char const* filename_, char const* function_, int line_, int severity_,
                                char const* tag_)
{    
    m_filename = filename_;
    m_function = function_;
    m_line = line_;
    m_severity = severity_;
    if (m_line >= 0) {
        // Because we might be attaching a record to another record to form a
        // "jumbo" record, sometimes the metadata isn't meaningful. We avoid
        // system calls in these cases.
        m_time = std::chrono::system_clock::now().time_since_epoch().count();
        m_thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    }
    if (tag_) {
        strncpy(m_tag, tag_, TAG_SIZE - 1);
    }
}

void LogRecordMetadata::set_data(char const* filename, char const* function, int line, int severity, char const* tag,
                                 uint64_t time, unsigned long threadh_id)
{
    m_filename = filename;
    m_function = function;
    m_line = line;
    m_severity = severity;
    m_time = time;
    m_thread_id = threadh_id;
    if (tag) {
        strncpy(m_tag, tag, TAG_SIZE - 1);
    }
}

LogRecord::LogRecord()
: m_message_max_size(0)
, m_message_byte_count(0)
, m_message(nullptr)
, m_more(nullptr)
, m_next(nullptr)
{
    m_meta.reset();
}

void LogRecord::reset()
{
    m_meta.reset();    
    m_message_byte_count = 0L;
    m_message[0] = '\0';
    m_more = nullptr;
    m_next = nullptr;
}

} // namespace slog
