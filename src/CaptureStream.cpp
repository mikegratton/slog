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

protected:

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
        if (length > max_write) {
            length = max_write;
        }

        memcpy(pptr(), s, length);
        pbump(length);

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

    void setbuf(char* cstring, long maxlength) {
        buf.pubsetbuf(cstring, maxlength);
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

thread_local IntrusiveStream st_stream; // Each thread has its own stream
NullStream s_null; // Since NullStream has no state, all threads share a copy

} // end of anon namespace

// On destruction, terminate the cstring, then forward the node to the 
// back end.
CaptureStream::~CaptureStream() {
    if (node) {
        st_stream.put(0);        
        push_to_sink(node); // Push to channel queue
    }
}

std::ostream& CaptureStream::stream() {
    if (node) {
        // Set the stream to write to the message buffer
        st_stream.setbuf(node->rec.message, node->rec.message_max_size);
        return st_stream;
    } else {
        // Failure to allocate, just eat the message
        return s_null;
    }
}


}

