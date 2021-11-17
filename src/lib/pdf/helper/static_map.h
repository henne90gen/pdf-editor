#pragma once

#include <unordered_map>

namespace pdf {

// TODO finish this implementation
template <typename K, typename V> struct StaticMap {
    struct Entry {
        enum struct Status {
            EMPTY  = 0,
            FILLED = 1,
        };

        Status status = Status::EMPTY;
        K key;
        V value;
    };
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = int;
        using pointer           = value_type *;
        using reference         = value_type &;

        explicit Iterator(pointer ptr) : m_ptr(ptr) {}

        reference operator*() const { return *m_ptr; }
        pointer operator->() { return m_ptr; }
        Iterator &operator++() {
            m_ptr++;
            return *this;
        }
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        friend bool operator==(const Iterator &a, const Iterator &b) { return a.m_ptr == b.m_ptr; };
        friend bool operator!=(const Iterator &a, const Iterator &b) { return a.m_ptr != b.m_ptr; };

      private:
        pointer m_ptr;
    };

    Entry *entries;
    size_t entryCount;
    size_t capacity;

    size_t hash(const K &key) { return 0; }

    [[nodiscard]] size_t size() const { return entryCount; }
    Iterator begin() { return Iterator(entries); }
    Iterator end() { return Iterator(); }

    static StaticMap<K, V> create(Allocator &allocator, const std::unordered_map<K, V> &map) {
        auto entryCount = map.size();
        auto capacity   = entryCount * 2;
        auto entries    = (Entry *)allocator.allocate_chunk(capacity * sizeof(Entry));
        return {
              .entries    = entries,
              .entryCount = entryCount,
              .capacity   = capacity,
        };
    }
};

} // namespace pdf
