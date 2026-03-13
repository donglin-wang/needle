#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// Builds a suffix trie from a corpus and serializes it to disk.
// O(n^2) build time and memory. Paired with TrieMatcher for search.
class TrieIndexer
{
    struct Node
    {
        std::unordered_map<int32_t, std::unique_ptr<Node>> children;
        std::vector<int32_t> positions;
    };

    std::unique_ptr<Node> root;

public:
    explicit TrieIndexer(const std::vector<int32_t> &text)
        : root(std::make_unique<Node>())
    {
        const int32_t n = static_cast<int32_t>(text.size());
        for (int32_t i = 0; i < n; i++)
            insert_suffix(text, i, n);
    }

    // Pre-order serialization:
    //   per node: [int32_t num_children][int32_t num_positions][positions...][children...]
    //   per child: [int32_t key][child node]
    void save_index(const std::string &path) const
    {
        std::ofstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot open " + path);
        write_node(file, *root);
    }

private:
    void insert_suffix(const std::vector<int32_t> &text, int32_t start, int32_t n)
    {
        Node *node = root.get();
        for (int32_t j = start; j < n; j++)
        {
            auto &child = node->children[text[j]];
            if (!child)
                child = std::make_unique<Node>();
            node = child.get();
            node->positions.push_back(start);
        }
    }

    static void write_node(std::ofstream &file, const Node &node)
    {
        int32_t num_children = static_cast<int32_t>(node.children.size());
        int32_t num_positions = static_cast<int32_t>(node.positions.size());
        file.write(reinterpret_cast<const char *>(&num_children), sizeof(num_children));
        file.write(reinterpret_cast<const char *>(&num_positions), sizeof(num_positions));
        if (num_positions > 0)
            file.write(reinterpret_cast<const char *>(node.positions.data()),
                       num_positions * sizeof(int32_t));
        for (const auto &[key, child] : node.children)
        {
            file.write(reinterpret_cast<const char *>(&key), sizeof(key));
            write_node(file, *child);
        }
    }
};
