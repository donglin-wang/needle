#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <stack>
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
    // Iterative pre-order deserialization to avoid stack overflow on deep tries.
    static std::unique_ptr<Node> read_node(std::ifstream &file)
    {
        struct Frame
        {
            Node *node;
            int32_t remaining_children;
        };

        auto root = std::make_unique<Node>();

        std::stack<Frame> stack;

        // Read root header.
        int32_t num_children, num_positions;
        file.read(reinterpret_cast<char *>(&num_children), sizeof(num_children));
        file.read(reinterpret_cast<char *>(&num_positions), sizeof(num_positions));
        if (num_positions > 0)
        {
            root->positions.resize(num_positions);
            file.read(reinterpret_cast<char *>(root->positions.data()),
                      num_positions * sizeof(int32_t));
        }
        stack.push({root.get(), num_children});

        while (!stack.empty())
        {
            auto &frame = stack.top();
            if (frame.remaining_children <= 0)
            {
                stack.pop();
                continue;
            }

            int32_t key;
            file.read(reinterpret_cast<char *>(&key), sizeof(key));
            frame.remaining_children--;

            auto child = std::make_unique<Node>();
            file.read(reinterpret_cast<char *>(&num_children), sizeof(num_children));
            file.read(reinterpret_cast<char *>(&num_positions), sizeof(num_positions));
            if (num_positions > 0)
            {
                child->positions.resize(num_positions);
                file.read(reinterpret_cast<char *>(child->positions.data()),
                          num_positions * sizeof(int32_t));
            }

            Node *child_ptr = child.get();
            frame.node->children[key] = std::move(child);
            stack.push({child_ptr, num_children});
        }

        return root;
    }
};
