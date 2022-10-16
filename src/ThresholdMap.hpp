#pragma once
#include "LogConfig.hpp"

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

    /**
     * Defines the severity threshold to apply for unknown or empty
     * tags 
     */
    void set_default(int thr) { defaultThreshold = thr; }

    /**
     * Add (or replace) a tag, setting its threshold
     */
    void add_tag(const char* tag, int threshold);

    /**
     * Look up the threshold for the given tag. If the tag isn't found, 
     * is null, or empty, then return the defaultThreshold
     */
    int operator[](const char* tag) const;

protected:

    static int map_compare(void const* vkey, void const* velement);
    static int map_sort(void const* velement1, void const* velement2);

    struct Threshold {
        char tag[TAG_SIZE];
        int threshold;
    };

    int defaultThreshold;
    Threshold* map;
    unsigned long mapSize;
    unsigned long mapCapacity;
};

using ThresholdMap = FlatThresholdMap;
}
