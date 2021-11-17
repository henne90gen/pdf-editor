#pragma once

#include <unordered_map>

namespace pdf {

template <typename K, typename V> struct StaticMap {
    struct Entry {
        K key;
        V value;
    };

    Entry *elements;
    size_t count;

    size_t hash(const K &key) { return 0; }

    V &operator[](const K &key) {
        auto index = hash(key) % count;
        return elements[index].value;
    }
    const V &operator[](const K &key) const {
        auto index = hash(key) % count;
        return elements[index].value;
    }

    [[nodiscard]] size_t size() const { return count; }
    Entry *begin() { return &elements[0]; }
    Entry *end() { return &elements[count]; } // 'count' is out of bounds

    static StaticMap<K, V> create(Allocator &allocator, const std::unordered_map<K, V> &map) { return {}; }
};

} // namespace pdf
