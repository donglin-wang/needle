#pragma once

#include <cstdint>
#include <random>
#include <string>

// Uniform random lowercase ASCII.
static std::string random_corpus(size_t n, uint64_t seed = 42)
{
    std::mt19937 rng(seed);
    std::uniform_int_distribution<> dist('a', 'z');
    std::string s(n, ' ');
    for (char &c : s)
        c = static_cast<char>(dist(rng));
    return s;
}

// Repeats a short token to fill n bytes. High match density — stresses
// result collection rather than search itself.
static std::string repetitive_corpus(size_t n, const std::string &token = "abcabc")
{
    std::string s;
    s.reserve(n);
    while (s.size() < n)
        s.append(token, 0, std::min(token.size(), n - s.size()));
    return s;
}

// Builds pseudo-natural-language text by picking words from a small
// vocabulary and joining them with spaces and occasional newlines.
static std::string natural_corpus(size_t n, uint64_t seed = 42)
{
    static const char *words[] = {
        "the", "of", "and", "to", "in", "a", "is", "that", "for", "it",
        "was", "on", "are", "be", "with", "as", "at", "this", "have", "from",
        "or", "an", "but", "not", "by", "one", "had", "word", "what", "all",
        "were", "we", "when", "your", "can", "said", "there", "each", "which",
        "their", "time", "will", "way", "about", "many", "then", "them",
        "would", "like", "been", "people", "has", "her", "two", "more",
        "write", "go", "see", "number", "no", "could", "my", "than",
        "first", "water", "called", "who", "its", "now", "find", "long",
        "down", "day", "did", "get", "come", "made", "may", "part",
        "over", "such", "after", "also", "back", "use", "just", "because",
        "good", "give", "most", "only", "tell", "very", "through", "great",
        "where", "help", "before", "line", "right", "too", "mean", "same"
    };
    static const size_t word_count = sizeof(words) / sizeof(words[0]);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> word_dist(0, word_count - 1);
    std::uniform_int_distribution<int> newline_dist(0, 15);

    std::string s;
    s.reserve(n);
    while (s.size() < n)
    {
        if (!s.empty())
            s += (newline_dist(rng) == 0) ? '\n' : ' ';
        s += words[word_dist(rng)];
    }
    s.resize(n);
    return s;
}
