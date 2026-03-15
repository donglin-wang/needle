#include <benchmark/benchmark.h>

#include "../src/matcher.hpp"
#include "../src/naive_matcher.hpp"
#include "../src/trie_indexer.hpp"
#include "../src/trie_matcher.hpp"
#include "../src/arena_trie_indexer.hpp"
#include "../src/arena_trie_matcher.hpp"
#include "../src/interpolation_matcher.hpp"
#include "../src/suffix_array_indexer.hpp"
#include "corpus_generator.hpp"

#include <string>
#include <sys/resource.h>

static struct rusage snapshot_rusage()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage;
}

static void report_page_faults(benchmark::State &state,
                               const struct rusage &before,
                               const struct rusage &after)
{
    state.counters["minor_faults"] = benchmark::Counter(
        static_cast<double>(after.ru_minflt - before.ru_minflt),
        benchmark::Counter::kAvgIterations);
    state.counters["major_faults"] = benchmark::Counter(
        static_cast<double>(after.ru_majflt - before.ru_majflt),
        benchmark::Counter::kAvgIterations);
}

static const std::string CORPUS_PATH           = "/tmp/needle_bench_corpus.bin";
static const std::string INDEX_PATH            = "/tmp/needle_bench_index.bin";
static const std::string TRIE_INDEX_PATH       = "/tmp/needle_bench_trie_index.bin";
static const std::string ARENA_TRIE_INDEX_PATH = "/tmp/needle_bench_arena_trie_index.bin";

static std::vector<int32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

// Extracts an 8-char pattern from the middle of the corpus.
static std::vector<int32_t> mid_pattern(const std::string &text, size_t n)
{
    return codepoints(text.substr(n / 2, 8));
}

// Helper: build and save corpus + index, return pattern.
static std::vector<int32_t> setup_index(const std::string &text, size_t n)
{
    auto text_codepoints = codepoints(text);
    save_corpus(text_codepoints, CORPUS_PATH);
    SuffixArrayIndexer indexer(std::move(text_codepoints));
    indexer.save_index(INDEX_PATH);
    return mid_pattern(text, n);
}

// Helper: save corpus only, return pattern.
static std::vector<int32_t> setup_corpus(const std::string &text, size_t n)
{
    save_corpus(codepoints(text), CORPUS_PATH);
    return mid_pattern(text, n);
}

// Helper: build trie index and save to disk, return pattern.
static std::vector<int32_t> setup_trie(const std::string &text, size_t n)
{
    auto text_codepoints = codepoints(text);
    TrieIndexer indexer(text_codepoints);
    indexer.save_index(TRIE_INDEX_PATH);
    return mid_pattern(text, n);
}

// Helper: build arena trie index and save to disk, return pattern.
static std::vector<int32_t> setup_arena_trie(const std::string &text, size_t n)
{
    auto text_codepoints = codepoints(text);
    ArenaTrieIndexer indexer(text_codepoints);
    indexer.save_index(ARENA_TRIE_INDEX_PATH);
    return mid_pattern(text, n);
}

// --- Index build ---

static void BM_IndexBuild(benchmark::State &state)
{
    std::string text = random_corpus(state.range(0));
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        SuffixArrayIndexer indexer(codepoints(text));
        benchmark::DoNotOptimize(indexer.get_sa().data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
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
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SASearch_Random)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_SASearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto pattern = setup_index(text, n);
    Matcher matcher(CORPUS_PATH, INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SASearch_Repetitive)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_SASearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto pattern = setup_index(text, n);
    Matcher matcher(CORPUS_PATH, INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
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
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Random)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_NaiveSearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto pattern = setup_corpus(text, n);
    NaiveMatcher matcher(CORPUS_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Repetitive)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_NaiveSearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto pattern = setup_corpus(text, n);
    NaiveMatcher matcher(CORPUS_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Natural)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

// --- Trie search: random, repetitive, natural ---
// All capped at 8KB — O(n^2) build dominates setup time regardless of corpus type.

static void BM_TrieSearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto pattern = setup_trie(text, n);
    TrieMatcher matcher(TRIE_INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_TrieSearch_Random)->Range(1 << 10, 1 << 13)->Unit(benchmark::kMicrosecond);

static void BM_TrieSearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto pattern = setup_trie(text, n);
    TrieMatcher matcher(TRIE_INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_TrieSearch_Repetitive)->Range(1 << 10, 1 << 13)->Unit(benchmark::kMicrosecond);

static void BM_TrieSearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto pattern = setup_trie(text, n);
    TrieMatcher matcher(TRIE_INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_TrieSearch_Natural)->Range(1 << 10, 1 << 13)->Unit(benchmark::kMicrosecond);

// --- Arena trie search: random, repetitive, natural ---
// Same caps as naive trie.

static void BM_ArenaTrieSearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto pattern = setup_arena_trie(text, n);
    ArenaTrieMatcher matcher(ARENA_TRIE_INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_ArenaTrieSearch_Random)->Range(1 << 10, 1 << 13)->Unit(benchmark::kMicrosecond);

static void BM_ArenaTrieSearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto pattern = setup_arena_trie(text, n);
    ArenaTrieMatcher matcher(ARENA_TRIE_INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_ArenaTrieSearch_Repetitive)->Range(1 << 10, 1 << 13)->Unit(benchmark::kMicrosecond);

static void BM_ArenaTrieSearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto pattern = setup_arena_trie(text, n);
    ArenaTrieMatcher matcher(ARENA_TRIE_INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_ArenaTrieSearch_Natural)->Range(1 << 10, 1 << 13)->Unit(benchmark::kMicrosecond);

// --- Interpolation search on SA: random, repetitive, natural ---

static void BM_InterpolationSearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto pattern = setup_index(text, n);
    InterpolationMatcher matcher(CORPUS_PATH, INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_InterpolationSearch_Random)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_InterpolationSearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto pattern = setup_index(text, n);
    InterpolationMatcher matcher(CORPUS_PATH, INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_InterpolationSearch_Repetitive)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

static void BM_InterpolationSearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto pattern = setup_index(text, n);
    InterpolationMatcher matcher(CORPUS_PATH, INDEX_PATH);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_InterpolationSearch_Natural)->Range(1 << 10, 1 << 24)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
