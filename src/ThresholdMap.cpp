#include "ThresholdMap.hpp"
#include <cstring>
#include <cstdlib>

namespace slog {

FlatThresholdMap::FlatThresholdMap() {
    defaultThreshold = 0;
    mapCapacity = 8;
    mapSize = 0;
    map = new Threshold[mapCapacity];
}

FlatThresholdMap::~FlatThresholdMap() {
    delete[] map;
}


FlatThresholdMap& FlatThresholdMap::operator= (FlatThresholdMap const& other) {
    defaultThreshold = other.defaultThreshold;
    mapCapacity = other.mapCapacity;
    mapSize = other.mapSize;
    map = new Threshold[mapCapacity];
    for (unsigned long i=0; i<mapSize; i++) {
        map[i] = other.map[i];
    }
    return *this;
}

FlatThresholdMap::FlatThresholdMap(FlatThresholdMap const& other) {
    *this = other;
}

int FlatThresholdMap::map_compare(void const* vkey, void const* velement) {
    char const* key = (char const*) vkey;
    Threshold const* element = (Threshold const*) velement;
    return strncmp(key, element->tag, TAG_SIZE);
}

int FlatThresholdMap::map_sort(const void* velement1, const void* velement2) {
    Threshold const* element1 = (Threshold const*) velement1;
    Threshold const* element2 = (Threshold const*) velement2;
    return strncmp(element1->tag, element2->tag, TAG_SIZE);
}


int FlatThresholdMap::operator[](const char* tag) const {
    if (nullptr == tag || 0 == tag[0]) {
        return defaultThreshold;
    }
    auto const* found = (Threshold const*) bsearch(tag, map, mapSize, sizeof(Threshold),
                        map_compare);
    if (found) {
        return found->threshold;
    }
    return defaultThreshold;
}


void FlatThresholdMap::add_tag(const char* tag, int threshold_) {
    // Check if this is a known tag 
    auto* found = (Threshold*) bsearch(tag, map, mapSize, sizeof(Threshold), map_compare);
    if (found) {
        found->threshold = threshold_;
        return;
    }
    
    // New tag, push it on the back
    if (mapSize == mapCapacity) {
        mapCapacity *= 2;
        Threshold* newMap = new Threshold[mapCapacity];
        for (long i=0; i<mapSize; i++) {
            newMap[i] = map[i];
        }
        delete[] map;
        map = newMap;
    }
    map[mapSize].threshold = threshold_;
    strncpy(map[mapSize].tag, tag, sizeof(map[mapSize].tag));
    mapSize++;

    // Sort
    qsort(map, mapSize, sizeof(FlatThresholdMap::Threshold), map_sort);
}


}
