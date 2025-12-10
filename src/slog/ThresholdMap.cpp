#include "ThresholdMap.hpp"
#include "SlogConfig.hpp"
#include <cstdlib>
#include <cstring>

namespace slog
{

FlatThresholdMap::FlatThresholdMap()
{
    defaultThreshold = 0;
    mapCapacity = 8;
    mapSize = 0;
    map = new Threshold[mapCapacity];
}

FlatThresholdMap::~FlatThresholdMap() { delete[] map; }

FlatThresholdMap& FlatThresholdMap::operator=(FlatThresholdMap const& other)
{
    defaultThreshold = other.defaultThreshold;
    mapCapacity = other.mapCapacity;
    mapSize = other.mapSize;
    map = new Threshold[mapCapacity];
    for (unsigned long i = 0; i < mapSize; i++) {
        map[i] = other.map[i];
    }
    return *this;
}

FlatThresholdMap::FlatThresholdMap(FlatThresholdMap const& other) { *this = other; }

FlatThresholdMap& FlatThresholdMap::operator=(FlatThresholdMap&& other) noexcept
{
    defaultThreshold = other.defaultThreshold;
    mapCapacity = other.mapCapacity;
    mapSize = other.mapSize;
    map = other.map;
    other.map = nullptr;    
    return *this;
}

FlatThresholdMap::FlatThresholdMap(FlatThresholdMap&& other) noexcept
{
    *this = other;
}

int FlatThresholdMap::map_compare(void const* vkey, void const* velement)
{
    char const* key = (char const*)vkey;
    Threshold const* element = reinterpret_cast<Threshold const*>(velement);
    return strncmp(key, element->tag, TAG_SIZE);
}

int FlatThresholdMap::map_sort(void const* velement1, void const* velement2)
{
    Threshold const* element1 = reinterpret_cast<Threshold const*>(velement1);
    Threshold const* element2 = reinterpret_cast<Threshold const*>(velement2);
    return strncmp(element1->tag, element2->tag, TAG_SIZE);
}

int FlatThresholdMap::operator[](char const* tag) const
{
    if (nullptr == tag || 0 == tag[0]) {
        return defaultThreshold;
    }
    auto const* found = reinterpret_cast<Threshold const*>(bsearch(tag, map, mapSize, sizeof(Threshold), map_compare));
    if (found) {
        return found->threshold;
    }
    return defaultThreshold;
}

void FlatThresholdMap::add_tag(char const* tag, int threshold_)
{
    // Check if this is a known tag
    auto* found = reinterpret_cast<Threshold*>(bsearch(tag, map, mapSize, sizeof(Threshold), map_compare));
    if (found) {
        found->threshold = threshold_;
        return;
    }

    // New tag, push it on the back
    if (mapSize == mapCapacity) {
        mapCapacity *= 2;
        Threshold* newMap = new Threshold[mapCapacity];
        for (unsigned long i = 0; i < mapSize; i++) {
            newMap[i] = map[i];
        }
        delete[] map;
        map = newMap;
    }
    map[mapSize].threshold = threshold_;
    strncpy(map[mapSize].tag, tag, TAG_SIZE-1);
    mapSize++;

    // Sort
    qsort(map, mapSize, sizeof(FlatThresholdMap::Threshold), map_sort);
}

} // namespace slog
