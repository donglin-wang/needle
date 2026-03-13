#include <gtest/gtest.h>

#include "../src/matcher.hpp"
#include "../src/suffix_array_indexer.hpp"

static std::vector<int32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

static const std::string CORPUS_PATH = "/tmp/needle_test_corpus.bin";
static const std::string INDEX_PATH = "/tmp/needle_test_index.bin";

static std::unique_ptr<Matcher> make_matcher(const std::string &text)
{
    auto text_codepoints = codepoints(text);
    save_corpus(text_codepoints, CORPUS_PATH);
    SuffixArrayIndexer idx(std::move(text_codepoints));
    idx.save_index(INDEX_PATH);
    return std::make_unique<Matcher>(CORPUS_PATH, INDEX_PATH);
}

TEST(Matcher, SingleOccurrence)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("ell")), (std::vector<int32_t>{1}));
}

TEST(Matcher, MultipleOccurrences)
{
    auto m = make_matcher("abab");
    EXPECT_EQ(m->search(codepoints("ab")), (std::vector<int32_t>{0, 2}));
}

TEST(Matcher, ResultsAreSorted)
{
    auto m = make_matcher("aaaa");
    EXPECT_EQ(m->search(codepoints("aa")), (std::vector<int32_t>{0, 1, 2}));
}

TEST(Matcher, NotFound)
{
    auto m = make_matcher("hello");
    EXPECT_TRUE(m->search(codepoints("xyz")).empty());
}

TEST(Matcher, EmptyPattern)
{
    auto m = make_matcher("hello");
    EXPECT_TRUE(m->search({}).empty());
}

TEST(Matcher, PatternAtStart)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("he")), (std::vector<int32_t>{0}));
}

TEST(Matcher, PatternAtEnd)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("lo")), (std::vector<int32_t>{3}));
}

TEST(Matcher, FullCorpusMatch)
{
    auto m = make_matcher("hello");
    EXPECT_EQ(m->search(codepoints("hello")), (std::vector<int32_t>{0}));
}

TEST(Matcher, PatternLongerThanCorpus)
{
    auto m = make_matcher("hi");
    EXPECT_TRUE(m->search(codepoints("toolong")).empty());
}
