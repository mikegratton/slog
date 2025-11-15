Slog: A C++ Logging Library for Robotic Systems
=================================================

* [GitHub Repository](https://github.com/mikegratton/slog)
* [Documentation](https://mikegratton.github.io/slog/)


Slog is an asynchronous stream-based logger with a memory pool.  It is designed
for applications where the business logic cannot wait for disk or network IO to
complete recording a log record before continuing.  For many server and robotic
programs, log traffic comes in bursts as sessions open, plans are formed, and so
on.  Slog queues up messages during the busy times and catches up on IO during
the lulls. For critical applications, Slog can guarantee it will not allocate
memory beyond its initial pool. For "normal" applications, Slog will allocate
more memory if you are logging faster than it can keep up.

## Features:

* Slog is configurable, but with sensible an unsurprising defaults 

* Slog has minimal impact on compile time. Unlike header-only log libraries, the
  main header for Slog is very light weight.

* Send log messages to journald, syslog, files, or just the console

* Tag messages with string data. Control the logging threshold per tag.

* Run multiple parallel loggers for logging different types of data (e.g. text
  logs versus binary logs)

* Write your own backend or message formatter using a simple API

* Fully asynchronous design avoids performing I/O work on the caller's thread.
  Log records are never copied.

* Flexible memory pool policies to control allocation in critical code.

## Quickstart

Using slog is as simple as
```cpp
/*** other_stuff.hpp ***/
#pragma once
#include "slog/slog.hpp" // All that's needed where logging is happening

inline void do_other_stuff()
{
    Slog(DBUG) << "You can see this";
    Slog(INFO, "net") << "But not this";
}
```
In the main.cpp, we can put some additional setup code
```cpp
/*** main.cpp ***/
#include "slog/LogSetup.hpp" // Required for advanced setup
#include "other_stuff.hpp" // Your logic here...

int main()
{
    slog::LogConfig config;
    config.set_default_threshold(slog::DBUG);      // Log everything DBUG or higher by default
    config.add_tag("net", slog::NOTE);             // Only log net-tagged messages at NOTE level
    auto sink = std::make_shared<slog::FileSink>();
    sink->set_file("/tmp", "example", "slog");     // Name files "/tmp/example_YYYYMMDDTHHMMSS_SEQUENCE.slog"
    sink->set_echo(false);                         // Don't echo to the console (default is true)
    sink->set_max_file_size(2 >> 30);              // Roll over big files (default is no rollover)
    config.set_sink(sink);
    slog::start_logger(config);
    
    do_other_stuff();

    return 0;
}
```
The `LogConfig` object contains the configuration for the logger. See the
example project in `example` for a basic setup.

## Design 

Slog has three design goals:

1. Do as little work on the business thread as possible 
2. Minimize the include size for `slog.hpp` and 
3. Provide clear failure modes.

For (1), Slog is asynchronous (see below). It also uses a special preallocated
pool of log records to avoid allocating memory during log capture.  It
aggressively checks to see if a message will be logged at all, avoiding
formatting the log string if the result would be ignored. This all means Slog
stays out of the way of your program as much as possible.

Likewise for (2), Slog only includes `<iostream>` (and then only when streams
are enabled).  It doesn't use any templates in the logging API, keeping the
compile-time cost of including `slog.hpp` as small as possible. Goal (2) also
precludes a header-only design, but building Slog as part of your project is
very easy.

Failure modes (3) are important for critical applications.  Memory and time
resources are finite, so careful programs should have plans for cases where
these run short. Slog provides three policies to control logging behavior when
resources are tight:

1. Allocate: If the log record pool becomes exhausted, Slog will attempt to
allocate more memory. This will slow down business code, but for general use, it
ensures that the logger will function provided there's free memory. This is the
default.
2. Block: If the pool is empty, block the business thread until records become
available. The maximum time to block is configurable.  If it is exhausted, the
log record is ignored. This mode ensures that the memory used by the logger is
bounded, and places a bound on the time per log message. With appropriate
tuning, this policy can meet needs for real time systems.
3. Discard: If the pool is empty, discard the log record. This is the most
extreme, never allocating or blocking. This choice is intended for real time
applications where delay of the business logic could result in loss of life or
grievous harm.


## Asynchronous Logging

Slog is an *asynchronous* logger, meaning log records are written to sinks
(files, sockets, etc.) on dedicated worker threads.  "Logging" on a business
thread captures the message to an internal buffer.  This is then pushed into a
thread-safe queue where a worker thread pops the message and performs the
(blocking) IO call.  This design minimizes blocking calls that are made on the
business thread. The risk with asynchronous logging is that the program may
terminate before this queue is written to disk.  Slog ties into the signal and
exit systems to ensure that the queue is processed before at exit in most cases.
These cases include normal end of program (`return` from main), calls to
`exit()` and the signals SIGINT, SIGABRT, and SIGTERM. Exiting using
`quick_exit()` can still result in lost messages, however.

### Signal Handling
Slog will only install a handler for SIGIN, SIGABRT, or SIGTERM if it discovers
the default handler in place. If you wish to ignore a signal, register SIG_IGN
before calling `start_logger()`.  If you have your own handlers for these
signals, you must call `stop_logger()` from that handler. Note that
`stop_logger()` is async signal safe.

## API

Slog's API is split into two parts: a very lightweight general include for code
that needs to write logs (`slog.hpp`), and a larger setup header that is used to
configure the logger (`LogSetup.hpp`).

### Slog Concepts

#### Severity
Each message in Slog has an attached severity integer.  These match the
traditional syslog levels of DBUG, INFO, NOTE, WARN, ERRR, CRIT, ALRT, and EMER.
You may add your own levels by using integers between the severity levels you
want.  For instance, to add a COOL level, we could set 
```cpp
int COOL = (slog::NOTE + slog::WARN)/2;
```
Note that higher numbers are treated as less severe.  In addition, negative
severity is considered "FATL" by Slog. FATL messages trigger calling `abort()`
and the abort signal handler draining of all log queues.

Slog filters messages based on a severity threshold per channel and per tag (see
below).  For example, if the threshold is set to NOTE, the code
```cpp
int a = 0;
Slog(INFO) << "Hello, I am incrementing a: " << a++;
```
will not log. Moreover, the line right of `Slog()` will not be executed.  (In
the example, the value of `a` will be zero at the end.) Thresholds are set up in
`LogConfig` before logging begins and cannot be changed while logging is
happening. You can check if a message would be logged by calling
`slog::will_log(severity, tag, channel)`.

#### Tags
Each message may have an optional tag associated.  These are size-limited
strings (the default limit is 15 characters) that can have custom log
thresholds.  Tags that don't have defined thresholds use the default. For
instance, keeping the same default threshold of NOTE as before,
```cpp
int a = 0;
Slog(INFO, "noisy") << "Increment a: " << a++;
Slog(INFO) << "Increment a: "<< a++;
```
would log at most once, if we set the threshold for tag "noisy" to INFO or
lower.

#### Channels
Slog can be configured to have multiple *channels*. A channel corresponds to an
independent back-end, with its own worker thread, its own sink, and its own set
of severity thresholds.  You can use this to separate events from data, for
instance.  Note that a particular message can be sent to only one channel.
Channels may have independent memory pools with different policies and sizes or
they can share pools with other channels. Slog represents a channel by an
integer, with the default channel being channel 0.

### General Use

Slog provides a set of function-like macros in `slog.hpp` for logging. In
general, this is the only include you'll need in locations where you log.  The
macros are

* `Slog(SEVERITY) << "My message"` : Log "My message" at level SEVERITY to the
default channel with no tag. This provides a `std::ostream` stream to log to.
* `Slog(SEVERITY, "tag") << "My message"` : Log "My message" at level SEVERITY
to the default channel with tag "tag".
* `Slog(SEVERITY, "tag", 2) << "My message"` : Log "My message" at level
SEVERITY to channel 2 with tag "tag".
* `Flog(SEVERITY, "A good number is %d", 42)` : Log "A good number is 42" to the
default channel with no tag. This uses printf-style formatting.
* `Flogt(SEVERITY, "tag", "A good number is %d", 42)` : Log "A good number is
42" to the default channel with tag "tag". This uses printf-style formatting.
* `Flogtc(SEVERITY, "tag", 2, "A good number is %d", 42)` : Log "A good number
is 42" to channel 2 with tag "tag". This uses printf-style formatting.
* `Blog(SEVERITY, "tag", 2).record(my_bytes, my_byte_count).record(more_bytes,
count2)` : Capture a binary log message in two parts with tag "tag" to channel
two.

All of these macros first check if the message will be logged given the current
severity threshold for the tag and channel.  If the message won't be logged, the
code afterwards *will not be executed*. That is, no strings are formatted, no
work is done.  Moreover, depending on the pool policy, if the message pool is
empty, these macros may (a) allocate, causing a delay (b) block for a
configurable amount of time waiting for a free record to become available or (c)
simply discard the log message. The default policy is to allocate.

Slog uses a custom `std::ostream` class that avoids some of the inefficiencies
of `std::stringstream`.  For the Flog family of macros, formatting is performed
with `vsnprintf`.

For binary logging, it is usually a good idea to define your own macros of the
form
```cpp
#define LogPoly Blog(NOTE, "poly", 2)
```
so you can simply log with
```cpp
Polygon myPolygon = ...;
auto buffer = serialize(myPolygon);
LogPoly.record(buffer, buffer.size()); 
```
This enforces the correct channel and tag so the record can be deserialized
later.

### Setup

For your main, `LogSetup.hpp` provides the API for configuring your log.  The
`start_logger()` call has three forms:

1. `void start_logger(int severity_threshold)` : Start the logger with the
default console sink back-end and the simple severity threshold given for all
tags. This only sets up the default channel.
2. `void start_logger(LogConfig const& config)` : Start the logger using the
given configuration for the default channel.  See below for the `LogConfig`
object.
3. `void start_logger(std::vector<LogConfig> const& configs)`: Start the logger
using the given set of configs for each channel. The default channel `0` uses
the first config.

The LogConfig object allows for configuring the logger behavior. It has four
main methods:

* `set_default_threshold(int thr)` : Sets the default severity threshold for all
  (or no) tags.
* `add_tag(const char* tag, int thr)` : Sets the threshold `thr` for tag `tag`.
* `set_sink(std::shared_ptr<LogSink> sink)` : Sets the sink where records will
  be written.
* `set_pool(std::shared_ptr<LogRecordPool> pool_)` : Configures the logger to
  use a custom log record pool.

In addition, you may call `stop_logger()` to cease logging, but this isn't
required before program termination.

### Log Sinks
Slog comes with three built-in sinks for recording messages, `ConsoleSink`,
`FileSink`,  
`JournaldSink`, and `SyslogSink`.  If none of these work for you, you may also
make your own sinks by implementing the API (one function call).

#### ConsoleSink
`ConsoleSink` writes messages to `stdout`.  It has only one feature:
* `set_formatter(Formatter format)`: Adjust the format of log messages. See
  below about Formatters.

#### FileSink
`FileSink` writes log messages to a file. It can also optionally echo those
messages to `stdout`.  It has a handful of useful features:

1. `set_echo(bool)` : Echo (or don't) messages to the console. Defaults to true.
2. `set_max_file_size(int)` : Set the maximum file size before rolling over to a
   new file. Defaults to unlimited.
3. `set_file(char const* location, char const* name, char const* end="log")` :
Sets the naming pattern for files.  `location` is the directory to write files
to. It defaults to `.`.  `name` is the "stem" of the file name. It defaults to
the program name. `end` is the filename extension. It defaults to `log`.  The
overall filename takes the form `[location]/[name]_[ISO Date]_[sequence].[end]`,
where `ISO Date` is the ISO 8601 timestamp when the program started and
`sequence` is a three digit counter that increases as rollover happens.
4. `set_formatter(Formatter format)`: Adjust the format of log messages. See
   below.
5. `set_file_header_format(LogFileFurniture formatter)`: Register a function to
   add contents to the begining of each log file. See below.
6. `set_file_footer_format(LogFileFurniture formatter)`: Register a fuction to
   be called when a log file is closed.

Formatter is an alias for `std::function<int (FILE* sink, LogRecord const&
rec)>`.  This takes the file to write to and the message to write and records
it, returning the number of bytes written. 

The `LogFileFurniture` type is 
```cpp
using LogFileFurniture = std::function<int(FILE* sink, int sequence, unsigned long time)>;
```
Each time a file is opened or closed, `formatter` is called with the file, the
sequence number, and the time (in nanoseconds since the Unix epoch).

#### JournaldSink
The Journald sink uses the structured logging features of Journald to record the
metadata.  These are logged (conceptually) as
```cpp
        sd_journal_send(
            "CODE_FUNC=%s", rec.meta().function(),
            "CODE_FILE=%s", rec.meta().filename(),
            "CODE_LINE=%d", rec.meta().line(),
            "THREAD=%ld",   rec.meta().thread_id(),
            "TIMESTAMP=%s", isoTime,
            "PRIORITY=%d",  rec.meta().severity()/100,
            "MESSAGE=%s",   rec.message(),
            NULL); 
```
allowing you to use the Journald features to filter the logs.  For a great
introduction to this, see http://0pointer.de/blog/projects/journalctl.html.  The
actual implementation uses an internal buffer to handle "jumbo" records and the
Journald LineMax value.

The sink also provides optional echoing to the console. The echoed logs use the
same `Formatter` function object as `FileSink`.

#### SyslogSink
The syslog sink logs in a variety of syslog formats to unix or internet sockets.
In its default form, it use UDP to send messages to `/dev/log`, the traditional
syslog unix socket. However, if constructed as
```cpp
    auto sink = std::make_shared<SyslogSink>("host.someplace.com:514");
```
it will send messages in RFC5424 format over the internet. TCP logging using RFC
6587 is also supported. Like the other sinks, it uses the `Formatter` function
object for formatting messages and supports echoing log messages to the console.

Difficulties in connecting to the logging socket are not reported by default.
You must compile your program with `-DSLOG_PRINT_SYSLOG_ERROR` to enable
debugging messages.

The API
* `set_formatter(Formatter format)` : Change the formatter
* `set_echo(bool doit = true)` : Turn echoing to the console on/off (default is
  on)
* `set_application_name(char const* application_name)` : Change the syslog
  application name (default is the program name)
* `set_facility(int facility)` : Change the syslog facility (default is 1)
* `set_rfc3164_protocol(bool doit)` : Use the old syslog format (default is RFC
  5424)
* `set_max_connection_attempts(int)` : Maximum number of reconnection attempts
  per record. If we can't connect in this number of attempts, the record is
  dropped.
* `set_max_connection_wait(wait)` : Time to wait to connect per attempt. The
  maximum possible delay before dropping a message will be
  (max_attempts)*(max_wait).


#### BinarySink
If using Slog to log binary data, the `BinarySink`, derived from `FileSink`
simple binary output file format. This sink writes log messages in the form
```
<leader> <record>
```
where the leader is produced by the record formatter for the `FileSink`. The
default `BinarySink` formatter writes 8 bytes of metadata before each record:  
```
4B    4B  
size  tag
```
The tag here is the first four characters of the tag, and will not be null
terminated in general. (If the tag is shorter than four bytes, this will be
padded with zeros.)

Slog also provides the `long_binary_format()` which records 32 bytes of metadata
```
4B    4B         8B         16B
size  severity   timestamp  tag
```
where "size" is a four byte unsigned int giving the size of the record (this
does not inlcude the size of the leader), "severity" is the four byte signed
severity code, "timestamp" is an 8 byte nanosecond count since
1970-01-01T00:00:00Z, and "tag" is a fifteen byte text string followed by one
null byte (for 16 total bytes). 

The "record" is simply the bytes passed to `Blog()`. Use the header and footer
formatters to add identifying information for the file.

Slog provides a default binary file header furniture function. This adds header
information to the start of each binary file to aid in identification and
parsing. The format is
```
4B    2B   2B      
SLOG  BOM  sequence
```
where "SLOG" is literally these four characters. The BOM is a byte order mark,
0xfeff. The sequence number identifies which file in the rotating file sequence
this is (counting up from zero). You may provide your own header via
`set_file_header_format()`.


### Tweaking the Format
The built-in sinks all use the `Formatter` functor defined in `LogSink.hpp` to
format messages:
```cpp
using Formatter = std::function<int (FILE* sink, LogRecord const& node)>;
```
This should return the number of bytes written to the `sink`.  You can use a
lambda in the setup to customize the format to your liking. Functions in
`LogSink.hpp` provide date, severity, and code location format helpers that
should make creating your own formatter easy.

### Writing Your Own Sink
Log sinks derive from this abstract class defined in `LogSink.hpp`:
```cpp
class LogSink
{
public:
    virtual ~LogSink() = default;
    virtual void record(LogRecord const& record) = 0;
    virtual void finalize() { }
};
```
As you can see, you can do what you like in `record`.  The `Formatter` functor
provides a convenient way to allow users to customize the format, but you don't
have to use it.

An example formatter is
```cpp
int my_format(FILE* sink, LogRecord const& rec) {        
    char time_str[32];    
    format_time(time_str, rec.meta().time(), 3, true);
    int count = fprintf(sink, "[%s] ", time_str);
    // Note records aren't null terminated, but the byte count is recorded
    count += fwrite(rec.message(), sizeof(char), rec.message_byte_count(), sink);
    // Note handling of "jumbo" records
    for (LogRecord const* more = rec.more(); more != nullptr; more = more->more()) {
        count += fwrite(more->message(), sizeof(char), rec.message_byte_count(), sink);
    }
    return count;
}
```
Note that `rec.more()` is a pointer to more message data if the message exceeds
the size available in a single record. The metadata for the `more` record is not
meaningful.

The `LogRecord` contains
```cpp
    LogRecordMetadata const& meta();  //! Metadata about the record (see below)
    uint32_t message_byte_count();    //! The number of bytes in message()
    char const* message();            //! The message. Note: this is not null-terminated.
    LogRecord const* more();          //! When a log record is larger than one message can hold,
                                      //! multiple records will be joined into a "jumbo" record.
                                      //! more() will be null if there are no more bytes.
```
The example above demonstrates handling the non-terminated `message()` and the
`more()` parts of a record. The `LogRecordMetadata` contains the following:
```cpp
    char const* tag();         //! The tag (null terminated)
    char const* filename();    //! filename containing the function where this message was recorded
    char const* function();    //! name of the function where this message was recorded    
    uint64_t time();           //! ns since Unix epoch
    unsigned long thread_id(); //! unique ID of the thread this message was recorded on
    int line();                //! program line number
    int severity();            //! Message importance. Lower numbers are more important    
```
As you can see, the default formatter doesn't include all of this information,
but you can easily customize it.


### Locale Setting

You may set a custom locale for a stream in Slog via the `set_locale()` method
of `LogConfig`.

Slog uses static thread-local stream objects that are initialized in an
undefined order (aka the "static initialization fiasco" of C++). As such, if you
change the global locale after starting the logger, the streams will not pick up
on this by default.  You can force Slog to update all of the stream locales via
`void set_locale(std::locale locale)` or `void set_locale_to_global()`.

## Building Slog

Slog is written in C++11 and has no required dependencies.  You'll probably be
happiest building it as part of a CMake super-build.  To do so, add this project
as a subdirectory of your code and put
```cpp
add_subdirectory(slog)
```
in your `CMakeLists.txt` file.  This will give you the target `slog` that you
can depend on.  Slog is a very small library and won't significantly impact
compile times.

Slog has an optional dependency on libsystemd for the journal sink.  To use
this, you'll need to install the systemd development package for your OS.  In
Debian or Ubuntu, you can do this via
```
apt install libsystemd-dev
```
Note that this doesn't add any *runtime* dependencies to your application, you
just need the headers for building.

## LogConfig

The `LogConfig` object is declared in `LogSetup.hpp` and offers control over the
logging process. You supply one LogConfig per channel you want to log (a
`std::vector<LogConfig>`).  If you just need one channel, you can pass a bare
`LogConfig`.

The setup methods are:

* `set_default_threshold(int thr)`: Set the default severity threshold for
  logging. Default is `slog::INFO`

* `add_tag(const char* tag, int thr)`: Set a special threshold for the given tag
    
* `set_sink(std::shared_ptr<LogSink> sink_)`: Set the sink you'd like to use
(FileSink, JournaldSink, etc.). The default sink is the FileSink.

* `set_pool(std::shared_ptr<LogRecordPool> pool_)`: Set the record pool.  The
default pool is an allocating record pool that allocates memory according to the
cmake variables below.

### LogRecordPool

The `LogRecordPool` constructor sets up the pool policies:
```cpp
LogRecordPool(LogRecordPoolPolicy i_policy, long i_pool_alloc_size, long i_message_size,
        long i_max_blocking_time_ms = 50);
```
The available policies are
* ALLOCATE: `malloc` more memory when the pool is empty
* BLOCK: block the business thread in the Slog() call until a record is
  available
* DISCARD: Discard the message in the Slog() call if no record is available

The other parameters are
* `i_pool_alloc_size` controls the size of each pool allocation. The default
  pool uses a 1 MB allocation. The pool size is rounded up to contain at least
  16 records.
* `i_message_size` controls the size of a single message. Longer messages are
   formed using the "jumbo" pointer -- concatenating multiple records from the
   pool. Choosing this to be a bit longer than your typical message can give you
   good performance. The default pool uses 512 B messages, roughly five terminal
   lines of text.  Message sizes are bounded below by 64 B.
* `i_max_blocking_time_ms` sets the maximum blocking time in milliseconds when
   `i_policy` is `BLOCK`. It has no impact for ALLOCATE or DISCARD policies.

The record pool is fully thread-safe, so sharing one pool `shared_ptr` between
multiple channels works fine.

## Compile-time Configuration
Slog has four compile-time cmake configuration options:

| Name                                |   Default  |    Notes                                                      |
|-------------------------------------|------------|---------------------------------------------------------------|
| `SLOG_LOG_TO_CONSOLE_WHEN_STOPPED`  |  OFF       | When the logger is stopped, dump records to the console       |
| `SLOG_STREAM`                       |  ON        | Turn this off to avoid including <iostream> and Slog() macros |
| `SLOG_DEFAULT_RECORD_SIZE`          |  512       | Default size of records                                       |
| `SLOG_DEFAULT_POOL_RECORD_COUNT`    |  256       | Default number of records in the pool                         |
| `SLOG_BUILD_TEST`                   |  OFF       | Build unit tests                                              |
| `SLOG_BUILD_EXAMPLE`                |  OFF       | Build example programs                                        |
| `SLOG_BUILD_BENCHMARK`              |  OFF       | Build benchmark program                                       |

Note the total memory allocation will be `SLOG_DEFAULT_POOL_RECORD_COUNT *
SLOG_DEFAULT_RECORD_SIZE`.

In addition, building with `-DSLOG_LOGGING_ENABLED=0` will suppress all logging
in a translation unit.

# Version History

* *1.0.0* Initial release.
* *1.1.0* Alter main include path to be `slog/slog.hpp`. Fix bug with "jumbo"
  messages being truncated.
* *1.2.0* 
    * Default sink changed to `ConsoleSink`. 
    * Made pool sizes configurable in cmake
    * Added a syslog sink that can use unix, UDP/IP, or TCP/IP sockets
    * Fixed signal handling bug when handlers installed before slog's handlers
* *1.3.0*
    * Add `BinarySink` and `Blog()` macro for binary logging
    * Moved details out of `slog.hpp` so that this header summarizes the basics
    * Tests, examples, and benchmark programs are now not built by default
* *1.3.1*
    * Fix issues when building Slog without journald support
* *2.0.0*
    * *Breaking change:* LogRecord and LogRecordMetadata fields are now accessed
      by methods. Fields like `message` are now access via `message()`
    * *Breaking change:* LogRecord `message` is no longer null terminated. You
      must use the `m_message_byte_count` to determine the end of the message.
      (This provides more uniformity in the handling of binary and text
      messages.)
    * Improved signal handlers to be async signal safe. 
    * `exit()` now causes Slog to flush its queue.
    * Add header/footer options for `FileSink` and `BinarySink` to add fixed
      file contents when a file is opened or closed.
    * `FileSink` and `BinarySink` will now attempt to create missing directories
      if required. This is done recursively (i.e. in the manner of `mkdir -p`).
    * Bugs related to binary messages larger than one record size have been
      fixed.
    * JournaldSink now respects the LineMax (maximum journal record size)
    * SyslogSink now attempts to reconnect a fixed number of times if the socket
      connection fails.    
    * The LogSink interface has a new optional method: `finalize()`. This method
      is called by Slog when stopping a sink. The default implementation does
      nothing.    
    * LogRecordPool has been refactored to use a more conventional memory
      allocation scheme.
    * The unit test suite has been expanded and automated.
    