#include <gtest/gtest.h>

#include "../src/utf8_reader.hpp"

static std::vector<int32_t> parse(std::initializer_list<uint8_t> bytes)
{
    UTF8Reader reader("");
    std::vector<uint8_t> buf(bytes);
    reader.parse_buffer(buf, buf.size());
    return reader.get_data();
}

TEST(UTF8Reader, ASCIISingleBytes)
{
    auto result = parse({'H', 'i', '!'});
    EXPECT_EQ(result, (std::vector<int32_t>{'H', 'i', '!'}));
}

TEST(UTF8Reader, TwoByteSequence)
{
    // U+00E9 é = 0xC3 0xA9
    auto result = parse({0xC3, 0xA9});
    EXPECT_EQ(result, (std::vector<int32_t>{0x00E9}));
}

TEST(UTF8Reader, ThreeByteSequence)
{
    // U+4E2D 中 = 0xE4 0xB8 0xAD
    auto result = parse({0xE4, 0xB8, 0xAD});
    EXPECT_EQ(result, (std::vector<int32_t>{0x4E2D}));
}

TEST(UTF8Reader, FourByteSequence)
{
    // U+1F600 😀 = 0xF0 0x9F 0x98 0x80
    auto result = parse({0xF0, 0x9F, 0x98, 0x80});
    EXPECT_EQ(result, (std::vector<int32_t>{0x1F600}));
}

TEST(UTF8Reader, InvalidLeadingByteProducesReplacement)
{
    auto result = parse({0xFF});
    EXPECT_EQ(result, (std::vector<int32_t>{0xFFFD}));
}

TEST(UTF8Reader, InvalidContinuationByteProducesReplacement)
{
    // 0xC3 expects a continuation byte, but 0x41 ('A') is not one
    auto result = parse({0xC3, 0x41});
    EXPECT_EQ(result[0], 0xFFFDu);
}

TEST(UTF8Reader, SurrogateCodePointProducesReplacement)
{
    // U+D800 encoded as UTF-8: 0xED 0xA0 0x80
    auto result = parse({0xED, 0xA0, 0x80});
    EXPECT_EQ(result, (std::vector<int32_t>{0xFFFD}));
}

TEST(UTF8Reader, IncompleteSequenceAtBoundaryReturnsCarry)
{
    UTF8Reader reader("");
    // 0xC3 alone starts a 2-byte sequence — incomplete
    std::vector<uint8_t> buf = {0xC3};
    size_t carry = reader.parse_buffer(buf, buf.size());
    EXPECT_EQ(carry, 1u);
    EXPECT_TRUE(reader.get_data().empty());
}

TEST(UTF8Reader, MixedASCIIAndMultibyte)
{
    // 'A', U+00E9, 'Z'
    auto result = parse({'A', 0xC3, 0xA9, 'Z'});
    EXPECT_EQ(result, (std::vector<int32_t>{'A', 0x00E9, 'Z'}));
}
