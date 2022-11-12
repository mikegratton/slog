#include "StatsSink.hpp"
#include <chrono>
#include <cstdio>

void StatsSink::record(const slog::RecordNode* node) {
    long delta = std::chrono::system_clock::now().time_since_epoch().count() - node->rec.meta.time;
    sample.push_back(delta);
}

StatsSink::~StatsSink() {
    FILE* f = fopen("stats.dat", "w");
    for (auto s : sample) {
        fprintf(f, "%ld\n", s);
    }
    fclose(f);
}


