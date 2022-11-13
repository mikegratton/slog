Slog: A C++ Logging Library for Robotic Systems
=================================================

Slog is an asynchronous stream-based logger with a memory pool.  It is designed for applications 
where the business logic cannot wait for disk or network IO to complete before continuing.  For 
many server and robotic programs, log traffic comes in bursts as sessions open, plans are formed,
and so on.  Slog queues up messages during the busy times and catches up on IO during the lulls.
For critical applications, Slog can guarantee it will not allocate memory beyond its initial pool.
For "normal" applications, Slog will allocate more memory if you are logging faster than it can 
keep up.

Features:

* Slog is configurable, but with sensible an unsurprising defaults. Write your own backend or
formatter, or use one of those provided

* Flexible memory pool policies to control allocation in critical code

* Optional journald sink for modern logging as well as a traditional file-based sink

* Slog uses a custom `ostream` that writes directly to a preallocated buffer

* Your own overloads of `operator<<` for your class just work with the logger

* Log records are never copied interally in Slog

* Slog's mutex-guarded sections are very short -- about four instructions maximum 


In fact, Slog began life as a lock-free project, but transitioned to mutexes when testing showed
no significant performance boost.  This is likely because Slog's main thread-safe queue has the 
back-end worker thread waiting for new records to arrive, and this is best handled using condition
variables.  Condition variables and mutexes allow the kernel insight into how to best schedule
Slog's activities.

## Quickstart

```cpp

/*** other_stuff.hpp ***/
#pragma once
#include "slog.hpp" // All that's needed where logging is happening

inline void do_other_stuff()
{
    Slog(DBUG) << "You can see this";
    Slog(INFO, "net") << "But not this";
}


/*** main.cpp ***/
#include "LogSetup.hpp" // Required for advanced setup

#include "other_stuff.hpp" // Your logic here...

int main()
{
    slog::LogConfig config;
    config.set_default_threshold(slog::DBUG);      // Log everything DBUG or higher by default
    config.add_tag("net", slog::NOTE);             // Only log net-tagged messages at NOTE level
    auto sink = std::make_shared<slog::FileSink>();
    sink->set_file("/tmp", "example", "slog");     // Name files "/tmp/example_DATE_SEQUENCE.slog"
    sink->set_echo(false);                         // Don't echo to the console (default true)
    sink->set_max_file_size(2 >> 30);              // Roll over big files (default no limit)
    config.set_sink(sink);
    slog::start_logger(config);
    
    do_other_stuff();

    return 0;
}
```

See the example project in `example` for a basic setup.

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
buffer.  This is then pushed into a thread-safe queue where a worker thread pops the message and
performs the (blocking) IO call.  This design minimizes blocking calls that 
are made on the business thread. The risk with asynchronous logging is that the program may terminate
before this queue is written to disk.  Slog ties into the signal system to ensure that the queue
is processed before the program exits in most cases.  These cases include normal end of program 
(`return` from main) and the signals SIGINT, SIGABRT, and SIGQUIT. 
Thus Slog will drain its queue on all trappable signals (ctrl-C -- SIG_INT -- included), as well 
as when `abort()` is called. Note that calling `exit()` from `stdlib.h` is not one of these cases -- 
this exits the program via a means that can evade the destructor of Slog's singleton.  If you 
are slogging in a context where you need to call exit (i.e. you have forked), you must call
`slog::stop_logger()` first to ensure a proper shutdown.

## API

Slog's API is split into two parts: a very light-weight general include for code that
needs to write logs (`slog.hpp`), and a larger setup header that is used to configure the logger
(`LogSetup.hpp`).

### Slog Concepts

#### Severity
Each message in Slog has an attached severity integer.  These match the traditional syslog levels of
DBUG, INFO, NOTE, WARN, ERRR, CRIT, ALRT, and EMER.  You may "add" your own levels by using 
integers between the severity levels you want.  For instance, to add a COOL level, we could set 
```cpp
int COOL = (slog::NOTE + slog::WARN)/2;
```
Note that higher numbers are treated as less severe.  In addition, negative severity is considered
"FATL" by Slog, triggering the draining of all log queues and a call to `abort()` ending the 
program. 

