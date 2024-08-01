#pragma once

#include <unordered_map>
#include <vector>

#include "pdf/memory/stl_allocator.h"

namespace pdf {

template <typename T> using Vector = std::vector<T, StlAllocator<T>>;
template <typename K, typename V>
using UnorderedMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, StlAllocator<std::pair<const K, V>>>;

} // namespace pdf
