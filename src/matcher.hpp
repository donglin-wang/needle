#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// RAII wrapper around a read-only memory-mapped file.
class MappedFile
{
    char *pointer = nullptr;
    size_t length = 0;
    int file_descriptor = -1;

public:
    explicit MappedFile(const std::string &path)
    {
        file_descriptor = open(path.c_str(), O_RDONLY);
        if (file_descriptor < 0)
            throw std::runtime_error("cannot open " + path);

        struct stat st;
        if (fstat(file_descriptor, &st) < 0)
        {
            close(file_descriptor);
            throw std::runtime_error("fstat failed for " + path);
        }

        length = static_cast<size_t>(st.st_size);
        void *p = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
        if (p == MAP_FAILED)
        {
            close(file_descriptor);
            throw std::runtime_error("mmap failed for " + path);
        }
        pointer = static_cast<char *>(p);
    }

    ~MappedFile()
    {
        if (pointer)
            munmap(pointer, length);
        if (file_descriptor >= 0)
            close(file_descriptor);
    }

    MappedFile(const MappedFile &) = delete;
    MappedFile &operator=(const MappedFile &) = delete;

    const char *data() const { return pointer; }
    size_t size() const { return length; }
};

// Searches for a pattern in a corpus using a pre-built suffix array.
// Both corpus and index are memory-mapped from disk; no heap allocation
// for the text or SA. IO is handled by the OS page cache on demand.
class Matcher
{
    MappedFile corpus_file;
    MappedFile index_file;
    const uint32_t *text;
    const int32_t *suffix_array;
    int32_t n;

public:
    // corpus_path: binary file written by suffix_array_corpus()
    // index_path:  binary file written by SuffixArrayIndexer::suffix_array_index()
    Matcher(const std::string &corpus_path, const std::string &index_path)
        : corpus_file(corpus_path), index_file(index_path)
    {
        // Corpus layout: [uint64_t n][uint32_t * n]
        const char *cp = corpus_file.data();
        uint64_t corpus_n;
        std::memcpy(&corpus_n, cp, sizeof(corpus_n));
        text = reinterpret_cast<const uint32_t *>(cp + sizeof(uint64_t));

        // Index layout: [uint64_t n][int32_t * n]
        const char *ip = index_file.data();
        uint64_t index_n;
        std::memcpy(&index_n, ip, sizeof(index_n));
        suffix_array = reinterpret_cast<const int32_t *>(ip + sizeof(uint64_t));
        n = static_cast<int32_t>(index_n);

        if (corpus_n != index_n)
            throw std::runtime_error("corpus and index length mismatch");
    }

    // Returns sorted starting positions of all occurrences of pattern.
    std::vector<int32_t> search(const std::vector<uint32_t> &pattern) const
    {
        if (pattern.empty())
            return {};

        const int32_t m = static_cast<int32_t>(pattern.size());

        // Compare the suffix at suffix_array[i] against the pattern.
        auto cmp = [&](int32_t pos) -> int {
            for (int32_t j = 0; j < m; j++)
            {
                if (pos + j >= n)
                    return -1; // suffix shorter than pattern
                if (text[pos + j] < pattern[j])
                    return -1;
                if (text[pos + j] > pattern[j])
                    return 1;
            }
            return 0;
        };

        // Find [left, right) in the SA — the range of suffixes starting
        // with pattern.
        int32_t lo = 0, hi = n;
        while (lo < hi)
        {
            int32_t mid = lo + (hi - lo) / 2;
            if (cmp(suffix_array[mid]) < 0)
                lo = mid + 1;
            else
                hi = mid;
        }
        const int32_t left = lo;

        hi = n;
        while (lo < hi)
        {
            int32_t mid = lo + (hi - lo) / 2;
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
};
