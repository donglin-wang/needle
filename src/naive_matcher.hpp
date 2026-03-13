#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// Linear scan substring search over a heap-resident corpus.
// Reads the entire corpus into memory upfront so the scan runs
// over a contiguous buffer with no page-fault overhead.
class NaiveMatcher
{
    std::vector<int32_t> text;
    int32_t n;

public:
    explicit NaiveMatcher(const std::string &corpus_path)
    {
        std::ifstream file(corpus_path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot open " + corpus_path);

        uint64_t corpus_n;
        file.read(reinterpret_cast<char *>(&corpus_n), sizeof(corpus_n));
        n = static_cast<int32_t>(corpus_n);
        text.resize(n);
        file.read(reinterpret_cast<char *>(text.data()), n * sizeof(int32_t));
    }

    std::vector<int32_t> search(const std::vector<int32_t> &pattern) const
    {
        if (pattern.empty())
            return {};

        const int32_t m = static_cast<int32_t>(pattern.size());
        std::vector<int32_t> results;

        for (int32_t i = 0; i <= n - m; i++)
        {
            if (std::memcmp(text.data() + i, pattern.data(), m * sizeof(int32_t)) == 0)
                results.push_back(i);
        }

        return results;
    }
};
