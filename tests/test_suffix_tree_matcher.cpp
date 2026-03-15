#include <gtest/gtest.h>

#include "../src/suffix_tree_indexer.hpp"
#include "../src/suffix_tree_matcher.hpp"

static std::vector<int32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

static const std::string INDEX_PATH = "/tmp/needle_test_suffix_tree_index.bin";

static std::unique_ptr<SuffixTreeMatcher> make_matcher(const std::string &text)
{
    auto cp = codepoints(text);
    SuffixTreeIndexer indexer(cp);
    indexer.save_index(INDEX_PATH);
    return std::make_unique<SuffixTreeMatcher>(INDEX_PATH);
}

TEST(SuffixTreeMatcher, SingleOccurrence)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("ell")), (std::vector<int32_t>{1}));
}

TEST(SuffixTreeMatcher, MultipleOccurrences)
{
    auto m = make_matcher("abab");
    auto results = m->search(codepoints("ab"));
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results, (std::vector<int32_t>{0, 2}));
}

TEST(SuffixTreeMatcher, ResultsAreSorted)
{
    auto m = make_matcher("aaaa");
    auto results = m->search(codepoints("aa"));
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results, (std::vector<int32_t>{0, 1, 2}));
}

TEST(SuffixTreeMatcher, NotFound)
{
    auto m = make_matcher("hello");
    EXPECT_TRUE(m->search(codepoints("xyz")).empty());
}

TEST(SuffixTreeMatcher, EmptyPattern)
{
    auto m = make_matcher("hello");
    EXPECT_TRUE(m->search({}).empty());
}

TEST(SuffixTreeMatcher, PatternAtStart)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("he")), (std::vector<int32_t>{0}));
}

TEST(SuffixTreeMatcher, PatternAtEnd)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("lo")), (std::vector<int32_t>{3}));
}

TEST(SuffixTreeMatcher, FullCorpusMatch)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("hello")), (std::vector<int32_t>{0}));
}

TEST(SuffixTreeMatcher, PatternLongerThanCorpus)
{
    auto m = make_matcher("hi");
    EXPECT_TRUE(m->search(codepoints("toolong")).empty());
}

TEST(SuffixTreeMatcher, BananaClassic)
{
    auto m = make_matcher("banana");
    auto results = m->search(codepoints("ana"));
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results, (std::vector<int32_t>{1, 3}));
}

TEST(SuffixTreeMatcher, SingleCharCorpus)
{
    auto m = make_matcher("a");
    EXPECT_EQ(m->search(codepoints("a")), (std::vector<int32_t>{0}));
    EXPECT_TRUE(m->search(codepoints("b")).empty());
}
