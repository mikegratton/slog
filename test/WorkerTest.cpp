#include "InMemorySink.hpp"
#include "doctest.h"
#include "slog/LogChannel.hpp"
#include "slog/LogRecordPool.hpp"
#include "slog/LogSetup.hpp"
#include "slog/LogWorker.hpp"
#include "slog/Signal.hpp"
#include "slog/FileSink.hpp"
#include "slog/ThresholdMap.hpp"
#include "slog/LoggerSingleton.hpp"
#include "slog/slog.hpp"
#include <chrono>
#include <thread>

TEST_CASE("LogWorker")
{
    slog::stop_logger();

    slog::LogWorker worker;
    CHECK(worker.channel_count() == 0);

    long alloc_size = 32*(sizeof(slog::LogRecord) + 128);
    auto pool = std::make_shared<slog::LogRecordPool>(slog::DISCARD, alloc_size, 128);
    slog::ThresholdMap tmap;
    tmap.set_default(slog::DBUG);
    auto sink = std::make_shared<InMemorySink>();
    auto channel = std::make_shared<slog::LogChannel>(sink, tmap, pool);
    worker.add_channel(15, channel);
    CHECK(worker.channel_count() == 1);
    CHECK(worker.get_channel(15)->pool_free_count() == 32);

    // Work around doctest issue string-ifying shared_ptr
    bool channel_is_null = (nullptr == worker.get_channel(0));
    CHECK(channel_is_null);

    worker.stop();
    CHECK(worker.channel_count() == 0);

    worker.add_channel(15, channel);
    slog::reset_worker_counts();
    slog::set_signal_state(slog::SLOG_ACTIVE);
    worker.start();
    worker.start();
    auto* message = pool->allocate();
    message->meta().capture("", "", 1, slog::INFO, "", 15);
    int size = snprintf(message->message(), message->capacity(), "hello");
    message->size(size);
    worker.push_to_queue(message);
    for (int i = 0; i < 50; i++) {
        if (worker.get_channel(15)->pool_free_count() == 32) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    worker.stop();
    CHECK(slog::get_signal_state() == slog::SLOG_STOPPED);    
    CHECK(worker.get_channel(15)->pool_free_count() == 32);
    REQUIRE(sink->contents().size() == 1);
    CHECK(sink->contents().front() == "hello");
}

TEST_CASE("LogWorker.twoThread")
{    
    auto sink = std::make_shared<InMemorySink>();
    auto fsink = std::make_shared<slog::FileSink>();
    fsink->set_echo(false);
    fsink->set_formatter(slog::no_meta_format);
    auto fsink2 = std::make_shared<slog::FileSink>();
    fsink2->set_file(".", "file2");
    fsink2->set_formatter(slog::no_meta_format);
    fsink2->set_echo(false);

    std::vector<slog::LogConfig> config(3);
    config[0].set_sink(sink);
    config[0].set_default_threshold(slog::DBUG);
    config[1].set_sink(fsink);
    config[1].set_default_threshold(slog::DBUG);
    config[1].set_worker_thread_id(1);
    config[2].set_sink(fsink2);
    config[2].set_worker_thread_id(1);
    config[2].set_default_threshold(slog::DBUG);

    slog::start_logger(config);
    CHECK(slog::detail::Logger::worker_count() == 2);
    Slog(NOTE, "", 0) << "inmem";
    Slog(NOTE, "", 1) << "fsink1";
    Slog(NOTE, "", 2) << "fsink2";
    slog::stop_logger();

    REQUIRE(sink->contents().size() == 1);
    CHECK(sink->contents()[0] == "inmem");

    FILE* f = fopen(fsink->get_file_name(), "r");
    REQUIRE(f);
    char buffer[1024];    
    CHECK(fgets(buffer, sizeof(buffer), f));
    CHECK(std::string("fsink1\n") == buffer);
    fclose(f);
    f = fopen(fsink2->get_file_name(), "r");
    REQUIRE(f);
    CHECK(fgets(buffer, sizeof(buffer), f));
    CHECK(std::string("fsink2\n") == buffer);
    fclose(f);
    std::remove(fsink->get_file_name());
    std::remove(fsink2->get_file_name());
}