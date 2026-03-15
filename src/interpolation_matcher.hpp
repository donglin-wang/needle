#pragma once

#include "mapped_file.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

// Searches for a pattern in a corpus using a pre-built suffix array with
// interpolation search instead of binary search. Estimates probe position
// from the first codepoint of boundary suffixes — theoretically O(m log log n)
// for uniform data, but with less predictable memory access patterns.
class InterpolationMatcher
{
    MappedFile corpus_file;
    MappedFile index_file;
    const int32_t *text;
    const int32_t *suffix_array;
    int32_t n;

public:
    InterpolationMatcher(const std::string &corpus_path, const std::string &index_path)
        : corpus_file(corpus_path), index_file(index_path)
    {
        const char *cp = corpus_file.data();
        uint64_t corpus_n;
        std::memcpy(&corpus_n, cp, sizeof(corpus_n));
        text = reinterpret_cast<const int32_t *>(cp + sizeof(uint64_t));

        const char *ip = index_file.data();
        uint64_t index_n;
        std::memcpy(&index_n, ip, sizeof(index_n));
        suffix_array = reinterpret_cast<const int32_t *>(ip + sizeof(uint64_t));
        n = static_cast<int32_t>(index_n);

        if (corpus_n != index_n)
            throw std::runtime_error("corpus and index length mismatch");
    }

    std::vector<int32_t> search(const std::vector<int32_t> &pattern) const
    {
        if (pattern.empty())
            return {};

        const int32_t m = static_cast<int32_t>(pattern.size());

        auto cmp = [&](int32_t pos) -> int {
            for (int32_t j = 0; j < m; j++)
            {
                if (pos + j >= n)
                    return -1;
                if (text[pos + j] < pattern[j])
                    return -1;
                if (text[pos + j] > pattern[j])
                    return 1;
            }
            return 0;
        };

        const int32_t target = pattern[0];

        // Find left boundary.
        int32_t lo = 0, hi = n;
        while (lo < hi)
        {
            int32_t mid = interpolate(lo, hi, target);
            if (cmp(suffix_array[mid]) < 0)
                lo = mid + 1;
            else
                hi = mid;
        }
        const int32_t left = lo;

        // Find right boundary.
        hi = n;
        while (lo < hi)
        {
            int32_t mid = interpolate(lo, hi, target);
            if (cmp(suffix_array[mid]) <= 0)
                lo = mid + 1;
            else
                hi = mid;
        }
        const int32_t right = lo;

        std::vector<int32_t> results(suffix_array + left, suffix_array + right);
        std::sort(results.begin(), results.end());
        return results;
    }

private:
    // Estimate probe position from first codepoint of boundary suffixes.
    // Falls back to midpoint when boundary values are equal.
    int32_t interpolate(int32_t lo, int32_t hi, int32_t target) const
    {
        if (hi - lo <= 2)
            return lo + (hi - lo) / 2;

        int32_t val_lo = text[suffix_array[lo]];
        int32_t val_hi = text[suffix_array[hi - 1]];

        if (val_lo == val_hi)
            return lo + (hi - lo) / 2;

        int32_t clamped = std::max(val_lo, std::min(target, val_hi));
        int64_t range = hi - lo;
        int64_t mid = lo + (range * (clamped - val_lo)) / (val_hi - val_lo);

        return static_cast<int32_t>(std::max(static_cast<int64_t>(lo),
                                             std::min(mid, static_cast<int64_t>(hi - 1))));
    }
};
