#include <cstring>
#include <algorithm>
#include "slogDetail.hpp"
#include "LogRecordPool.hpp"  // For RecordNode

namespace slog {

CaptureBinary::CaptureBinary(RecordNode* i_node, long i_channel)
{
    set_node(i_node, i_channel);
}

CaptureBinary::~CaptureBinary()
{
    slog::push_to_sink(node, channel);
}

CaptureBinary& CaptureBinary::record(void const* bytes, long byte_count)
{
    long count = 0;
    do {
        count += write_some(reinterpret_cast<char const*>(bytes) + count, byte_count - count);
        if (count < byte_count) {
            RecordNode* extra =
                get_fresh_record(channel, nullptr, nullptr, 0, node->rec.meta.severity, node->rec.meta.tag);
            if (nullptr == extra) { return *this; }
            attach(node, extra);
            set_node(extra, channel);
        }
    } while (count < byte_count);
    return *this;
}

std::streamsize CaptureBinary::write_some(char const* s, std::streamsize count)
{
    count = std::min(count, end - cursor);
    memcpy(cursor, s, count);
    *currentByteCount += count;
    cursor += count;
    return count;
}

void CaptureBinary::set_node(RecordNode* i_node, long i_channel)
{
    node = i_node;
    channel = i_channel;
    cursor = node->rec.message;
    currentByteCount = &node->rec.message_byte_count;
    *currentByteCount = 0L;
    end = cursor + node->rec.message_max_size;
}

}  // namespace slog