#ifndef SLOG_NO_STREAM
#include <streambuf>
#include <ostream>
#include <cstring>
#include "slog.hpp"
#include "LogRecordPool.hpp" // For RecordNode

namespace slog {

namespace {
    // A streambuf that writes to a fixed sized array. If the message 
    // is too long, tack a null char on the end and discard the rest.
class IntrusiveBuf : public std::streambuf {
public:
    void set_node(RecordNode* node, long channel) {
        m_node = node;
        m_channel = channel;
        pubsetbuf(node->rec.message, node->rec.message_max_size);
    }
    
protected:

    RecordNode* m_node;
    long m_channel;
    
    std::streambuf* setbuf(char* buffer, std::streamsize length_) override {
        setp(buffer, buffer + length_);
        return this;
    }
    
    int overflow(int) override {
        char* last = epptr();
        if (last) {
            *(last-1) = '\0';
        }
        return traits_type().eof();
    }


    std::streamsize xsputn(char const* s, std::streamsize length) override {
        std::streamsize max_write = epptr() - pptr();
        if (length < max_write) {
            memcpy(pptr(), s, length);
            pbump(length);
        } else {
            std::streamsize write_length = max_write - 1;
            memcpy(pptr(), s, write_length);
            m_node->rec.message[m_node->rec.message_max_size-1] = '\0';            
            s += write_length;
            length -= write_length;
                        
            RecordNode* extra = get_fresh_record(m_channel, nullptr, nullptr, 0, m_node->rec.meta.severity,
                                               m_node->rec.meta.tag);                      
            attach(m_node, extra);
            set_node(extra, m_channel);
            xsputn(s, length);            
        }

        if (pptr() == epptr()) {
            overflow(0);
        }
        return length;
    }
};

// An ostream using an IntrusiveBuf. Note that in general, the null terminator
// must be manually added to the stream via terminate()
class IntrusiveStream : public std::ostream {
public:
    IntrusiveStream()
        : std::ostream(&buf) {
    }

    void set_node(RecordNode* node, long channel) {
        buf.set_node(node, channel);
        clear();
    }

    void terminate() {
        buf.sputc(0);
    }

protected:
    IntrusiveBuf buf;
};

// An ostream that just discards all input
class NullStream : public std::ostream {
public:
    NullStream() : std::ostream(&m_sb) {}
private:
    class NullBuffer : public std::streambuf {
    public:
        int overflow(int c) override { return c; }
    } m_sb;
};


std::locale s_locale;
int s_locale_version = 0;
class StreamHolder {
public:
    StreamHolder() : m_locale_version(s_locale_version) { }
    
    IntrusiveStream& stream() {
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
NullStream s_null; // Since NullStream has no state, all threads share a copy

} // end of anon namespace

void set_locale(std::locale locale)
{
    s_locale = locale;
    s_locale_version++;
}

void set_locale_to_global() { set_locale(std::locale()); }

// On destruction, terminate the cstring, then forward the node to the 
// back end.
CaptureStream::~CaptureStream() {
    if (node) {
        st_stream.stream_direct() << std::ends;          
        push_to_sink(node, channel); // Push to channel queue
    }
}

std::ostream& CaptureStream::stream() {
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


}

#endif
