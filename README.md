# Needle

Implementations of four substring search algorithms, benchmarked to study how memory access patterns and hardware cache behaviour diverge from theoretical complexity.

## Question

Big-O complexity counts operations, not memory accesses. A cache miss costs ~100–300 cycles; a register operation costs ~1. This project asks: when algorithms differ in their memory access patterns, does the hardware-aware algorithm beat the theoretically superior one?

## Algorithms compared

| Algorithm | Build | Search | Notes |
|-----------|-------|--------|-------|
| Naive linear scan | none | O(nm) | Heap-resident corpus, `memcmp` scan |
| Suffix array (binary search) | O(n) | O(m log n) | `mmap`-based, flat `int32_t` array, built with libsais (SA-IS) |
| Suffix array (interpolation search) | O(n) | O(m log log n) | Same index, value-driven probe position |
| Suffix tree (Ukkonen) | O(n) | O(m) | `mmap`-based, compressed edge representation |

All search benchmarks measure the full cost of loading the index from disk and searching — no pre-warmed in-memory state. Page fault counts are recorded via `getrusage` alongside wall-clock time.

## Key findings

**Naive scan vs suffix array.** The crossover point is between 4 KB and 32 KB. Below 4 KB, naive scan wins because the corpus fits in L1 cache and a `memcmp` loop has no indirection. Above 32 KB, the SA pulls ahead rapidly. At 64 MB natural text, the SA is **175× faster**.

**Suffix tree vs suffix array.** Despite Ukkonen's O(n) build complexity, the suffix tree build is **10× slower** than libsais at 64 MB. The tree triggered 43.5 million minor page faults during construction vs 16,657 for the SA — each fault requiring the kernel to zero-fill and map a new page. The tree's per-character node allocation defeats the hardware prefetcher. The SA build operates on a single contiguous `int32_t` array with sequential access; the tree does not.

**Binary search vs interpolation search.** Interpolation search's O(m log log n) saves roughly 10 comparisons at 64 MB. Each binary search comparison costs ~1 cycle (data typically cached); each interpolation search comparison costs ~300 cycles (cache miss from value-driven random access). At 64 MB, binary search causes 78 page faults; interpolation search causes 16,423 — a 211× difference. Interpolation search is **24× slower** at 64 MB, and slower than naive linear scan on repetitive text.

Full results and analysis: [`results/analysis.md`](results/analysis.md)

## Implementation

The corpus is decoded from UTF-8 to `int32_t` codepoints at index time. All algorithms operate on codepoint arrays, making search Unicode-correct. Codepoints are `int32_t` throughout (not `uint32_t`) — safe because Unicode is capped at U+10FFFF, and libsais requires signed input.

The suffix array and suffix tree indexes are serialised to disk as flat binary files and loaded via `mmap`. Searches touch only the pages required by the access pattern, with the OS page cache determining how much I/O actually occurs. Benchmark iterations measure this cold-load cost explicitly rather than hiding it.

## Build

Requires CMake 3.20+ and a C++20 compiler. Dependencies (libsais, GoogleTest, Google Benchmark) are fetched automatically.

```bash
# Build the CLI
make build

# Run tests
make test

# Run benchmarks
make bench
```

## CLI

```bash
# Index a UTF-8 text file
needle index corpus.txt corpus.bin index.bin

# Search the index
needle search corpus.bin index.bin "pattern"
```

## Structure

```
src/
  utf8_reader.hpp          — streaming UTF-8 → int32_t decoder with buffer carry
  suffix_array_indexer.hpp — wraps libsais, builds and serialises suffix array
  suffix_tree_indexer.hpp  — Ukkonen's algorithm, serialises to mmap-friendly format
  matcher.hpp              — mmap suffix array search (binary)
  interpolation_matcher.hpp — mmap suffix array search (interpolation)
  suffix_tree_matcher.hpp  — mmap suffix tree search
  naive_matcher.hpp        — heap-resident linear scan
  mapped_file.hpp          — RAII mmap wrapper
benchmarks/
  bench_search.cpp         — Google Benchmark suite with getrusage page fault counters
  corpus_generator.hpp     — random, repetitive, and natural language corpus generators
results/
  analysis.md              — full experimental write-up
tests/                     — GoogleTest suites
```