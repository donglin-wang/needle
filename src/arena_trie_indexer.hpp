#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// Builds a suffix trie using arena-style allocation: all nodes live in a single
// vector, children are (key, node_index) pairs. Same O(n^2) algorithm as
// TrieIndexer, but nodes are physically contiguous in memory.
//
// Serialization format (all values int32_t):
//   [num_nodes][total_children][total_positions]
//   [ArenaNode * num_nodes]           — fixed-size descriptors
//   [ChildEntry * total_children]     — all children, sorted by key per node
//   [int32_t * total_positions]       — all position values
class ArenaTrieIndexer
{
    struct BuildNode
    {
        std::vector<std::pair<int32_t, int32_t>> children; // (key, node_index)
        std::vector<int32_t> positions;
    };

    std::vector<BuildNode> nodes;

public:
    explicit ArenaTrieIndexer(const std::vector<int32_t> &text)
    {
        nodes.emplace_back(); // root = index 0
        const int32_t n = static_cast<int32_t>(text.size());
        for (int32_t i = 0; i < n; i++)
            insert_suffix(text, i, n);
    }

    void save_index(const std::string &path) const
    {
        // Sort children by key for binary search at query time.
        // We work on a copy to keep save_index const.
        auto sorted = nodes;
        for (auto &node : sorted)
            std::sort(node.children.begin(), node.children.end());

        int32_t num_nodes = static_cast<int32_t>(sorted.size());
        int32_t total_children = 0;
        int32_t total_positions = 0;
        for (const auto &node : sorted)
        {
            total_children += static_cast<int32_t>(node.children.size());
            total_positions += static_cast<int32_t>(node.positions.size());
        }

        std::ofstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot open " + path);

        // Header
        file.write(reinterpret_cast<const char *>(&num_nodes), 4);
        file.write(reinterpret_cast<const char *>(&total_children), 4);
        file.write(reinterpret_cast<const char *>(&total_positions), 4);

        // Node descriptors: [children_offset, children_count, positions_offset, positions_count]
        int32_t children_offset = 0;
        int32_t positions_offset = 0;
        for (const auto &node : sorted)
        {
            int32_t children_count = static_cast<int32_t>(node.children.size());
            int32_t positions_count = static_cast<int32_t>(node.positions.size());
            file.write(reinterpret_cast<const char *>(&children_offset), 4);
            file.write(reinterpret_cast<const char *>(&children_count), 4);
            file.write(reinterpret_cast<const char *>(&positions_offset), 4);
            file.write(reinterpret_cast<const char *>(&positions_count), 4);
            children_offset += children_count;
            positions_offset += positions_count;
        }

        // Children: pairs of (key, node_index)
        for (const auto &node : sorted)
            for (const auto &[key, index] : node.children)
            {
                file.write(reinterpret_cast<const char *>(&key), 4);
                file.write(reinterpret_cast<const char *>(&index), 4);
            }

        // Positions
        for (const auto &node : sorted)
            if (!node.positions.empty())
                file.write(reinterpret_cast<const char *>(node.positions.data()),
                           node.positions.size() * 4);
    }

private:
    void insert_suffix(const std::vector<int32_t> &text, int32_t start, int32_t n)
    {
        int32_t current = 0; // root
        for (int32_t j = start; j < n; j++)
        {
            int32_t key = text[j];
            int32_t child = find_child(current, key);
            if (child < 0)
            {
                child = static_cast<int32_t>(nodes.size());
                nodes.emplace_back();
                nodes[current].children.push_back({key, child});
            }
            current = child;
            nodes[current].positions.push_back(start);
        }
    }

    int32_t find_child(int32_t node_index, int32_t key) const
    {
        for (const auto &[k, idx] : nodes[node_index].children)
            if (k == key)
                return idx;
        return -1;
    }
};
