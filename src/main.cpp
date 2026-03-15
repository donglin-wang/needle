#include "matcher.hpp"
#include "suffix_array_indexer.hpp"
#include "suffix_tree_indexer.hpp"
#include "suffix_tree_matcher.hpp"
#include "utf8_reader.hpp"

#include <iostream>
#include <string>

static std::vector<int32_t> decode_pattern(const std::string &pattern)
{
    UTF8Reader reader("");
    std::vector<uint8_t> bytes(pattern.begin(), pattern.end());
    reader.parse_buffer(bytes, bytes.size());
    return reader.get_data();
}

static void cmd_index_sa(const std::string &corpus_utf8,
                         const std::string &corpus_bin,
                         const std::string &index_bin)
{
    UTF8Reader reader(corpus_utf8);
    reader.read();

    save_corpus(reader.get_data(), corpus_bin);

    SuffixArrayIndexer indexer(std::move(reader.get_data()));
    indexer.save_index(index_bin);

    std::cout << "suffix array index written to " << index_bin << "\n";
}

static void cmd_search_sa(const std::string &corpus_bin,
                          const std::string &index_bin,
                          const std::string &pattern)
{
    Matcher matcher(corpus_bin, index_bin);
    auto matches = matcher.search(decode_pattern(pattern));

    std::cout << matches.size() << " match(es)\n";
    for (int32_t pos : matches)
        std::cout << "  position " << pos << "\n";
}

static void cmd_index_st(const std::string &corpus_utf8,
                         const std::string &index_bin)
{
    UTF8Reader reader(corpus_utf8);
    reader.read();

    SuffixTreeIndexer indexer(reader.get_data());
    indexer.save_index(index_bin);

    std::cout << "suffix tree index written to " << index_bin << "\n";
}

static void cmd_search_st(const std::string &index_bin,
                          const std::string &pattern)
{
    SuffixTreeMatcher matcher(index_bin);
    auto matches = matcher.search(decode_pattern(pattern));

    std::cout << matches.size() << " match(es)\n";
    for (int32_t pos : matches)
        std::cout << "  position " << pos << "\n";
}

static void usage()
{
    std::cerr << "usage:\n"
              << "  needle index    <corpus.txt> <corpus.bin> <index.bin>\n"
              << "  needle search   <corpus.bin> <index.bin> <pattern>\n"
              << "  needle index-st <corpus.txt> <index.bin>\n"
              << "  needle search-st <index.bin> <pattern>\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        usage();
        return 1;
    }

    std::string cmd = argv[1];

    try
    {
        if (cmd == "index" && argc == 5)
            cmd_index_sa(argv[2], argv[3], argv[4]);
        else if (cmd == "search" && argc == 5)
            cmd_search_sa(argv[2], argv[3], argv[4]);
        else if (cmd == "index-st" && argc == 4)
            cmd_index_st(argv[2], argv[3]);
        else if (cmd == "search-st" && argc == 4)
            cmd_search_st(argv[2], argv[3]);
        else
        {
            usage();
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
