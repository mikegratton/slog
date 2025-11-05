#ifndef SLOG_NO_STREAM
#include <cstring>
#include <ostream>
#include <streambuf>

#include "slogDetail.hpp"
#include "LogRecordPool.hpp"  // For RecordNode

namespace slog {

namespace {
/// A streambuf that writes to the buffer in a node. If the message
/// is too long for that buffer, another node is allocated
class IntrusiveBuf : public std::streambuf {
   public:
    void set_node(RecordNode* node, long channel)
    {
        m_node = node;
        m_channel = channel;
        m_cursor = node->rec.message;
        m_currentByteCount = &node->rec.message_byte_count;
        *m_currentByteCount = 0L;
        m_end = m_cursor + node->rec.message_max_size - 1;
    }

   protected:
    RecordNode* m_node;
    long* m_currentByteCount;
    long m_channel;
    char* m_cursor;
    char* m_end;

    std::streamsize write_some(char const* s, std::streamsize count)
    {
        count = std::min(count, m_end - m_cursor);
        memcpy(m_cursor, s, count);
        *m_currentByteCount += count;
        m_cursor += count;
        *m_cursor = '\0';
        return count;
    }

    std::streamsize xsputn(char const* s, std::streamsize length) override
    {
        std::streamsize count = 0;
        do {
            count += write_some(s + count, length - count);
            if (count < length) {
                RecordNode* extra =
                    get_fresh_record(m_channel, nullptr, nullptr, -1, ~0, nullptr);
                if (nullptr == extra) { return count; }
                attach(m_node, extra);
                set_node(extra, m_channel);
            }
        } while (count < length);
        return count;
    }
};

/// An ostream using an IntrusiveBuf
class IntrusiveStream : public std::ostream {
   public:
    IntrusiveStream() : std::ostream(&buf) {}

    void set_node(RecordNode* node, long channel)
    {
        buf.set_node(node, channel);
        clear();
    }

   protected:
    IntrusiveBuf buf;
};

/// An ostream that just discards all input
class NullStream : public std::ostream {
   public:
    NullStream() : std::ostream(&m_sb) {}

   private:
    class NullBuffer : public std::streambuf {
       public:
        int overflow(int c) override { return c; }
    } m_sb;
};

namespace {
/// Global locale for slog
std::locale s_locale;

/// Tracking of the current locale
int s_locale_version = 0;
} // namespace

/// A locale managing stream
class StreamHolder
{
  public:
    StreamHolder()
        : m_locale_version(s_locale_version)
    {
        m_stream.imbue(s_locale);
    }

    IntrusiveStream& stream()
    {
        if (m_locale_version < s_locale_version) {
            m_stream.imbue(s_locale);
            m_locale_version = s_locale_version;
        }
        return m_stream;
    }

    IntrusiveStream& stream_direct() { return m_stream; }

  protected:
    IntrusiveStream m_stream;
    int m_locale_version;
};

thread_local StreamHolder st_stream;  // Each thread has its own stream
NullStream s_null;                    // Since NullStream has no state, all threads share a copy

}  // namespace

void set_locale(std::locale locale)
{
    s_locale = locale;
    s_locale_version++;
}

void set_locale_to_global()
{
    set_locale(std::locale());
}

// On destruction, forward the node to the backend.
CaptureStream::~CaptureStream()
{
    if (node) {
        push_to_sink(node, channel);  // Push to channel queue
    }
}

// Obtain a stream to log to
std::ostream& CaptureStream::stream()
{
    if (node) {
        // Set the stream to write to the message buffer
        IntrusiveStream& s = st_stream.stream();
        s.set_node(node, channel);
        return s;
    } else {
        // Failure to allocate, just eat the message
        return s_null;
    }
}

}  // namespace slog

#endif
