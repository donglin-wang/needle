#pragma once

#include <libsais.h>

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// Binary format for both corpus and index files:
//   [uint64_t n][element_type * n]

inline void save_corpus(const std::vector<uint32_t> &text, const std::string &path)
{
    std::ofstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("cannot open " + path);
    uint64_t n = text.size();
    f.write(reinterpret_cast<const char *>(&n), sizeof(n));
    f.write(reinterpret_cast<const char *>(text.data()), n * sizeof(uint32_t));
}

class SuffixArrayIndexer
{
    std::vector<int32_t> sa;

public:
    // Takes ownership of codepoints. The corpus is released from memory
    // before libsais_int runs, so peak usage is one int32_t copy + the SA.
    explicit SuffixArrayIndexer(std::vector<uint32_t> codepoints)
    {
        build(std::move(codepoints));
    }

    void save_index(const std::string &path) const
    {
        std::ofstream f(path, std::ios::binary);
        if (!f)
            throw std::runtime_error("cannot open " + path);
        uint64_t n = sa.size();
        f.write(reinterpret_cast<const char *>(&n), sizeof(n));
        f.write(reinterpret_cast<const char *>(sa.data()), n * sizeof(int32_t));
    }

    const std::vector<int32_t> &get_sa() const { return sa; }

private:
    void build(std::vector<uint32_t> text)
    {
        if (text.empty())
            return;

        const size_t n = text.size();

        // Safe: Unicode caps at U+10FFFF (spec hard limit), fits in int32_t.
        // int32_t required by libsais_int API.
        std::vector<int32_t> t(n);
        for (size_t i = 0; i < n; i++)
            t[i] = static_cast<int32_t>(text[i]);

        // Release corpus memory before the (potentially long) SA construction.
        std::vector<uint32_t>().swap(text);

        sa.resize(n);
        constexpr int32_t k = 0x110000; // Unicode ceiling
        int32_t ret = libsais_int(t.data(), sa.data(), static_cast<int32_t>(n), k, 0);
        if (ret != 0)
            throw std::runtime_error("libsais_int failed");
    }
};
