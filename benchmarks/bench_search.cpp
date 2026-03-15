#include <benchmark/benchmark.h>

#include "../src/matcher.hpp"
#include "../src/naive_matcher.hpp"
#include "../src/interpolation_matcher.hpp"
#include "../src/suffix_array_indexer.hpp"
#include "../src/suffix_tree_indexer.hpp"
#include "../src/suffix_tree_matcher.hpp"
#include "corpus_generator.hpp"

#include <chrono>
#include <string>
#include <sys/resource.h>

using Clock = std::chrono::high_resolution_clock;

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

static double us_between(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double, std::micro>(b - a).count();
}

static const std::string CORPUS_PATH           = "/tmp/needle_bench_corpus.bin";
static const std::string INDEX_PATH            = "/tmp/needle_bench_index.bin";
static const std::string SUFFIX_TREE_INDEX_PATH = "/tmp/needle_bench_suffix_tree_index.bin";

static std::vector<int32_t> codepoints(const std::string &s)
{
    return {s.begin(), s.end()};
}

static std::vector<int32_t> mid_pattern(const std::string &text, size_t n)
{
    return codepoints(text.substr(n / 2, 8));
}

// ============================================================================
// SA — build and search
// Build: construct SA from codepoints + serialize (corpus + index) to disk.
// Search: mmap + binary search (single combined duration — mmap is lazy).
// ============================================================================

static void BM_SABuild_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto cp = codepoints(text);
    double construct_us = 0, serialize_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto input = cp;
        auto t0 = Clock::now();
        SuffixArrayIndexer indexer(std::move(input));
        auto t1 = Clock::now();
        save_corpus(cp, CORPUS_PATH);
        indexer.save_index(INDEX_PATH);
        auto t2 = Clock::now();
        construct_us += us_between(t0, t1);
        serialize_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["construct_us"] = benchmark::Counter(construct_us, benchmark::Counter::kAvgIterations);
    state.counters["serialize_us"] = benchmark::Counter(serialize_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SABuild_Random)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMillisecond);

static void BM_SABuild_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto cp = codepoints(text);
    double construct_us = 0, serialize_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto input = cp;
        auto t0 = Clock::now();
        SuffixArrayIndexer indexer(std::move(input));
        auto t1 = Clock::now();
        save_corpus(cp, CORPUS_PATH);
        indexer.save_index(INDEX_PATH);
        auto t2 = Clock::now();
        construct_us += us_between(t0, t1);
        serialize_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["construct_us"] = benchmark::Counter(construct_us, benchmark::Counter::kAvgIterations);
    state.counters["serialize_us"] = benchmark::Counter(serialize_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SABuild_Repetitive)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMillisecond);

static void BM_SABuild_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto cp = codepoints(text);
    double construct_us = 0, serialize_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto input = cp;
        auto t0 = Clock::now();
        SuffixArrayIndexer indexer(std::move(input));
        auto t1 = Clock::now();
        save_corpus(cp, CORPUS_PATH);
        indexer.save_index(INDEX_PATH);
        auto t2 = Clock::now();
        construct_us += us_between(t0, t1);
        serialize_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["construct_us"] = benchmark::Counter(construct_us, benchmark::Counter::kAvgIterations);
    state.counters["serialize_us"] = benchmark::Counter(serialize_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SABuild_Natural)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMillisecond);

static void BM_SASearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto cp = codepoints(text);
    save_corpus(cp, CORPUS_PATH);
    SuffixArrayIndexer indexer(std::move(cp));
    indexer.save_index(INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        Matcher matcher(CORPUS_PATH, INDEX_PATH);
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SASearch_Random)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

static void BM_SASearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto cp = codepoints(text);
    save_corpus(cp, CORPUS_PATH);
    SuffixArrayIndexer indexer(std::move(cp));
    indexer.save_index(INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        Matcher matcher(CORPUS_PATH, INDEX_PATH);
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SASearch_Repetitive)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

