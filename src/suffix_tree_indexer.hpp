#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

// Builds a suffix tree using Ukkonen's algorithm — O(n) construction.
// Edges represent substrings via (start, end) indices into the text (path
// compression), producing at most 2n+1 nodes vs the suffix trie's O(n^2).
// A sentinel character (-1) is appended to guarantee every suffix ends at
// a unique leaf.
//
// Serialization format (all values int32_t, mmap-friendly):
//   [num_nodes][total_children][total_positions][text_length]
//   [SerialNode * num_nodes]           — 6 fields each
//   [ChildEntry * total_children]      — sorted by key per node
//   [int32_t * total_positions]        — leaf positions in DFS order
//   [int32_t * text_length]            — original text + sentinel
class SuffixTreeIndexer
{
    struct Node
    {
        int32_t start;
        int32_t end;          // -1 for leaves (use leaf_end)
        int32_t suffix_link;
        int32_t suffix_index; // >= 0 for leaves, -1 for internal
        std::vector<std::pair<int32_t, int32_t>> children; // (key, node_index)
    };

    std::vector<Node> nodes;
    std::vector<int32_t> text;
    int32_t leaf_end;
    int32_t active_node;
    int32_t active_edge;
    int32_t active_length;
    int32_t remaining;

    int32_t new_node(int32_t start, int32_t end)
    {
        int32_t index = static_cast<int32_t>(nodes.size());
        nodes.push_back({start, end, 0, -1, {}});
        return index;
    }

    int32_t edge_length(int32_t node_index) const
    {
        const auto &node = nodes[node_index];
        return (node.end == -1 ? leaf_end : node.end) - node.start;
    }

    int32_t find_child(int32_t node_index, int32_t key) const
    {
        for (const auto &[k, index] : nodes[node_index].children)
            if (k == key)
                return index;
        return -1;
    }

    void add_child(int32_t parent, int32_t key, int32_t child)
    {
        nodes[parent].children.push_back({key, child});
    }

    void replace_child(int32_t parent, int32_t key, int32_t new_child)
    {
        for (auto &[k, index] : nodes[parent].children)
            if (k == key)
            {
                index = new_child;
                return;
            }
    }

    void extend(int32_t phase)
    {
        leaf_end = phase + 1;
        remaining++;
        int32_t last_new_internal = -1;

        while (remaining > 0)
        {
            if (active_length == 0)
                active_edge = phase;

            int32_t child = find_child(active_node, text[active_edge]);

            if (child < 0)
            {
                int32_t leaf = new_node(phase, -1);
                add_child(active_node, text[active_edge], leaf);
                if (last_new_internal >= 0)
                {
                    nodes[last_new_internal].suffix_link = active_node;
                    last_new_internal = -1;
                }
            }
            else
            {
                int32_t length = edge_length(child);
                if (active_length >= length)
                {
                    active_node = child;
                    active_length -= length;
                    active_edge += length;
                    continue;
                }

                if (text[nodes[child].start + active_length] == text[phase])
                {
                    active_length++;
                    if (last_new_internal >= 0)
                    {
                        nodes[last_new_internal].suffix_link = active_node;
                        last_new_internal = -1;
                    }
                    break;
                }

                // Split the edge.
                int32_t split = new_node(nodes[child].start,
                                         nodes[child].start + active_length);
                replace_child(active_node, text[active_edge], split);
                nodes[child].start += active_length;
                add_child(split, text[nodes[child].start], child);

                int32_t leaf = new_node(phase, -1);
                add_child(split, text[phase], leaf);

                if (last_new_internal >= 0)
                    nodes[last_new_internal].suffix_link = split;
                last_new_internal = split;
            }

            remaining--;
            if (active_node == 0 && active_length > 0)
            {
                active_length--;
                active_edge = phase - remaining + 1;
            }
            else if (nodes[active_node].suffix_link > 0)
            {
                active_node = nodes[active_node].suffix_link;
            }
            else
            {
                active_node = 0;
            }
        }
    }

