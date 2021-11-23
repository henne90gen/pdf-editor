#pragma once

#include <spdlog/spdlog.h>
#include <unordered_map>
#include <optional>

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
        using value_type        = Entry;
        using pointer           = value_type *;
        using reference         = value_type &;

        explicit Iterator(pointer ptr) : m_ptr(ptr) {
            while (m_ptr->status == Entry::Status::EMPTY) {
                m_ptr++;
            }
        }

        reference operator*() const { return *m_ptr; }
        pointer operator->() { return m_ptr; }
        Iterator &operator++() {
            m_ptr++;
            while (m_ptr->status == Entry::Status::EMPTY) {
                m_ptr++;
            }
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

    [[nodiscard]] size_t size() const { return entryCount; }
    Iterator begin() { return Iterator(&entries[0]); }
    Iterator begin() const { return Iterator(&entries[0]); }
    Iterator end() { return Iterator(&entries[capacity]); }
    Iterator end() const { return Iterator(&entries[capacity]); }

    /// searches the map for the given key and returns either the found element or an empty optional
    std::optional<V> find(const K &key) {
        auto i = 0;
        auto h = std::hash<K>()(key);
        while (true) {
            auto index  = (h + i) % capacity;
            auto &entry = entries[index];
            if (entry.status == Entry::Status::EMPTY) {
                return {};
            }

            if (key == entry.key) {
                return entry.value;
            }

            i++;
            // NOTE: this only works because we are not modifying the map
            // -> thus there will always be an empty slot
        }
        return {};
    }

    /// same as find(), except that the element is removed after retrieving it
    std::optional<V> remove(const K &key) {
        auto i = 0;
        auto h = std::hash<K>()(key);
        while (true) {
            auto index  = (h + i) % capacity;
            auto &entry = entries[index];
            if (entry.status == Entry::Status::EMPTY) {
                return {};
            }

            if (key == entry.key) {
                auto value = entry.value;
                entry.status = Entry::Status::EMPTY;
                entry.key    = {};
                entry.value  = {};
                return value;
            }

            i++;
            // NOTE: this only works because we are not modifying the map
            // -> thus there will always be an empty slot
        }
        return {};
    }

    static StaticMap<K, V> create(Allocator &allocator, const std::unordered_map<K, V> &map) {
        auto entryCount = map.size();
        auto capacity   = entryCount * 2; // TODO think about using less memory here
        auto entries    = (Entry *)allocator.allocate_chunk(capacity * sizeof(Entry));
        std::memset(entries, 0, capacity * sizeof(Entry));

        for (auto &entry : map) {
            // TODO switch to quadratic probing
            auto i = 0;
            auto h = std::hash<K>()(entry.first);
            while (true) {
                auto index     = (h + i) % capacity;
                auto &newEntry = entries[index];
                if (newEntry.status != Entry::Status::EMPTY) {
                    i++;
                    continue;
                }

                newEntry.status = Entry::Status::FILLED;
                newEntry.key    = entry.first;
                newEntry.value  = entry.second;
                break;
            }
        }

        return {
              .entries    = entries,
              .entryCount = entryCount,
              .capacity   = capacity,
        };
    }
};

} // namespace pdf
