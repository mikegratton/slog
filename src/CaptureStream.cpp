#include <streambuf>
#include <ostream>
#include <CaptureStream.hpp>
#include <cstring>

namespace slog {

namespace {
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


class NullStream : public std::ostream {
public:
    NullStream() : std::ostream(&m_sb) {}
private:
    class NullBuffer : public std::streambuf {
    public:
        int overflow(int c) override { return c; }
    } m_sb;
};

thread_local IntrusiveStream st_stream;
NullStream s_null;

} // end of anon namespace

CaptureStream::~CaptureStream() {
    if (rec) {
        st_stream.put(0);        
        push_to_sink(rec); // Push to queue
    }
}

std::ostream& CaptureStream::stream() {
    if (rec) {
        st_stream.setbuf(rec->message, rec->message_max_size);
        return st_stream;
    } else {
        return s_null;
    }
}


}