    // Resolve leaf ends and compute suffix_index for each leaf.
    void compute_suffix_indices()
    {
        int32_t n = static_cast<int32_t>(text.size());
        for (auto &node : nodes)
            if (node.end == -1)
                node.end = n;

        struct Frame
        {
            int32_t node_index;
            int32_t depth;
        };
        std::stack<Frame> stack;
        stack.push({0, 0});

        while (!stack.empty())
        {
            auto [node_index, depth] = stack.top();
            stack.pop();
            auto &node = nodes[node_index];
            int32_t length = node_index == 0 ? 0 : (node.end - node.start);
            int32_t new_depth = depth + length;

            if (node.children.empty())
                node.suffix_index = n - new_depth;
            else
                for (const auto &[key, child_index] : node.children)
                    stack.push({child_index, new_depth});
        }
    }

public:
    explicit SuffixTreeIndexer(const std::vector<int32_t> &input)
    {
        text = input;
        text.push_back(-1); // sentinel ensures every suffix ends at a unique leaf

        new_node(0, 0); // root
        active_node = 0;
        active_edge = -1;
        active_length = 0;
        remaining = 0;
        leaf_end = 0;

        for (int32_t i = 0; i < static_cast<int32_t>(text.size()); i++)
            extend(i);

        compute_suffix_indices();
    }

    void save_index(const std::string &path) const
    {
        auto sorted = nodes;
        for (auto &node : sorted)
            std::sort(node.children.begin(), node.children.end());

        // Collect positions via iterative post-order DFS.
        // Each node gets a contiguous range covering all leaves in its subtree.
        int32_t num_nodes = static_cast<int32_t>(sorted.size());
        std::vector<int32_t> pos_offset(num_nodes);
        std::vector<int32_t> pos_count(num_nodes);
        std::vector<int32_t> all_positions;
        int32_t original_length = static_cast<int32_t>(text.size()) - 1;

        struct Frame
        {
            int32_t node_index;
            int32_t child_cursor;
        };
        std::stack<Frame> stack;
        stack.push({0, 0});

        while (!stack.empty())
        {
            auto &frame = stack.top();
            auto &node = sorted[frame.node_index];

            if (frame.child_cursor == 0)
                pos_offset[frame.node_index] =
                    static_cast<int32_t>(all_positions.size());

            if (frame.child_cursor < static_cast<int32_t>(node.children.size()))
            {
                int32_t child = node.children[frame.child_cursor].second;
                frame.child_cursor++;
                stack.push({child, 0});
            }
            else
            {
                if (node.children.empty() && node.suffix_index >= 0 &&
                    node.suffix_index < original_length)
                    all_positions.push_back(node.suffix_index);
                pos_count[frame.node_index] =
                    static_cast<int32_t>(all_positions.size()) -
                    pos_offset[frame.node_index];
                stack.pop();
            }
        }

        int32_t total_children = 0;
        for (const auto &node : sorted)
            total_children += static_cast<int32_t>(node.children.size());
        int32_t total_positions = static_cast<int32_t>(all_positions.size());
        int32_t text_length = static_cast<int32_t>(text.size());

        std::ofstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot open " + path);

        // Header (16 bytes)
        file.write(reinterpret_cast<const char *>(&num_nodes), 4);
        file.write(reinterpret_cast<const char *>(&total_children), 4);
        file.write(reinterpret_cast<const char *>(&total_positions), 4);
        file.write(reinterpret_cast<const char *>(&text_length), 4);

        // Node descriptors (24 bytes each)
        int32_t children_offset = 0;
        for (int32_t i = 0; i < num_nodes; i++)
        {
            const auto &node = sorted[i];
            int32_t edge_start = node.start;
            int32_t edge_end = node.end;
            int32_t cc = static_cast<int32_t>(node.children.size());
            file.write(reinterpret_cast<const char *>(&edge_start), 4);
            file.write(reinterpret_cast<const char *>(&edge_end), 4);
            file.write(reinterpret_cast<const char *>(&children_offset), 4);
            file.write(reinterpret_cast<const char *>(&cc), 4);
            file.write(reinterpret_cast<const char *>(&pos_offset[i]), 4);
            file.write(reinterpret_cast<const char *>(&pos_count[i]), 4);
            children_offset += cc;
        }

        // Children array
        for (const auto &node : sorted)
            for (const auto &[key, index] : node.children)
            {
                file.write(reinterpret_cast<const char *>(&key), 4);
                file.write(reinterpret_cast<const char *>(&index), 4);
            }

        // Positions array
        if (!all_positions.empty())
            file.write(reinterpret_cast<const char *>(all_positions.data()),
                       all_positions.size() * 4);

        // Text (including sentinel)
        file.write(reinterpret_cast<const char *>(text.data()),
                   text.size() * 4);
    }
};
