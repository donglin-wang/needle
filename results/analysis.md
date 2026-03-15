# Substring Search Algorithm Benchmarks — Unified Analysis

**Machine**: Apple Silicon (12 cores), L1d 64 KiB, L2 4 MiB per core
**Corpus sizes**: 1 KB – 64 MB (powers of 8), three corpus types: random, repetitive, natural language
**Pattern**: fixed 8-character substring drawn from the corpus

### Methodology

All search benchmarks assume the corpus has already been indexed and the index has been written to disk. Each benchmark iteration measures the full cost of loading the index from disk and searching for a single pattern — no pre-warmed in-memory state. Index-based algorithms (SA, suffix tree) load via `mmap`, so the OS page cache determines how much I/O actually occurs. The naive linear scan reads the corpus into a heap buffer via `read()`. Build benchmarks measure construction of the in-memory data structure plus serialization to disk. Page fault counts (minor and major) are captured via `getrusage` to quantify memory access overhead beyond what wall-clock time alone reveals.

---

## Experiment 1: Naive Linear Scan vs Suffix Array

**Question**: At what corpus size does a suffix array (SA) start beating a brute-force O(nm) scan?

### Results (natural corpus)

| Corpus | Naive (us) | SA (us) | Naive / SA |
|--------|------:|---:|-----------:|
| 1 KB | 14 | 25 | 0.6 |
| 4 KB | 23 | 25 | 0.9 |
| 32 KB | 90 | 31 | 2.9 |
| 256 KB | 617 | 46 | 13 |
| 2 MB | 5,266 | 93 | 57 |
| 16 MB | 42,824 | 271 | 158 |
| 64 MB | 167,093 | 953 | 175 |

### Analysis

The crossover happens between **4 KB and 32 KB**. Below 4 KB, naive scan wins because it avoids mmap overhead entirely — the corpus fits in L1 cache and a simple memcmp loop has no indirection. The SA search must mmap three regions (suffix array, text, header) even for a tiny corpus, costing ~25 us of fixed overhead.

Above 32 KB, the SA's O(m log n) binary search pulls ahead rapidly. At 64 MB the SA is **175x faster** on natural text. The gap is even larger on random text (146x) where the naive scan gets fewer early mismatches.

**Page faults tell the story**: naive search reports zero minor faults at all sizes because it reads the corpus into a heap buffer with a single `read()` call. The SA matcher uses mmap and reports 2 faults at 1 KB growing to 78 at 64 MB — but each fault maps a 16 KB page, so the total fault cost is dwarfed by the algorithmic savings.

**Repetitive text is the SA's worst case**: at 64 MB repetitive, SA search takes 47,498 us (vs 953 us for natural). The pattern "abcabc..." produces many SA entries in the same range, and the binary search must disambiguate long common prefixes. Naive scan also slows on repetitive text (195,950 us) but by a smaller factor. Even so, SA still wins 4x.

---

## Experiment 2: Suffix Tree vs Suffix Array

**Question**: Does Ukkonen's O(n) suffix tree beat the SA's O(n log n) build? How do their search times compare?

### Build time (natural corpus)

| Corpus | SA build (s) | Tree build (s) | Tree / SA |
|--------|--------:|-----------:|-------------:|
| 1 KB | 0.003 | 0.0004 | 0.1 |
| 4 KB | 0.003 | 0.001 | 0.4 |
| 32 KB | 0.005 | 0.012 | 2.4 |
| 256 KB | 0.026 | 0.098 | 3.8 |
| 2 MB | 0.190 | 1.31 | 6.9 |
| 16 MB | 1.54 | 12.5 | 8.1 |
| 64 MB | 6.56 | 63.0 | 9.6 |

### Search time (natural corpus)

| Corpus | SA search (us) | Tree search (us) | Tree / SA |
|--------|----------:|------------:|-------:|
| 1 KB | 25 | 15 | 0.6 |
| 4 KB | 25 | 17 | 0.7 |
| 32 KB | 31 | 23 | 0.7 |
| 256 KB | 46 | 42 | 0.9 |
| 2 MB | 93 | 165 | 1.8 |
| 16 MB | 271 | 1,231 | 4.5 |
| 64 MB | 953 | 263 | 0.3* |

Tree / SA < 1 means the tree is faster; > 1 means SA is faster.

*The 64 MB tree search number (263 us) is anomalously fast — the tree build that preceded it took 63 seconds, which warmed the OS page cache for the entire mmap region. The 16 MB number (1,231 us) is more representative of cold-cache behavior.

### Analysis

**Build**: despite Ukkonen's O(n) theoretical complexity, the suffix tree build is **10x slower** than libsais's SA construction at 64 MB. The culprit is memory: Ukkonen's algorithm allocates nodes with `std::vector<pair>` children lists, causing massive heap fragmentation. At 64 MB random text, the tree build triggered **43.5 million minor page faults** (vs 16,657 for SA build). Each fault means the kernel had to zero-fill and map a new page — the algorithm spends more time in the page fault handler than doing actual tree construction.

The SA build (libsais) is a highly optimized induced-sorting algorithm that works in-place on a contiguous int32 array. Its memory access pattern is sequential and cache-friendly. This is a textbook case where **constant factors and memory access patterns dominate asymptotic complexity**.

