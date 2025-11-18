#pragma once

namespace slog
{

// Avoid leaking details of LogRecord in slog.hpp
class LogRecord;

/**
 * @brief RecordInserter writes bytes into LogRecord nodes, handling allocating
 * subsequent LogRecords to form jumbo records as required. 
*/
class RecordInserter
{
  public:

    using difference_type = long;

    /// Construct a capture object with the given head node destined for
    /// the given channel
    RecordInserter(LogRecord* node, int channel);

    /// Push the record to the channel sink
    ~RecordInserter();

    RecordInserter(RecordInserter const&) = default;
    RecordInserter& operator=(RecordInserter const&) = default;
    RecordInserter(RecordInserter&&) noexcept;
    RecordInserter& operator=(RecordInserter&&) noexcept;

    /**
     * @brief Write bytes into the record
     * @param bytes -- Byte array
     * @param byte_count -- Byte count to write from bytes
     */
    long write(void const* bytes, long byte_count);

  private:
    /// Write bytes to the current node. This will not write beyond that node's remaining capacity
    long write_some(char const* bytes, long byte_count);

    /// Set the node. If there's a non-null head_node, this node is attached to the current node
    /// and the cursor/buffer_end are reset to point to its message region
    void set_node(LogRecord* new_node);

    /// Set the number of bytes written to current_node
    void set_byte_count();

    LogRecord* head_node;
    LogRecord* current_node;
    char* cursor;
    char const* buffer_end;
    int channel;
};

struct RecordInserterIterator {
    RecordInserter* inserter;
    using difference_type = long;
    
    RecordInserterIterator(RecordInserter* intserter_)
        : inserter(intserter_)
    {
    }

    /**
     * @brief Insert a character into the record.
     *
     * This advances the cursor. It is less efficient than calls to write()
     */
    RecordInserterIterator& operator=(char c)
    {
        inserter->write(&c, 1);
        return *this;
    }

    // No-op operators required of back insert iterators
    RecordInserterIterator& operator*() { return *this; }
    RecordInserterIterator& operator++() { return *this; }
    RecordInserterIterator& operator++(int) { return *this; }
};

} // namespace slog