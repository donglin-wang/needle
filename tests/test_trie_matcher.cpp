#include <gtest/gtest.h>

#include "../src/trie_indexer.hpp"
#include "../src/trie_matcher.hpp"

static std::vector<int32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

static const std::string INDEX_PATH = "/tmp/needle_test_trie_index.bin";

static std::unique_ptr<TrieMatcher> make_matcher(const std::string &text)
{
    auto cp = codepoints(text);
    TrieIndexer indexer(cp);
    indexer.save_index(INDEX_PATH);
    return std::make_unique<TrieMatcher>(INDEX_PATH);
}

TEST(TrieMatcher, SingleOccurrence)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("ell")), (std::vector<int32_t>{1}));
}

TEST(TrieMatcher, MultipleOccurrences)
{
    auto m = make_matcher("abab");
    auto results = m->search(codepoints("ab"));
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results, (std::vector<int32_t>{0, 2}));
}

TEST(TrieMatcher, ResultsAreSorted)
{
    auto m = make_matcher("aaaa");
    auto results = m->search(codepoints("aa"));
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results, (std::vector<int32_t>{0, 1, 2}));
}

TEST(TrieMatcher, NotFound)
{
    auto m = make_matcher("hello");
    EXPECT_TRUE(m->search(codepoints("xyz")).empty());
}

TEST(TrieMatcher, EmptyPattern)
{
    auto m = make_matcher("hello");
    EXPECT_TRUE(m->search({}).empty());
}

TEST(TrieMatcher, PatternAtStart)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("he")), (std::vector<int32_t>{0}));
}

TEST(TrieMatcher, PatternAtEnd)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("lo")), (std::vector<int32_t>{3}));
}

TEST(TrieMatcher, FullCorpusMatch)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("hello")), (std::vector<int32_t>{0}));
}

TEST(TrieMatcher, PatternLongerThanCorpus)
{
    auto m = make_matcher("hi");
    EXPECT_TRUE(m->search(codepoints("toolong")).empty());
}
