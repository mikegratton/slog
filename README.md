# Slog: A C++ Logging Library for Robotic Systems

Slog is an asynchronous stream-based logger with a static memory pool.  The "S" could stand
for any number of things. It is designed for applications where the business logic cannot
wait for disk or network IO to complete before continuing.  For many server and robotic 
programs, this is the right idea.  However, Slog may require some tuning to work appropriately
for your application.  Other general-purpose log libraries may be better if this doesn't 
appeal to you.

## Quickstart

## Design 

## Asynchronous Logging

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
will not pick up on this by default.  To force Slog to update the stream locale, call 
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
Note that this doesn't add any *runtime* dependencies to your application, we just need the 
headers for building.

## Configuring Slog


## Customization

### Tweaking the Format

### Writing Your Own Sink

