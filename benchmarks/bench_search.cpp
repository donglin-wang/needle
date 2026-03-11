#include <benchmark/benchmark.h>

#include "../src/matcher.hpp"
#include "../src/naive_matcher.hpp"
#include "../src/suffix_array_indexer.hpp"
#include "corpus_generator.hpp"

#include <string>

static const std::string CORPUS_PATH = "/tmp/needle_bench_corpus.bin";
static const std::string INDEX_PATH  = "/tmp/needle_bench_index.bin";

static std::vector<uint32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

// Extracts an 8-char pattern from the middle of the corpus.
static std::vector<uint32_t> mid_pattern(const std::string &text, size_t n)
{
    return codepoints(text.substr(n / 2, 8));
}

// Helper: build and save corpus + index, return pattern.
static std::vector<uint32_t> setup_index(const std::string &text, size_t n)
{
    auto text_codepoints = codepoints(text);
    save_corpus(text_codepoints, CORPUS_PATH);
    SuffixArrayIndexer indexer(std::move(text_codepoints));
    indexer.save_index(INDEX_PATH);
    return mid_pattern(text, n);
}

// Helper: save corpus only, return pattern.
static std::vector<uint32_t> setup_corpus(const std::string &text, size_t n)
{
    save_corpus(codepoints(text), CORPUS_PATH);
    return mid_pattern(text, n);
}

// --- Index build ---

static void BM_IndexBuild(benchmark::State &state)
{
    std::string text = random_corpus(state.range(0));
    for (auto _ : state)
    {
        SuffixArrayIndexer indexer(codepoints(text));
        benchmark::DoNotOptimize(indexer.get_sa().data());
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_IndexBuild)->Range(1 << 10, 1 << 20)->Unit(benchmark::kMillisecond);

// --- SA search: random, repetitive, natural ---

static void BM_SASearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto pattern = setup_index(text, n);
    Matcher matcher(CORPUS_PATH, INDEX_PATH);
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SASearch_Random)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_SASearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto pattern = setup_index(text, n);
    Matcher matcher(CORPUS_PATH, INDEX_PATH);
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SASearch_Repetitive)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_SASearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto pattern = setup_index(text, n);
    Matcher matcher(CORPUS_PATH, INDEX_PATH);
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SASearch_Natural)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

// --- Naive search: random, repetitive, natural ---

static void BM_NaiveSearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto pattern = setup_corpus(text, n);
    NaiveMatcher matcher(CORPUS_PATH);
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Random)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_NaiveSearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto pattern = setup_corpus(text, n);
    NaiveMatcher matcher(CORPUS_PATH);
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Repetitive)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_NaiveSearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto pattern = setup_corpus(text, n);
    NaiveMatcher matcher(CORPUS_PATH);
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Natural)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
