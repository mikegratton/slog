#include "RecordInserter.hpp"
#include "LogConfig.hpp"
#include "LogRecord.hpp"
#include "slogDetail.hpp"
#include <cstring>

namespace slog
{

RecordInserter::RecordInserter(LogRecord* i_node, int i_channel)
    : head_node(i_node),
      current_node(nullptr),
      cursor(nullptr),
      buffer_end(nullptr),
      channel(i_channel)
{
    set_node(i_node);
}

RecordInserter::~RecordInserter()
{
    if (head_node) {
        set_byte_count();
        slog::push_to_sink(head_node, channel);
    }
}

RecordInserter::RecordInserter(RecordInserter&& other) noexcept
{
    *this = std::move(other);
}

RecordInserter& RecordInserter::operator=(RecordInserter&& other) noexcept
{
    head_node = other.head_node;
    current_node = other.current_node;
    cursor = other.cursor;
    buffer_end = other.buffer_end;
    channel = other.channel;

    // Guard against double submission
    other.head_node = nullptr;
    other.current_node = nullptr;
    other.cursor = nullptr;
    other.buffer_end = nullptr;
    channel = DEFAULT_CHANNEL;
    return *this;
}

RecordInserter& RecordInserter::write(void const* bytes, long byte_count)
{
    long count = write_some(reinterpret_cast<char const*>(bytes), byte_count);
    while (count < byte_count) {
        LogRecord* extra = get_fresh_record(channel, nullptr, nullptr, -1, ~0, nullptr);
        if (nullptr == extra) {
            return *this;
        }
        set_node(extra);
        count += write_some(reinterpret_cast<char const*>(bytes) + count, byte_count - count);
    }
    return *this;
}

RecordInserter& RecordInserter::operator=(char c) { return write(&c, 1); }

long RecordInserter::write_some(char const* source, long byte_count)
{
    byte_count = std::min(byte_count, buffer_end - cursor);
    memcpy(cursor, source, byte_count);
    cursor += byte_count;
    return byte_count;
}

void RecordInserter::set_node(LogRecord* i_node)
{
    if (current_node) {
        set_byte_count();
        current_node->attach(i_node);
    }
    current_node = i_node;
    cursor = current_node->message();
    buffer_end = cursor + current_node->capacity();
}

void RecordInserter::set_byte_count() { current_node->size(cursor - current_node->message()); }

} // namespace slog