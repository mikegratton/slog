#ifndef SLOG_NO_STREAM
#include <ios>
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
/** 
 * @brief A streambuf that uses RecordInserter to write to LogRecord nodes.
 */
class IntrusiveBuf : public std::streambuf
{
  public:

    void set_inserter(RecordInserter* inserter_)
    {
        assert(inserter_);
        inserter = inserter_;        
    }

    void release_inserter() { inserter = nullptr; }

  private:
    RecordInserter* inserter;
    
    std::streamsize xsputn(char const* s, std::streamsize length) override
    {
        // FIXME detect errors somehow?
        inserter->write(s, length);
        return length;        
    }

    std::streambuf::int_type overflow(std::streambuf::int_type ch) override
    {
        if (std::streambuf::traits_type::eq_int_type(ch, std::streambuf::traits_type::eof())) {
            return 1;
        }
        *inserter = ch;
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

    void set_inserter(RecordInserter* inserter_)
    {
        buf.set_inserter(inserter_);        
        clear();
    }

    void release_inserter() { buf.release_inserter(); }

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

CaptureStream::CaptureStream(LogRecord* node, int channel)
: inserter(node, channel)
{
    if (node) {
        st_stream.stream_direct().set_inserter(&inserter);
        stream_ptr = &st_stream.stream();
    } else {
        stream_ptr = &s_null;
    }
}

// All we do here is ensure there are no references to the defunct object. When
// inserter goes out of scope, it will forward the record to the sink.
CaptureStream::~CaptureStream()
{
    if (stream_ptr != &s_null) {        
        st_stream.stream_direct().release_inserter();
    }
}

} // namespace slog

#endif