static void BM_SASearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto cp = codepoints(text);
    save_corpus(cp, CORPUS_PATH);
    SuffixArrayIndexer indexer(std::move(cp));
    indexer.save_index(INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        Matcher matcher(CORPUS_PATH, INDEX_PATH);
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SASearch_Natural)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Naive — no index to build; search = load corpus from disk + linear scan.
// Two sub-durations: load_us and search_us.
// ============================================================================

static void BM_NaiveSearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    save_corpus(codepoints(text), CORPUS_PATH);
    auto pattern = mid_pattern(text, n);
    double load_us = 0, search_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        NaiveMatcher matcher(CORPUS_PATH);
        auto t1 = Clock::now();
        auto results = matcher.search(pattern);
        auto t2 = Clock::now();
        benchmark::DoNotOptimize(results.data());
        load_us += us_between(t0, t1);
        search_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["load_us"] = benchmark::Counter(load_us, benchmark::Counter::kAvgIterations);
    state.counters["search_us"] = benchmark::Counter(search_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Random)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

static void BM_NaiveSearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    save_corpus(codepoints(text), CORPUS_PATH);
    auto pattern = mid_pattern(text, n);
    double load_us = 0, search_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        NaiveMatcher matcher(CORPUS_PATH);
        auto t1 = Clock::now();
        auto results = matcher.search(pattern);
        auto t2 = Clock::now();
        benchmark::DoNotOptimize(results.data());
        load_us += us_between(t0, t1);
        search_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["load_us"] = benchmark::Counter(load_us, benchmark::Counter::kAvgIterations);
    state.counters["search_us"] = benchmark::Counter(search_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Repetitive)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

static void BM_NaiveSearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    save_corpus(codepoints(text), CORPUS_PATH);
    auto pattern = mid_pattern(text, n);
    double load_us = 0, search_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        NaiveMatcher matcher(CORPUS_PATH);
        auto t1 = Clock::now();
        auto results = matcher.search(pattern);
        auto t2 = Clock::now();
        benchmark::DoNotOptimize(results.data());
        load_us += us_between(t0, t1);
        search_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["load_us"] = benchmark::Counter(load_us, benchmark::Counter::kAvgIterations);
    state.counters["search_us"] = benchmark::Counter(search_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_NaiveSearch_Natural)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Suffix Tree — Ukkonen's O(n) construction, mmap search.
// Build: construct suffix tree + serialize flat arrays to disk.
// Search: mmap + compressed edge traversal. Two sub-durations.
// O(n) build allows full range (1KB–16MB).
// ============================================================================

static void BM_SuffixTreeBuild_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto cp = codepoints(text);
    double construct_us = 0, serialize_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        SuffixTreeIndexer indexer(cp);
        auto t1 = Clock::now();
        indexer.save_index(SUFFIX_TREE_INDEX_PATH);
        auto t2 = Clock::now();
        construct_us += us_between(t0, t1);
        serialize_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["construct_us"] = benchmark::Counter(construct_us, benchmark::Counter::kAvgIterations);
    state.counters["serialize_us"] = benchmark::Counter(serialize_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SuffixTreeBuild_Random)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMillisecond);

static void BM_SuffixTreeBuild_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto cp = codepoints(text);
    double construct_us = 0, serialize_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        SuffixTreeIndexer indexer(cp);
        auto t1 = Clock::now();
        indexer.save_index(SUFFIX_TREE_INDEX_PATH);
        auto t2 = Clock::now();
        construct_us += us_between(t0, t1);
        serialize_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["construct_us"] = benchmark::Counter(construct_us, benchmark::Counter::kAvgIterations);
    state.counters["serialize_us"] = benchmark::Counter(serialize_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SuffixTreeBuild_Repetitive)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMillisecond);

static void BM_SuffixTreeBuild_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto cp = codepoints(text);
    double construct_us = 0, serialize_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        SuffixTreeIndexer indexer(cp);
        auto t1 = Clock::now();
        indexer.save_index(SUFFIX_TREE_INDEX_PATH);
        auto t2 = Clock::now();
        construct_us += us_between(t0, t1);
        serialize_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["construct_us"] = benchmark::Counter(construct_us, benchmark::Counter::kAvgIterations);
    state.counters["serialize_us"] = benchmark::Counter(serialize_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SuffixTreeBuild_Natural)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMillisecond);