Slog filters messages based on a severity threshold per channel and per tag (see below).  If the 
threshold is set to e.g. NOTE, a call like
```cpp
int a = 0;
Slog(INFO) << "Hello, I am incrementing a: " << a++;
```
will not be executed.  The value of `a` will be zero at the end.  
Thresholds are set up in `LogConfig` before logging begins
and cannot be changed while logging is happening.
You can check if a message
would be logged by calling `slog::will_log(int severity, char* tag = "", int channel = 0)`.

#### Tags
Each message may have an optional tag associated.  These are size-limited strings (the default 
limit is 16 characters) that can have custom log thresholds.  Tags that don't have defined
thresholds use the default. For instance
```cpp
int a = 0;
Slog(INFO, "noisy") << "Increment a: " << a++;
Slog(INFO) << "Increment a: "<< a++;
```
could log zero, one, or two times.  

#### Channels
Slog can be configured to have multiple *channels* -- independent back-ends.  Each channel
has its own worker thread, its own sink, and its own set of severity thresholds.  You can use this
to separate events and data, for instance.  Note that a particular message can be sent to only one
channel.  Channels can have independent pools with different policies, sizes, and so on, or they
can share pools.

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
pool is empty, these macros may (a) allocate, causing a delay (b) block for a configurable 
amount of time waiting for a message to become available or (c) simply discard the log message.

Slog uses a custom `std::ostream` class that avoids the inefficiencies of `std::stringstream`.  For 
the Flog family of macros, formatting is performed with `vsnprintf`.

### Setup

For your main, `LogSetup.hpp` provides the API for configuring your log.  There are three start 
calls:

1. `void start_logger(int severity_threshold)` : Start the logger with the default file sink back-end
and the simple severity threshold given for all tags. This only sets up the default channel.
2. `void start_logger(LogConfig const& config)` : Start the logger using the given configuration for the 
default channel.  See below for the `LogConfig` object.
3. `void start_logger(std::vector<LogConfig> const& configs)`: Start the logger using the given set 
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
Slog comes with three built-in sinks for recording messages, `ConsoleSink`, `FileSink` and 
`JournaldSink`.  You may also make your own sinks by implementing the very small API.

#### ConsoleSink
`ConsoleSink` writes messages to `stdout`.  It has only one feature:
* `set_formatter(Formatter format)`: Adjust the format of log messages. See below.

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
```cpp
int default_format(FILE* sink, LogRecord const&) {    
    char severity_str[16];
    char time_str[32];
    format_severity(severity_str, rec.meta.severity);
    format_time(time_str, rec.meta.time, 3);    
    int count = fprintf(sink, "[%s %s %s] %s", severity_str, rec.meta.tag, time_str, rec.message);
    // Note handling of oversized records
    for (LogRecord const* more = rec.more; more != nullptr; more = more->more) {
        count += fprintf(sink, "%s", more->message);
    }
    return count;
}
```
Note that `rec.more` is a pointer to more message data if the message exceeds the size available in
a single record. The metadata for the `more` record is not meaningful.

The `LogRecord` meta contains useful metadata about the record:
```cpp
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
```cpp
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


### Tweaking the Format
The built-in sinks all use the `Formatter` functor defined in `LogSink.hpp` to format messages:
```cpp
using Formatter = std::function<int (FILE* sink, LogRecord const& node)>;
```
This should return the number of bytes written to the `sink`.  You can use a lambda in the setup
to customize the format to your liking. The helper functions in `LogSink.hpp`
provide date, severity, and code location format helpers, but are not required.

