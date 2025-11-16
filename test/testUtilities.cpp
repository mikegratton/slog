#include "testUtilities.hpp"
#include "LogConfig.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/wait.h>
#include <unistd.h>

namespace slog
{

void rmdir(char const* dirname) { ::rmdir(dirname); }

int severity_from_string(char const* text)
{
    if (strncmp(text, "DBUG", 4) == 0) {
        return slog::DBUG;
    }
    if (strncmp(text, "INFO", 4) == 0) {
        return slog::INFO;
    }
    if (strncmp(text, "NOTE", 4) == 0) {
        return slog::NOTE;
    }
    if (strncmp(text, "WARN", 4) == 0) {
        return slog::WARN;
    }
    if (strncmp(text, "ERRR", 4) == 0) {
        return slog::ERRR;
    }
    if (strncmp(text, "CRIT", 4) == 0) {
        return slog::CRIT;
    }
    if (strncmp(text, "ALRT", 4) == 0) {
        return slog::ALRT;
    }
    if (strncmp(text, "EMER", 4) == 0) {
        return slog::EMER;
    }
    return slog::FATL;
}

bool parse_time(uint64_t& o_nanoseconds, char const* time_str)
{
    // 2020-01-02 12:13:14.567Z
    constexpr uint64_t NANOS_PER_SEC = 1000000000ULL;
    constexpr uint64_t NANOS_PER_MILLI = 1000000ULL;

    struct tm time_count {
    };
    time_count.tm_year = atoi(time_str) - 1900;
    time_str += 5;
    time_count.tm_mon = atoi(time_str) - 1;
    time_str += 3;
    time_count.tm_mday = atoi(time_str);
    time_str += 3;
    time_count.tm_hour = atoi(time_str);
    time_str += 3;
    time_count.tm_min = atoi(time_str);
    time_str += 3;
    double seconds = atof(time_str);
    time_count.tm_sec = (int)std::trunc(seconds);
    uint64_t millis = 1000 * (seconds - std::trunc(seconds));
    time_t time = timegm(&time_count);
    o_nanoseconds = time * NANOS_PER_SEC + millis * NANOS_PER_MILLI;
    return true;
}

void parse_log_record(TestLogRecord& o_record, char const* recordString)
{
    int severity = 0;
    char tag[TAG_SIZE]{};
    uint64_t time = 0;

    assert(recordString[0] == '[');
    recordString++;
    severity = severity_from_string(recordString);
    recordString += 5;
    if (recordString[23] != ']') { // There's a tag
        char* cursor = tag;
        while (*recordString != ' ' && *recordString) {
            *cursor++ = *recordString++;
        }
        *cursor++ = '\0';
        recordString++;
    }
    parse_time(time, recordString);
    recordString += 23;
    assert(recordString[0] == ']');
    o_record.meta.set_data("", "", 1, severity, tag, time, -1L);
    strncpy(o_record.message, recordString + 2, sizeof(o_record.message));
    o_record.message_byte_count = strlen(o_record.message) - 1; // exclude newline
}

FILE* popen2(char const* command, char const* mode, pid_t* pid)
{
    int constexpr READ = 0;
    int constexpr WRITE = 1;
    int pfp[2];                /* the pipe and the process */
    FILE* fp;                  /* fdopen makes a fd a stream   */
    int parent_end, child_end; /* of pipe          */

    if (*mode == 'r') { /* figure out direction     */
        parent_end = READ;
        child_end = WRITE;
    } else if (*mode == 'w') {
        parent_end = WRITE;
        child_end = READ;
    } else
        return NULL;

    if (pipe(pfp) == -1) /* get a pipe       */
        return NULL;
    if ((*pid = fork()) == -1) { /* and a process    */
        close(pfp[0]);           /* or dispose of pipe   */
        close(pfp[1]);
        return NULL;
    }

    /* --------------- parent code here ------------------- */
    /*   need to close one end and fdopen other end     */

    if (*pid > 0) {
        if (close(pfp[child_end]) == -1)
            return NULL;
        return fdopen(pfp[parent_end], mode); /* same mode */
    }

    /* --------------- child code here --------------------- */
    /*   need to redirect stdin or stdout then exec the cmd  */

    if (close(pfp[parent_end]) == -1) /* close the other end  */
        exit(1);                      /* do NOT return    */

    if (dup2(pfp[child_end], child_end) == -1)
        exit(1);

    if (close(pfp[child_end]) == -1) /* done with this one   */
        exit(1);
    /* all set to run cmd   */
    execl("/bin/sh", "sh", "-c", command, NULL);
    exit(1);
}

int wait_for_child(int pid)
{
    int exit_code = -1;
    int wait_status = 0;
    for ( ; ; ) {
        waitpid(pid, &wait_status, 0);
        printf("Current status of test child %d is ", pid);
        if (WIFEXITED(wait_status)) {
            exit_code = WEXITSTATUS(wait_status);
            if (exit_code >= 128) {
                exit_code -= 128;
                printf("killed by signal %d\n", exit_code);
            } else {
                printf("exited, status=%d\n", exit_code);
            }
            break;
        } else if (WIFSIGNALED(wait_status)) {
            exit_code = WTERMSIG(wait_status);
            printf("killed by signal %d\n", exit_code);
            break;
        } else if (WIFSTOPPED(wait_status)) {
            exit_code = WSTOPSIG(wait_status);
            printf("stopped by signal %d\n", exit_code);
            break;
        } else if (WIFCONTINUED(wait_status)) {
            printf("continued\n");
        }
    }
    return exit_code;
}

std::string get_path_to_test()
{
    char buffer[2048];
    auto N = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    assert(N != -1);

    std::string full_path(buffer);
    // Strip off the last bit
    auto p = full_path.rfind('/');
    if (p != std::string::npos) {
        return full_path.substr(0, p);
    }
    return full_path;
}

} // namespace slog