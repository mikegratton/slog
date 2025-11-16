#include "LogConfig.hpp"
#include <ios>
#ifndef SLOG_NO_STREAM
#include <cstring>
#include <cassert>
#include <ostream>
#include <streambuf>

#include "LogRecord.hpp" 
#include "slogDetail.hpp"

namespace slog
{

namespace
{
/// A streambuf that writes to the buffer in a node. If the message
/// is too long for that buffer, another node is allocated
class IntrusiveBuf : public std::streambuf
{
  public:

    // Get the head LogRecord*, setting the internal nodes to null.
    // Idempotent.
    LogRecord* take_node() 
    {
        LogRecord* returnValue = nullptr;
        if (m_recordHead) {
            set_node_byte_count();
            returnValue = m_recordHead;
            m_recordHead = m_node = nullptr;
            m_channel = DEFAULT_CHANNEL;
            setp(nullptr, nullptr);
        }
        return returnValue;
    }

    void set_node(LogRecord* node, int channel)
    {
        if (m_recordHead) {
            set_node_byte_count();
            assert(m_node->message_byte_count() == m_node->message_max_size());
            m_node->attach(node);
        } else {
            m_recordHead = node;
        }
        m_node = node;
        m_channel = channel;
        setp(node->message(), node->message() + node->message_max_size());
    }

    void set_node_byte_count()
    {
        m_node->message_byte_count(pptr() - pbase());
    }

    int channel() const { return m_channel; }

  protected:
    LogRecord* m_recordHead{nullptr};
    LogRecord* m_node{nullptr};
    int m_channel{DEFAULT_CHANNEL};    

    std::streamsize write_some(char const* s, std::streamsize count)
    {
        count = std::min(count, epptr() - pptr());
        memcpy(pptr(), s, count);
        pbump(count);
        return count;
    }

    std::streamsize xsputn(char const* s, std::streamsize length) override
    {
        std::streamsize count = 0;
        do {
            count += write_some(s + count, length - count);
            if (count < length) {
                this->overflow(std::streambuf::traits_type::eof());
            }
        } while (count < length);
        return count;
    }

    std::streambuf::int_type overflow(std::streambuf::int_type ch) override
    {
        if (pptr() >= epptr()) {
            LogRecord* extra = get_fresh_record(m_channel, nullptr, nullptr, -1, ~0, nullptr);
            if (!extra) {
                return std::streambuf::traits_type::eof();
            }
            set_node(extra, m_channel);
        }
        if (std::streambuf::traits_type::eq_int_type(ch, std::streambuf::traits_type::eof())) {
            return 1;
        }
        write_some(reinterpret_cast<char*>(&ch), 1);
        return ch;
    }
};

/// An ostream using an IntrusiveBuf
class IntrusiveStream : public std::ostream
{
  public:
    IntrusiveStream()
        : std::ostream(&buf)
    {
    }

    void set_node(LogRecord* node, int channel)
    {
        assert(buf.take_node() == nullptr);
        buf.set_node(node, channel);
        clear();
    }

    LogRecord* take_node() { return buf.take_node(); }

    int channel() const { return buf.channel(); }

  protected:
    IntrusiveBuf buf;
};

/// An ostream that just discards all input
class NullStream : public std::ostream
{
  public:
    NullStream()
        : std::ostream(&m_sb)
    {
    }

  private:
    class NullBuffer : public std::streambuf
    {
      public:
        
        std::streamsize xsputn(char const*, std::streamsize length) override { return length; }

        int overflow(int c) override
        {
            return std::streambuf::traits_type::eq_int_type(c, std::streambuf::traits_type::eof()) ? 1 : c;
        }
    } m_sb;
};

namespace
{
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

thread_local StreamHolder st_stream; // Each thread has its own stream
NullStream s_null;                   // Since NullStream has no state, all threads share a copy

} // namespace

void set_locale(std::locale locale)
{
    s_locale = locale;
    s_locale_version++;
}

void set_locale_to_global() { set_locale(std::locale()); }

CaptureStream::CaptureStream(LogRecord* node_, int channel_)    
{
    if (node_) {
        st_stream.stream_direct().set_node(node_, channel_);
        stream_ptr = &st_stream.stream();
    } else {
        stream_ptr = &s_null;
    }
}

// On destruction, forward the node to the backend.
CaptureStream::~CaptureStream()
{
    if (stream_ptr != &s_null) {        
        auto& stream = st_stream.stream_direct();        
        push_to_sink(stream.take_node(), stream.channel());
    }
}

} // namespace slog

#endif
