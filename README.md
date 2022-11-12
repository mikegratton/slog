# Slog: A C++ Logging Library for Robotic Systems

Slog is an asynchronous stream-based logger with a static memory pool.  The "S" could stand
for any number of things. It is designed for applications where the business logic cannot
wait for disk or network IO to complete before continuing.  For many server and robotic 
programs, this is the right idea.  However, Slog may require some tuning to work appropriately
for your application.  Other general-purpose log libraries may be better if this doesn't 
appeal to you.

## Quickstart

## Design 

Slog has three design goals:

1. Do as little work on the business thread as possible 
2. Minimize the include size for `slog.hpp` and 
3. Provide clear failure modes.

For (1), Slog is asynchronous (see below). It also uses a special pre-allocated pool of log 
records to avoid allocating memory during log capture.  It agressively checks to see if a message 
will be logged at all, avoiding formatting the log string if the result would be ignored. This all 
means Slog stays out of the way of your program as much as possible.

Likewise for (2), Slog only includes `<iostream>` (and then only when streams are enabled).  It 
doesn't use any templates in the logging API, keeping the compile-time cost of including 
`slog.hpp` as small as possible. Goal (2) also precludes a header-only design, but building Slog
as part of your project is very easy and recommended.

Failure modes (3) are important for critical applications.  Memory and time resources are finite,
so careful programs should have plans for cases where these run short. Slog provides three policies
to control logging behavior when resources are tight:

1. Allocate: If the log record pool becomes exhausted, Slog will attempt to allocate more memory.
This will slow down business code, but for general use, it ensures that the logger will function
provided there's free memory. This is the default.
2. Block: If the pool is empty, block the business thread until records become available. The 
maximum time to block is configurable.  If it is exhausted, the log record is ignored. This mode 
ensures that the memory used by the logger is bounded, and places a bound on the time per 
log message. With appropriate tuning, this policy can meet needs for real time systems.
3. Discard: If the pool is empty, discard the log record. This is the most extreme, never allocating 
or blocking. This choice is intended for real time applications where delay of the business 
logic could result in loss of life or grevious harm.



## Asynchronous Logging

Slog is an *asynchronous* logger, meaning log records are written to sinks (files, sockets, etc.)
on dedicated worker threads.  "Logging" on a business thread captures the message to an internal 
buffer.  This is then pushed into a thread-safe queue.  This design minimizes blocking calls that 
are made on the business thread. The risk with asynchronous logging is that the program may terminate
before this queue is written to disk.  Slog ties into the signal system to ensure that the queue
is written out before the program exists in most cases.  Note that calling `exit()` is not one
of these cases -- this exits the program via a means that evades all signal trapping.  Slog will
drain its queue on all trappable signals (ctrl-C -- SIG_INT -- included), as well as when
`abort()` is called.

## API

Slog's API is split into two parts: a very light-weight general include for code that
needs to write logs, and a larger setup header that is used to configure the logger.


### General Use

Slog provides a set of macro-like functions in `slog.hpp` for logging. In general, this is
the only include you'll need in locations where you log.  The macros are

* `Slog(SEVERITY) << "My message"` : Log "My message" at level SEVERITY to the default channel
with no tag. This provides a `std::ostream` stream to log to.
* `Slog(SEVERITY, "tag") << "My message"` : Log "My message" at level SEVERITY to the default channel
with tag "tag".
* `Slog(SEVERITY, "tag", 2) << "My message"` : Log "My message" at level SEVERITY to channel 2
with tag "tag".
* `Flog(SEVERITY, "A good number is %d", 42)` : Log "A good number is 42" to the default channel
with no tag. This uses printf-style formatting.
* `Flogt(SEVERITY, "tag", "A good number is %d", 42)` : Log "A good number is 42" to the default channel
with tag "tag". This uses printf-style formatting.
* `Flogtc(SEVERITY, "tag", 2, "A good number is %d", 42)` : Log "A good number is 42" to channel 2
with tag "tag". This uses printf-style formatting.

All of these macros agressively check if the message will be logged given the current severity threshold
for the tag and channel.  If the message won't be logged, the code afterwards *will not be executed*.
That is, no strings are formatted, no work is done.  Moreover, depending on setup, if the message
pool is empty, these macros may silently skip logging if `SLOG_POOL_BLOCKS_WHEN_EMPTY` is not set.
This makes the runtime cost of `DBUG` messages very slight.

Slog uses a custom `std::ostream` class that avoids the inefficiencies of `std::stringstream`.  For 
the Flog family of macros, formatting is performed with `vsnprintf`.

### Setup

For your main, `LogSetup.hpp` provides the API for configuring your log.  There are three start 
calls:

1. `void start_logger(int severity_threshold)` : Start the logger with the default file sink back-end
and the simple severity threshold given for all tags. This only sets up the default channel.
2. `void start_logger(LogConfig& config)` : Start the logger using the given configuration for the 
default channel.  See below for the `LogConfig` object.
3. `void start_logger(std::vector<LogConfig>& configs)`: Start the logger using the given set 
of configs for each channel. The default channel `0` uses the front config.

The LogConfig object provides a small API for configuring the logger behavior:

* `LogConfig::set_default_threshold(int thr)` : Sets the default severity threshold for all (or no) tags.
* `LogConfig::add_tag(const char* tag, int thr)` : Sets the threshold `thr` for tag `tag`.
* `LogConfig::set_sink(std::unique_ptr<LogSink> sink)` : Takes the log sink instance given to use for the back end.
* `LogConfig::set_sink(LogSink* sink)` : Takes the log sink instance given to use for the back end. Slog takes 
ownership of the pointer and will perform cleanup when required.

