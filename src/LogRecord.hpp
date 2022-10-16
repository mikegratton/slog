#pragma once
#include "LogConfig.hpp"


namespace slog {

/**
 * Metadata associated with every log message
 */
struct LogRecordMetadata {

    LogRecordMetadata();
    void reset();
    
    /**
     * Capture the metadata. This assumes that filename and function are
     * static strings in the program (i.e. those produced by __FILE__ and 
     * __FUNCTION__ macros), while tag may change on the caller's thread.
     * Therefore tag is copied, but filename and function just take the 
     * pointer values.
     */
    void capture(char const* filename, char const* function, int line,
                       int severity, const char* tag);
        
    char tag[TAG_SIZE];      //! Associated tag metadata
    char const* filename;    //! filename containing the function where this message was recorded
    char const* function;    //! name of the function where this message was recorded    
    unsigned long time;      //! ns since Unix epoch
    unsigned long thread_id; //! unique ID of the thread this message was recorded on
    int line;                //!program line number
    int severity;            //! Message importance. Lower numbers are more important    

};

class MutexLogRecordPool;
class LfLogRecordPool;

/**
 * A LogRecord is a message string combined with the associated metadata.
 * These objects are managed by LogRecordPool.
 */
struct LogRecord {
protected:
    friend class MutexLogRecordPool;
    friend class LfLogRecordPool;
    LogRecord(char* message_, long max_message_size_);    
public:
    void reset(); //! Clean out this record

    LogRecordMetadata meta; //! Metadata
    long message_max_size;  //! maximum size of the message string (including null terminator)
    char* message;          //! Actual log message. 
};


}
