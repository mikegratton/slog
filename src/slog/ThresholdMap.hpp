#pragma once
#include "LogConfig.hpp"

namespace slog
{

/**
 * @brief A char const* to int map with a default value for
 * missing/empty/null keys.
 *
 * This is a flat structure sorted by key (and re-sorted with
 * every addTag()), then searched via bisection.
 */
class FlatThresholdMap
{
  public:
    FlatThresholdMap();
    ~FlatThresholdMap();
    FlatThresholdMap& operator=(FlatThresholdMap const&);
    FlatThresholdMap(FlatThresholdMap const& other);
    FlatThresholdMap& operator=(FlatThresholdMap&&) noexcept = default;
    FlatThresholdMap(FlatThresholdMap&& other) noexcept = default;

    /**
     * @brief Defines the severity threshold to apply for unknown or empty
     * tags
     */
    void set_default(int thr) { defaultThreshold = thr; }

    /**
     * @brief Add (or replace) a tag, setting its threshold
     */
    void add_tag(char const* tag, int threshold);

    /**
     * @brief Look up the threshold for the given tag.
     *
     * If the tag isn't found, is null, or is empty, then return the
     * defaultThreshold
     */
    int operator[](char const* tag) const;

  protected:
    // Function used to search the map
    static int map_compare(void const* vkey, void const* velement);

    // Function used to sort the map
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
} // namespace slog
