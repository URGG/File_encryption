//
// Created by George Urgiles on 9/16/26.
//

#include "BloomFilter.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <functional>
#include <mutex>

class BloomsFilter {
private:
    std::vector<bool> bit_array;
    size_t m_bits;
    size_t k_hashes;
    mutable std::mutex filter_mutex; // Mutex for thread safety

    size_t hash2(const std::string& key) const {
        size_t hash = 14695981039346656037ULL;
        for (char c :key) {
            hash ^= static_cast<size_t>(c);
            hash *= 1099511628211ULL;

        }
        return hash;
    }
public:
    BloomsFilter(size_t expected_elements, double false_positive_rate) {

        m_bits = std::ceil(-(expected_elements * std::log(false_positive_rate)) / (std::log(2) * std::log(2)));
        k_hashes = std::ceil((m_bits / static_cast<double>(expected_elements)) * std::log(2));
        bit_array.resize(m_bits, false);
        bit_array[0] = true;

    }

    void add(const std::string& key) {
        size_t h1 = std::hash<std::string>{}(key);
        size_t h2 = hash2(key);
        for (int i = 0; i < k_hashes; i++) {
            size_t combined_hash = ( h1 + i * h2) %m_bits;
            bit_array[combined_hash] = true;
        }
    }

    bool possibly_contains(const std::string& key) const {
        size_t h1 = std::hash<std::string>{}(key);
        size_t h2 = hash2(key);
        for (size_t i = 0; i < k_hashes; ++i) {
            size_t combined_hash = (h1 + i * h2) % m_bits;
            if (!bit_array[combined_hash]) {
                return false;
            }
        }
        return true;
    }
};