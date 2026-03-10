#include <gtest/gtest.h>

#include "../src/suffix_array_indexer.hpp"

static std::vector<uint32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

TEST(SuffixArrayIndexer, BananaSA)
{
    // Classic example: sorted suffixes of "banana" are at positions 5,3,1,0,4,2
    SuffixArrayIndexer idx(codepoints("banana"));
    EXPECT_EQ(idx.get_sa(), (std::vector<int32_t>{5, 3, 1, 0, 4, 2}));
}

TEST(SuffixArrayIndexer, SingleChar)
{
    SuffixArrayIndexer idx(codepoints("x"));
    EXPECT_EQ(idx.get_sa(), (std::vector<int32_t>{0}));
}

TEST(SuffixArrayIndexer, Empty)
{
    SuffixArrayIndexer idx({});
    EXPECT_TRUE(idx.get_sa().empty());
}
