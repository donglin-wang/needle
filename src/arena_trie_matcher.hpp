#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// Deserializes an arena-format suffix trie and searches it.
// All nodes, children, and positions live in three contiguous arrays —
// cache-friendly contrast to TrieMatcher's per-node heap allocations.
class ArenaTrieMatcher
{
    struct ArenaNode
    {
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

    std::vector<ArenaNode> nodes;
    std::vector<ChildEntry> children;
    std::vector<int32_t> positions;

public:
    explicit ArenaTrieMatcher(const std::string &index_path)
    {
        std::ifstream file(index_path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot open " + index_path);

        int32_t num_nodes, total_children, total_positions;
        file.read(reinterpret_cast<char *>(&num_nodes), 4);
        file.read(reinterpret_cast<char *>(&total_children), 4);
        file.read(reinterpret_cast<char *>(&total_positions), 4);

        nodes.resize(num_nodes);
        file.read(reinterpret_cast<char *>(nodes.data()),
                  num_nodes * sizeof(ArenaNode));

        children.resize(total_children);
        file.read(reinterpret_cast<char *>(children.data()),
                  total_children * sizeof(ChildEntry));

        positions.resize(total_positions);
        file.read(reinterpret_cast<char *>(positions.data()),
                  total_positions * sizeof(int32_t));
    }

    std::vector<int32_t> search(const std::vector<int32_t> &pattern) const
    {
        if (pattern.empty())
            return {};

        int32_t current = 0; // root
        for (int32_t codepoint : pattern)
        {
            int32_t child = find_child(current, codepoint);
            if (child < 0)
                return {};
            current = child;
        }

        const auto &node = nodes[current];
        return {positions.begin() + node.positions_offset,
                positions.begin() + node.positions_offset + node.positions_count};
    }

private:
    int32_t find_child(int32_t node_index, int32_t key) const
    {
        const auto &node = nodes[node_index];
        const auto *begin = children.data() + node.children_offset;
        const auto *end = begin + node.children_count;

        // Children are sorted by key — binary search.
        auto it = std::lower_bound(begin, end, key,
                                   [](const ChildEntry &entry, int32_t k)
                                   { return entry.key < k; });

        if (it != end && it->key == key)
            return it->node_index;
        return -1;
    }
};
