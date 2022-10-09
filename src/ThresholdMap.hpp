#pragma once

namespace slog {

/*
 * A char const* to int map with a default value for
 * missing/empty/null keys. This is a flat structure 
 * sorted by key (and re-sorted with every addTag()),
 * then searched via bisection
 */
class FlatThresholdMap {
public:
    FlatThresholdMap();
    ~FlatThresholdMap();
    FlatThresholdMap& operator= (FlatThresholdMap const&);
    FlatThresholdMap(FlatThresholdMap const& other);

    void set_default(int thr) { defaultThreshold = thr; }

    void add_tag(const char* tag, int threshold);

    int operator[](const char* tag) const;

protected:

    static int map_compare(void const* vkey, void const* velement);
    static int map_sort(void const* velement1, void const* velement2);

    static constexpr unsigned long TAG_SIZE = 32 - sizeof(int);
    struct alignas(32) Threshold {
        char tag[TAG_SIZE];
        int threshold;
    };

    int defaultThreshold;
    Threshold* map;
    unsigned long mapSize;
    unsigned long mapCapacity;
};

#if 0
// Testing shows these options are slower
#include <map>
#include <unordered_map>
#include <string>
template<class Map>
class BasicThresholdMap {
public:
    BasicThresholdMap() : defaultThreshold(0) { }
    
    void set_default(int thr) {
        defaultThreshold = thr;
    }

    void add_tag(const char* tag, int threshold) { map[tag] = threshold; }

    int operator[](const char* tag) const {
        auto it = map.find(tag);
        if (it == map.end()) {
            return defaultThreshold;
        }
        return it->second;
    }
protected:
    int defaultThreshold;
    Map map;
};
#endif

using ThresholdMap = FlatThresholdMap;
}
