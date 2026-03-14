#include <gtest/gtest.h>

#include "../src/arena_trie_indexer.hpp"
#include "../src/arena_trie_matcher.hpp"

static std::vector<int32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

static const std::string INDEX_PATH = "/tmp/needle_test_arena_trie_index.bin";

static std::unique_ptr<ArenaTrieMatcher> make_matcher(const std::string &text)
{
    auto cp = codepoints(text);
    ArenaTrieIndexer indexer(cp);
    indexer.save_index(INDEX_PATH);
    return std::make_unique<ArenaTrieMatcher>(INDEX_PATH);
}

TEST(ArenaTrieMatcher, SingleOccurrence)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("ell")), (std::vector<int32_t>{1}));
}

TEST(ArenaTrieMatcher, MultipleOccurrences)
{
    auto m = make_matcher("abab");
    auto results = m->search(codepoints("ab"));
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results, (std::vector<int32_t>{0, 2}));
}

TEST(ArenaTrieMatcher, ResultsAreSorted)
{
    auto m = make_matcher("aaaa");
    auto results = m->search(codepoints("aa"));
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results, (std::vector<int32_t>{0, 1, 2}));
}

TEST(ArenaTrieMatcher, NotFound)
{
    auto m = make_matcher("hello");
    EXPECT_TRUE(m->search(codepoints("xyz")).empty());
}

TEST(ArenaTrieMatcher, EmptyPattern)
{
    auto m = make_matcher("hello");
    EXPECT_TRUE(m->search({}).empty());
}

TEST(ArenaTrieMatcher, PatternAtStart)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("he")), (std::vector<int32_t>{0}));
}

TEST(ArenaTrieMatcher, PatternAtEnd)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("lo")), (std::vector<int32_t>{3}));
}

TEST(ArenaTrieMatcher, FullCorpusMatch)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("hello")), (std::vector<int32_t>{0}));
}

TEST(ArenaTrieMatcher, PatternLongerThanCorpus)
{
    auto m = make_matcher("hi");
    EXPECT_TRUE(m->search(codepoints("toolong")).empty());
}