In addition, you may call `stop_logger()` to cease logging, but this isn't required before program 
termination.

### Log Sinks
Slog comes with two sinks for recording messages, `FileSink` and `JournaldSink`.

#### FileSink
`FileSink` writes log messages to a file via the C `fprintf()` API. It can also optionally echo
those messages to `stdout`.  It has a handful of useful features:

1. `set_echo(bool)` : Echo (or don't) messages to the console. Defaults to true.
2. `set_max_file_size(int)` : Set the maximum file size before rolling over to a new file. Defaults to unlimited.
3. `set_file(char const* location, char const* name, char const* end="log")` : Sets the naming pattern for 
files.  `location` is the directory to write files to. It defaults to `.`.  `name` is the "stem" of the file name.
It defaults to the program name. `end` is the filename extension. It defaults to `log`.  The overall
filename takes the form `[location]/[name]_[ISO Date]_[sequence].[end]`, where `ISO Date` is the 
ISO 8601 timestamp when the program started and `sequence` is a three digit counter that increases as rollover
happens.
4. `set_formatter(Formatter format)`: Adjust the format of log messages. See below.

Formatter is an alias for `std::function<int (FILE* sink, LogRecord const& rec)>`.  This takes
the file to write to and the message to write and records it, returning the number of bytes
written. The default formatter in `LogSink.cpp` is a good example:
```
int default_format(FILE* sink, LogRecord const& rec) {    
    char severity_str[16];
    char time_str[32];
    format_severity(severity_str, rec.meta.severity);
    format_time(time_str, rec.meta.time, 3);
    return fprintf(sink, "[%s %s %s] %s", severity_str, rec.meta.tag, time_str, rec.message);    
}
```
The `LogRecord` meta contains useful metadata about the record:
```
    char tag[TAG_SIZE];      //! The tag
    char const* filename;    //! filename containing the function where this message was recorded
    char const* function;    //! name of the function where this message was recorded    
    unsigned long time;      //! ns since Unix epoch
    unsigned long thread_id; //! unique ID of the thread this message was recorded on
    int line;                //!program line number
    int severity;            //! Message importance. Lower numbers are more important    
```
As you can see, the default formatter doesn't include all of this information, but you can 
easily customize it.

#### JournaldSink
The Journald sink uses the structured logging features of Jornald to record the metadata.  These
are logged as
```
        sd_journal_send(
            "CODE_FUNC=%s", rec.meta.function,
            "CODE_FILE=%s", rec.meta.filename,
            "CODE_LINE=%d", rec.meta.line,
            "THREAD=%ld",   rec.meta.thread_id,
            "TIMESTAMP=%s", isoTime,
            "PRIORITY=%d",  rec.meta.severity/100,
            "MESSAGE=%s",   rec.message,
            NULL); 
```
allowing you to use the Jornald features to filter the logs.  For a great introduction to this,
see http://0pointer.de/blog/projects/journalctl.html.

The sink also provides optional echoing to the console. The echoed logs use the same `Formatter` 
function object as `FileSink`.

### Locale Setting

Slog uses a thread-local stream object that is initialized in an order you can't control (the old 
static intitialization fiasco of C++ lore). As such, if you change the global locale, the streams 
will not pick up on this by default.  Slog will pick up the global locate on a `start_logger()` call,
but you can force Slog to update the stream locale at any time via
`void set_locale(std::locale locale)` or `void set_locale_to_global()`.

## Building Slog

Slog is written in C++11 and has no required dependencies.  Because the configuration
of Slog is important for each application, you'll probably be happiest building it 
as part of a CMake superbuild.  To do so, add this project as a subdirectory of your
code and put
```
add_subdirectory(slog)
```
in your `CMakeLists.txt` file.  This will give you the target `slog` that you can depend 
on.

Slog has an optional dependency on libsystemd for the journal sink.  To use this, you'll 
need to install the systemd development package for your OS.  In Debian or Ubuntu, you 
can do this via
```
apt install libsystemd-dev
```
Note that this doesn't add any *runtime* dependencies to your application, you just need the 
headers for building.

## Configuring Slog

Slog has many build-time configuration options.  The easiest way to set these is to use 
`ccmake` to see the full list.  Here's a rundown:

+-------------------------+-----------------------------+----------------------------------------+
| Name                    |    Default                  |    Notes                               |
+-------------------------+-----------------------------+----------------------------------------+
| `SLOG_ALWAYS_LOG`       |   OFF                       | When the logger is stopped, dump records to the console |
| `SLOG_MAX_BLOCK_TIME_MS`|   50                        | When SLOG_EMPTY_POOL_BLOCKS is set, this is the maximum time to wait for a free record |
| `SLOG_MAX_CHANNEL`      |   1                         | Number of logger channels |
| `SLOG_MESSAGE_SIZE`     |   1024                      | Maximum message size in bytes |
| `SLOG_POOL_POLICY`      |   SLOG_EMPTY_POOL_BLOCKS    | One of SLOG_EMPTY_POOL_BLOCKS, SLOG_EMPTY_POOL_ALLOCATES, or SLOG_EMPTY_POOL_DISCARDS|
| `SLOG_POOL_SIZE`        |   1024000                   | Size of pre-allocated record pool |
+-------------------------+-----------------------------+-----------------------------------+

## Customization

### Tweaking the Format

### Writing Your Own Sink

