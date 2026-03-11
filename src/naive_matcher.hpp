#pragma once

#include "mapped_file.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

// Linear scan substring search over an mmap'd corpus file.
// Same corpus format and IO path as Matcher — the only difference
// is the search algorithm: O(n*m) scan instead of suffix-array binary search.
class NaiveMatcher
{
    MappedFile corpus_file;
    const uint32_t *text;
    int32_t n;

public:
    explicit NaiveMatcher(const std::string &corpus_path)
        : corpus_file(corpus_path)
    {
        const char *cp = corpus_file.data();
        uint64_t corpus_n;
        std::memcpy(&corpus_n, cp, sizeof(corpus_n));
        text = reinterpret_cast<const uint32_t *>(cp + sizeof(uint64_t));
        n = static_cast<int32_t>(corpus_n);
    }

    std::vector<int32_t> search(const std::vector<uint32_t> &pattern) const
    {
        if (pattern.empty())
            return {};

        const int32_t m = static_cast<int32_t>(pattern.size());
        std::vector<int32_t> results;

        for (int32_t i = 0; i <= n - m; i++)
        {
            if (std::memcmp(text + i, pattern.data(), m * sizeof(uint32_t)) == 0)
                results.push_back(i);
        }

        return results;
    }
};