### Writing Your Own Sink
Log sinks derive from this abstract class defined in `LogSink.hpp`:
```cpp
class LogSink
{
public:
    virtual ~LogSink() = default;
    virtual void record(LogRecord const& record) = 0;
};
```
As you can see, you can do what you like in `record`.  The `Formatter` functor provides
a convenient way to allow users to customize the format, but you don't have to use it.

### Locale Setting

Slog uses a thread-local stream object that is initialized in an order you can't control (the old 
static intitialization fiasco of C++ lore). As such, if you change the global locale after starting
the logger, the streams will not pick up on this by default.  You can force Slog to update the 
stream locale via `void set_locale(std::locale locale)` or `void set_locale_to_global()`.

## Building Slog

Slog is written in C++11 and has no required dependencies.  You'll probably be happiest building it 
as part of a CMake superbuild.  To do so, add this project as a subdirectory of your
code and put
```cpp
add_subdirectory(slog)
```
in your `CMakeLists.txt` file.  This will give you the target `slog` that you can depend 
on.  Slog is a very small library and won't significantly impact compile times.

Slog has an optional dependency on libsystemd for the journal sink.  To use this, you'll 
need to install the systemd development package for your OS.  In Debian or Ubuntu, you 
can do this via
```
apt install libsystemd-dev
```
Note that this doesn't add any *runtime* dependencies to your application, you just need the 
headers for building.

## LogConfig

The `LogConfig` object is declared in `LogSetup.hpp` and offers control over the logging process.
You supply one LogConfig per channel you want to log (a `std::vector<LogConfig>`).  If you just
need one channel, you can pass a bare `LogConfig`.

The setup methods are:

* `set_default_threshold(int thr)`: Set the default severity threshold for logging. Default is `slog::INFO`

* `add_tag(const char* tag, int thr)`: Set a special threshold for the given tag
    
* `set_sink(std::shared_ptr<LogSink> sink_)`: Set the sink you'd like to use (FileSink, JournaldSink, etc.). 
The default sink is the FileSink.

* `set_pool(std::shared_ptr<LogRecordPool> pool_)`: Set the record pool.  The default pool is an allocating
record pool that allocates in 1 MB chunks with 1 kB records.

### LogRecordPool

The `LogRecordPool` constructor sets up the pool policies:
```cpp
LogRecordPool(LogRecordPoolPolicy i_policy, long i_pool_alloc_size, long i_message_size,
        long i_max_blocking_time_ms = 50);
```
The available policies are
* ALLOCATE: `malloc` more memory when the pool is empty
* BLOCK: block the business thread in the Slog() call until a record is available
* DISCARD: Discard the message in the Slog() call if no record is available

The other parameters are
* `i_pool_alloc_size` controls the size of each pool allocation. The default pool uses a 1 MB allocation.
* `i_message_size` controls the size of a single message. Longer messages are formed using the "jumbo" 
   pointer -- concatinating multiple records from the pool. Choosing this to be a bit longer than your 
   typical message can give you good performance. The defaulty pool uses 1 kB messages, roughly ten 
   terminal lines of text.
* `i_max_blocking_time_ms` sets the maximum blocking time in milliseconds when `i_policy` is `BLOCK`. 
   It has no impact for ALLOCATE or DISCARD policies.

The record pool is fully thread-safe, so sharing one pool `shared_ptr` between multiple channels 
works fine.

## Compile-time Configuration
Slog has two compile-time cmake configuration options:

+-------------------------+-----------------------------+----------------------------------------+
| Name                    |    Default                  |    Notes                               |
+-------------------------+-----------------------------+----------------------------------------+
| `SLOG_ALWAYS_LOG`       |   OFF                       | When the logger is stopped, dump records to the console |
| `SLOG_STREAM`           |   ON                        | Turn this off to avoid including <iostream> and Slog() macros |
+-------------------------+-----------------------------+-----------------------------------+

In addition, building with `-DSLOG_LOGGING_ENABLED=0` will suppress all logging in this translation
unit.
