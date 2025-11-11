#include "LogRecordPool.hpp" // For RecordNode
#include "slogDetail.hpp"
#include <algorithm>
#include <cstring>

namespace slog
{

CaptureBinary::CaptureBinary(RecordNode* i_node, int i_channel)
    : head_node(i_node),
      current_node(nullptr),      
      cursor(nullptr),
      buffer_end(nullptr),
      channel(i_channel)
{
    set_node(i_node);
}

CaptureBinary::~CaptureBinary()
{
    set_byte_count();
    slog::push_to_sink(head_node, channel);
}

CaptureBinary& CaptureBinary::record(void const* message, long byte_count)
{
    long count = write_some(reinterpret_cast<char const*>(message), byte_count);
    while (count < byte_count) {           
        RecordNode* extra = get_fresh_record(channel, nullptr, nullptr, -1, ~0, nullptr);
        if (nullptr == extra) {
            return *this;
        }            
        set_node(extra);
        count += write_some(reinterpret_cast<char const*>(message) + count, byte_count - count);      
    }
    return *this;
}

std::streamsize CaptureBinary::write_some(char const* source, long byte_count)
{
    byte_count = std::min(byte_count, buffer_end - cursor);
    memcpy(cursor, source, byte_count);    
    cursor += byte_count;
    return byte_count;
}

void CaptureBinary::set_node(RecordNode* i_node)
{
    if (current_node) {
        set_byte_count();
        attach(current_node, i_node);        
    }
    current_node = i_node;        
    cursor = current_node->rec.message;    
    buffer_end = cursor + current_node->rec.message_max_size;
}

void CaptureBinary::set_byte_count()
{
    current_node->rec.message_byte_count = cursor - current_node->rec.message;
}

} // namespace slog