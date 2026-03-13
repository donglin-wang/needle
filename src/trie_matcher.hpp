#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// Deserializes a suffix trie from disk and searches it.
// Each node is a separate heap allocation with an unordered_map of children —
// deliberately cache-unfriendly to contrast with contiguous index structures.
class TrieMatcher
{
    struct Node
    {
        std::unordered_map<int32_t, std::unique_ptr<Node>> children;
        std::vector<int32_t> positions;
    };

    std::unique_ptr<Node> root;

public:
    explicit TrieMatcher(const std::string &index_path)
    {
        std::ifstream file(index_path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot open " + index_path);
        root = read_node(file);
    }

    std::vector<int32_t> search(const std::vector<int32_t> &pattern) const
    {
        if (pattern.empty())
            return {};

        const Node *node = root.get();
        for (int32_t codepoint : pattern)
        {
            auto it = node->children.find(codepoint);
            if (it == node->children.end())
                return {};
            node = it->second.get();
        }

        return node->positions;
    }

private:
    static std::unique_ptr<Node> read_node(std::ifstream &file)
    {
        auto node = std::make_unique<Node>();

        int32_t num_children, num_positions;
        file.read(reinterpret_cast<char *>(&num_children), sizeof(num_children));
        file.read(reinterpret_cast<char *>(&num_positions), sizeof(num_positions));

        if (num_positions > 0)
        {
            node->positions.resize(num_positions);
            file.read(reinterpret_cast<char *>(node->positions.data()),
                      num_positions * sizeof(int32_t));
        }

        for (int32_t i = 0; i < num_children; i++)
        {
            int32_t key;
            file.read(reinterpret_cast<char *>(&key), sizeof(key));
            node->children[key] = read_node(file);
        }

        return node;
    }
};
