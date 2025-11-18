#include "doctest.h"
#include "slog/JournaldSink.hpp"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include <cstdint>
#include <thread>
#define SD_JOURNAL_SUPPRESS_LOCATION
#include <systemd/sd-journal.h>

TEST_CASE("Journald")
{
    slog::LogConfig config;
    config.set_default_threshold(slog::INFO);
    config.set_sink(std::make_shared<slog::JournaldSink>());
    slog::start_logger(config);

    Slog(NOTE, "tag") << "Test log";
    slog::stop_logger();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    sd_journal* journal;
    REQUIRE(0 == sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY |
                                               SD_JOURNAL_CURRENT_USER));

    int i = 0;
    int const maxRecordsToSearch = 128;
    SD_JOURNAL_FOREACH_BACKWARDS(journal)
    {
        char const* data;
        std::size_t N;
        if (0 == sd_journal_get_data(journal, "CODE_FILE", (void const**)&data,
                                     &N)) {
            std::string codeFile(data);
            if (std::string::npos != codeFile.find("JournaldTest.cpp")) {
                REQUIRE(0 == sd_journal_get_data(journal, "CODE_FUNC",
                                                 (void const**)&data, &N));
                std::string codeFunc(data);
                CHECK(codeFunc.find("DOCTEST_ANON_FUNC") != std::string::npos);
                REQUIRE(0 == sd_journal_get_data(journal, "THREAD",
                                                 (void const**)&data, &N));
                REQUIRE(0 == sd_journal_get_data(journal, "TIMESTAMP",
                                                 (void const**)&data, &N));
                REQUIRE(0 == sd_journal_get_data(journal, "PRIORITY",
                                                 (void const**)&data, &N));
                std::string priority(data);
                auto equal = priority.find("=");
                REQUIRE(equal != std::string::npos);
                int prio = atoi(priority.substr(equal + 1).c_str());
                CHECK(prio == slog::NOTE / 100);
                REQUIRE(0 == sd_journal_get_data(journal, "TAG",
                                                 (void const**)&data, &N));
                std::string tag(data);
                equal = tag.find("=");
                REQUIRE(equal != std::string::npos);
                CHECK(tag.substr(equal + 1) == "tag");
                REQUIRE(0 == sd_journal_get_data(journal, "MESSAGE",
                                                 (void const**)&data, &N));
                std::string message(data);
                equal = message.find("=");
                REQUIRE(equal != std::string::npos);
                message = message.substr(equal + 1);
                // Note the terminators in systemd seem a little dodgy
                CHECK(0 == strncmp(message.c_str(), "Test log", 8));                
                break;
            }
        }
        i++;
        if (i >= maxRecordsToSearch) {
            break;
        }
    }
    CHECK(i < maxRecordsToSearch);
    sd_journal_close(journal);
}

TEST_CASE("Journald.big")
{
    long line_max = slog::JournaldSink::line_max();
    std::string text(line_max + 32, '\0');
    for (long i = 0; i < text.size(); i++) {
        text[i] = 'a' + (i % 26);
    }
    slog::LogConfig config;
    config.set_default_threshold(slog::INFO);
    config.set_sink(std::make_shared<slog::JournaldSink>());
    slog::start_logger(config);

    Slog(NOTE, "tag") << text;
    slog::stop_logger();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    sd_journal* journal;
    REQUIRE(0 == sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY |
                                               SD_JOURNAL_CURRENT_USER));

    int i = 0;
    int found_count = 0;
    int const maxRecordsToSearch = 128;
    SD_JOURNAL_FOREACH_BACKWARDS(journal)
    {
        char const* data;
        std::size_t N;
        if (0 == sd_journal_get_data(journal, "CODE_FILE", (void const**)&data,
                                     &N)) {
            std::string codeFile(data);
            if (std::string::npos != codeFile.find("JournaldTest.cpp")) {
                REQUIRE(0 == sd_journal_get_data(journal, "CODE_FUNC",
                                                 (void const**)&data, &N));
                std::string codeFunc(data);
                CHECK(codeFunc.find("DOCTEST_ANON_FUNC") != std::string::npos);
                REQUIRE(0 == sd_journal_get_data(journal, "THREAD",
                                                 (void const**)&data, &N));
                REQUIRE(0 == sd_journal_get_data(journal, "TIMESTAMP",
                                                 (void const**)&data, &N));
                REQUIRE(0 == sd_journal_get_data(journal, "PRIORITY",
                                                 (void const**)&data, &N));
                std::string priority(data);
                auto equal = priority.find("=");
                REQUIRE(equal != std::string::npos);
                int prio = atoi(priority.substr(equal + 1).c_str());
                CHECK(prio == slog::NOTE / 100);
                REQUIRE(0 == sd_journal_get_data(journal, "TAG",
                                                 (void const**)&data, &N));
                std::string tag(data);
                equal = tag.find("=");
                REQUIRE(equal != std::string::npos);
                CHECK(tag.substr(equal + 1) == "tag");
                REQUIRE(0 == sd_journal_get_data(journal, "MESSAGE",
                                                 (void const**)&data, &N));                
                uint64_t expected_size = (found_count == 0? 32 + 8 : line_max - 8);                
                // N is the total length plus "MESSAGE=" (8 chars)
                CHECK(N - 8 == expected_size);
                found_count++;
                if (found_count >= 2) { 
                    break;
                }
            }
        }
        i++;
        if (i >= maxRecordsToSearch) {
            break;
        }
    }
    CHECK(i < maxRecordsToSearch);
    sd_journal_close(journal);
}

#if 0
// FIXME this is not a good test. It is true in general for modern systems,
// but we need some other way to check for LineMax to compare with
TEST_CASE("Journald.LineMax" )
{
    auto line_max = slog::JournaldSink::line_max();    
    CHECK(line_max == 48 * 1024);
}
#endif