#pragma once

#include "mapped_file.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Searches a suffix tree via mmap — zero deserialization.
// Walks compressed edges (path labels stored as text[start..end) indices),
// using binary search on sorted children at each node. O(m) search where
// m = pattern length.
class SuffixTreeMatcher
{
    struct SerialNode
    {
        int32_t edge_start;
        int32_t edge_end;
        int32_t children_offset;
        int32_t children_count;
        int32_t positions_offset;
        int32_t positions_count;
    };

    struct ChildEntry
    {
        int32_t key;
        int32_t node_index;
    };

    MappedFile file;
    int32_t num_nodes;
    const SerialNode *nodes;
    const ChildEntry *children;
    const int32_t *positions;
    const int32_t *text;
    int32_t text_length;

public:
    explicit SuffixTreeMatcher(const std::string &index_path)
        : file(index_path)
    {
        const char *p = file.data();

        int32_t total_children, total_positions;
        std::memcpy(&num_nodes, p, 4);
        std::memcpy(&total_children, p + 4, 4);
        std::memcpy(&total_positions, p + 8, 4);
        std::memcpy(&text_length, p + 12, 4);
        p += 16;

        nodes = reinterpret_cast<const SerialNode *>(p);
        p += num_nodes * sizeof(SerialNode);

        children = reinterpret_cast<const ChildEntry *>(p);
        p += total_children * sizeof(ChildEntry);

        positions = reinterpret_cast<const int32_t *>(p);
        p += total_positions * sizeof(int32_t);

        text = reinterpret_cast<const int32_t *>(p);
    }

    std::vector<int32_t> search(const std::vector<int32_t> &pattern) const
    {
        if (pattern.empty())
            return {};

        const int32_t m = static_cast<int32_t>(pattern.size());
        int32_t current = 0; // root
        int32_t i = 0;

        while (i < m)
        {
            int32_t child = find_child(current, pattern[i]);
            if (child < 0)
                return {};

            const auto &node = nodes[child];
            int32_t edge_length = node.edge_end - node.edge_start;

            for (int32_t j = 0; j < edge_length && i < m; j++, i++)
                if (text[node.edge_start + j] != pattern[i])
                    return {};

            if (i < m)
                current = child;
            else
                return {positions + node.positions_offset,
                        positions + node.positions_offset + node.positions_count};
        }

        return {};
    }

private:
    int32_t find_child(int32_t node_index, int32_t key) const
    {
        const auto &node = nodes[node_index];
        const auto *begin = children + node.children_offset;
        const auto *end = begin + node.children_count;

        auto it = std::lower_bound(begin, end, key,
                                   [](const ChildEntry &entry, int32_t k)
                                   { return entry.key < k; });

        if (it != end && it->key == key)
            return it->node_index;
        return -1;
    }
};