static void BM_SuffixTreeSearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto cp = codepoints(text);
    SuffixTreeIndexer indexer(cp);
    indexer.save_index(SUFFIX_TREE_INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    double load_us = 0, search_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        SuffixTreeMatcher matcher(SUFFIX_TREE_INDEX_PATH);
        auto t1 = Clock::now();
        auto results = matcher.search(pattern);
        auto t2 = Clock::now();
        benchmark::DoNotOptimize(results.data());
        load_us += us_between(t0, t1);
        search_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["load_us"] = benchmark::Counter(load_us, benchmark::Counter::kAvgIterations);
    state.counters["search_us"] = benchmark::Counter(search_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SuffixTreeSearch_Random)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

static void BM_SuffixTreeSearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto cp = codepoints(text);
    SuffixTreeIndexer indexer(cp);
    indexer.save_index(SUFFIX_TREE_INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    double load_us = 0, search_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        SuffixTreeMatcher matcher(SUFFIX_TREE_INDEX_PATH);
        auto t1 = Clock::now();
        auto results = matcher.search(pattern);
        auto t2 = Clock::now();
        benchmark::DoNotOptimize(results.data());
        load_us += us_between(t0, t1);
        search_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["load_us"] = benchmark::Counter(load_us, benchmark::Counter::kAvgIterations);
    state.counters["search_us"] = benchmark::Counter(search_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SuffixTreeSearch_Repetitive)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

static void BM_SuffixTreeSearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto cp = codepoints(text);
    SuffixTreeIndexer indexer(cp);
    indexer.save_index(SUFFIX_TREE_INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    double load_us = 0, search_us = 0;
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        auto t0 = Clock::now();
        SuffixTreeMatcher matcher(SUFFIX_TREE_INDEX_PATH);
        auto t1 = Clock::now();
        auto results = matcher.search(pattern);
        auto t2 = Clock::now();
        benchmark::DoNotOptimize(results.data());
        load_us += us_between(t0, t1);
        search_us += us_between(t1, t2);
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.counters["load_us"] = benchmark::Counter(load_us, benchmark::Counter::kAvgIterations);
    state.counters["search_us"] = benchmark::Counter(search_us, benchmark::Counter::kAvgIterations);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_SuffixTreeSearch_Natural)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Interpolation — uses same SA index as Matcher.
// Build: same as SA (not duplicated — filter with BM_SABuild).
// Search: mmap + interpolation search (single combined duration).
// ============================================================================

static void BM_InterpolationSearch_Random(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = random_corpus(n);
    auto cp = codepoints(text);
    save_corpus(cp, CORPUS_PATH);
    SuffixArrayIndexer indexer(std::move(cp));
    indexer.save_index(INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        InterpolationMatcher matcher(CORPUS_PATH, INDEX_PATH);
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_InterpolationSearch_Random)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

static void BM_InterpolationSearch_Repetitive(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = repetitive_corpus(n);
    auto cp = codepoints(text);
    save_corpus(cp, CORPUS_PATH);
    SuffixArrayIndexer indexer(std::move(cp));
    indexer.save_index(INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        InterpolationMatcher matcher(CORPUS_PATH, INDEX_PATH);
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_InterpolationSearch_Repetitive)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

static void BM_InterpolationSearch_Natural(benchmark::State &state)
{
    const size_t n = state.range(0);
    std::string text = natural_corpus(n);
    auto cp = codepoints(text);
    save_corpus(cp, CORPUS_PATH);
    SuffixArrayIndexer indexer(std::move(cp));
    indexer.save_index(INDEX_PATH);
    auto pattern = mid_pattern(text, n);
    auto before = snapshot_rusage();
    for (auto _ : state)
    {
        InterpolationMatcher matcher(CORPUS_PATH, INDEX_PATH);
        auto results = matcher.search(pattern);
        benchmark::DoNotOptimize(results.data());
    }
    auto after = snapshot_rusage();
    report_page_faults(state, before, after);
    state.SetBytesProcessed(state.iterations() * n);
}
BENCHMARK(BM_InterpolationSearch_Natural)->Range(1 << 10, 1 << 26)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
