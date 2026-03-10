#include "matcher.hpp"
#include "suffix_array_indexer.hpp"
#include "utf8_reader.hpp"

#include <iostream>
#include <string>

static void cmd_index(const std::string &corpus_utf8,
                      const std::string &corpus_bin,
                      const std::string &index_bin)
{
    UTF8Reader reader(corpus_utf8);
    reader.read();

    // Save decoded codepoints before moving them into the indexer.
    save_corpus(reader.get_data(), corpus_bin);

    SuffixArrayIndexer indexer(std::move(reader.get_data()));
    indexer.save_index(index_bin);

    std::cout << "index written to " << index_bin << "\n";
}

static void cmd_search(const std::string &corpus_bin,
                       const std::string &index_bin,
                       const std::string &pattern)
{
    // Decode the raw UTF-8 bytes of the command-line argument into codepoints.
    UTF8Reader preader("");
    std::vector<uint8_t> bytes(pattern.begin(), pattern.end());
    preader.parse_buffer(bytes, bytes.size());

    Matcher matcher(corpus_bin, index_bin);
    auto matches = matcher.search(preader.get_data());

    std::cout << matches.size() << " match(es)\n";
    for (int32_t pos : matches)
        std::cout << "  position " << pos << "\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage:\n"
                  << "  needle index <corpus.txt> <corpus.bin> <index.bin>\n"
                  << "  needle search <corpus.bin> <index.bin> <pattern>\n";
        return 1;
    }

    std::string cmd = argv[1];

    try
    {
        if (cmd == "index" && argc == 5)
            cmd_index(argv[2], argv[3], argv[4]);
        else if (cmd == "search" && argc == 5)
            cmd_search(argv[2], argv[3], argv[4]);
        else
        {
            std::cerr << "invalid arguments\n";
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
