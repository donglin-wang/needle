#include <benchmark/benchmark.h>

#include "../src/matcher.hpp"
#include "../src/naive_matcher.hpp"
#include "../src/suffix_array_indexer.hpp"

#include <random>
#include <string>

static const std::string CORPUS_PATH = "/tmp/needle_bench_corpus.bin";
static const std::string INDEX_PATH  = "/tmp/needle_bench_index.bin";

static std::string random_corpus(size_t n, uint64_t seed = 42)
{
    std::mt19937 rng(seed);
    std::uniform_int_distribution<> dist('a', 'z');
    std::string s(n, ' ');
    for (char &c : s)
        c = static_cast<char>(dist(rng));
    return s;
}

static std::vector<uint32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

// Measures suffix array construction throughput at varying corpus sizes.
static void BM_IndexBuild(benchmark::State &state)
{
    std::string text = random_corpus(state.range(0));
    for (auto _ : state)
    {
        SuffixArrayIndexer idx(codepoints(text));
        benchmark::DoNotOptimize(idx.get_sa().data());
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_IndexBuild)->Range(1 << 10, 1 << 20)->Unit(benchmark::kMillisecond);

// Measures binary search throughput against a pre-built index.
// Setup (index build + mmap) is outside the timed loop.
static void BM_Search(benchmark::State &state)
{
    std::string text = random_corpus(state.range(0));
    auto text_codepoints = codepoints(text);
    save_corpus(text_codepoints, CORPUS_PATH);
    SuffixArrayIndexer idx(std::move(text_codepoints));
    idx.save_index(INDEX_PATH);

    // 8-char pattern taken from the middle of the corpus
    auto pattern = codepoints(text.substr(state.range(0) / 2, 8));

    Matcher matcher(CORPUS_PATH, INDEX_PATH);
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Search)->Range(1 << 10, 1 << 20)->Unit(benchmark::kMicrosecond);

// Measures naive linear scan throughput over the same mmap'd corpus.
static void BM_NaiveSearch(benchmark::State &state)
{
    std::string text = random_corpus(state.range(0));
    auto text_codepoints = codepoints(text);
    save_corpus(text_codepoints, CORPUS_PATH);

    auto pattern = codepoints(text.substr(state.range(0) / 2, 8));

    NaiveMatcher matcher(CORPUS_PATH);
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_NaiveSearch)->Range(1 << 10, 1 << 20)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