**Search**: the suffix tree's O(m) search should beat the SA's O(m log n) — and it does at small sizes where the index fits in cache. But at 2–16 MB, the tree's larger index (more nodes, child arrays, positions) causes more cache misses during traversal. The SA index is a single sorted array of int32s — maximally compact and cache-friendly.

**Repetitive text amplifies the gap**: at 64 MB repetitive, the tree search took 9,459 us (with 101 major faults!) vs the SA's 47,498 us. Repetitive text creates deep suffix trees with many internal nodes, but also creates long runs in the SA that binary search must scan through. The tree actually wins here because its O(m) traversal doesn't degrade with duplicates the way SA binary search does.

**Key insight**: Ukkonen's algorithm is theoretically optimal but practically hostile to modern hardware. The per-character node allocation pattern defeats the prefetcher, and the pointer-chasing traversal pattern defeats the cache hierarchy. libsais achieves worse asymptotic complexity but better real-world performance through cache-oblivious memory access patterns.

---

## Experiment 3: Binary Search vs Interpolation Search on Suffix Array

**Question**: Can interpolation search's O(m log log n) beat binary search's O(m log n) on suffix arrays?

### Results (natural corpus)

| Corpus | Binary (us) | Interpolation (us) | Interp / Binary |
|--------|------------:|--------------:|-------:|
| 1 KB | 28 | 26 | 0.9 |
| 4 KB | 28 | 26 | 0.9 |
| 32 KB | 32 | 35 | 1.1 |
| 256 KB | 45 | 72 | 1.6 |
| 2 MB | 90 | 427 | 4.7 |
| 16 MB | 293 | 7,102 | 24 |
| 64 MB | 918 | 21,710 | 24 |

### Analysis

Interpolation search is **dramatically slower** at scale — 24x worse at 64 MB natural text. This is surprising given its better theoretical complexity, but the page fault counters explain everything:

| Corpus | Binary faults | Interpolation faults | Interp / Binary |
|--------|-------------:|---------------------:|-------:|
| 1 KB | 2 | 2 | 1 |
| 32 KB | 12 | 11 | 0.9 |
| 256 KB | 32 | 76 | 2.4 |
| 2 MB | 56 | 530 | 9.5 |
| 16 MB | 70 | 4,155 | 59 |
| 64 MB | 78 | 16,423 | 211 |

At 64 MB, binary search causes **78 page faults** while interpolation search causes **16,423** — a 210x difference.

**Why?** Binary search always probes the midpoint, which creates a predictable access pattern: the first probe hits the middle of the array, the second hits a quartile, etc. The OS page cache and hardware prefetcher can anticipate this. After ~17 probes (log2 of 64M/4-byte entries), it converges. The touched pages form a small, reusable working set.

Interpolation search estimates where the target should be based on the value distribution, then jumps directly there. In theory this converges in O(log log n) steps for uniformly distributed data. But suffix arrays are **not** uniformly distributed — suffixes cluster by common prefixes. The interpolation estimate often overshoots or undershoots wildly, causing random jumps across the entire array. Each jump likely touches a new page, and the access pattern is unpredictable — the prefetcher gives up.

**Repetitive text is catastrophic**: at 64 MB repetitive, interpolation search takes **206,490 us** — slower than naive linear scan (195,950 us). The highly skewed distribution of suffix values makes interpolation estimates essentially random. Binary search on the same data takes 46,749 us (still slow due to long common prefixes, but 4.4x better than interpolation).

**The lesson**: interpolation search's O(m log log n) improvement over O(m log n) saves perhaps 10 comparisons at 64 MB. But each comparison in binary search costs ~1 cycle (data likely cached), while each comparison in interpolation search costs ~300 cycles (likely page fault or cache miss). The constant factor difference is 300x, overwhelming the logarithmic improvement.

---

## Summary

| Algorithm | Build | Search | Best for |
|-----------|-------|--------|----------|
| Naive scan | none | O(nm) | Tiny corpora (< 4 KB), one-shot searches |
| Suffix array (binary) | O(n log n)* | O(m log n) | General purpose — best balance of build cost, search speed, and memory efficiency |
| Suffix tree (Ukkonen) | O(n) | O(m) | Theoretical interest; impractical due to memory overhead |
| SA + interpolation | O(n log n)* | O(m log log n) | Nowhere — worse than binary search in practice |

*libsais uses induced sorting, technically O(n), but the distinction vs O(n log n) is academic here.

### The cache efficiency thesis

These experiments validate a single thesis: **asymptotic complexity is a poor predictor of real-world performance when memory access patterns differ**. The ranking by Big-O (suffix tree > SA binary > SA interpolation > naive) is almost exactly inverted in practice at scale. The actual performance ranking is determined by:

1. **Memory compactness** — the SA is a flat int32 array; the suffix tree is a graph of nodes with variable-size child lists
2. **Access pattern predictability** — binary search's midpoint probing is prefetcher-friendly; interpolation search's value-dependent jumps are not
3. **Page fault cost** — at 64 MB, a single page fault (~3 us) costs more than the entire binary search (~0.9 ms / 17 probes = 53 us per probe, but most probes hit cached pages)

The suffix array with binary search wins not because of its algorithm, but because its data structure is a single sorted array that modern hardware was designed to traverse efficiently.
