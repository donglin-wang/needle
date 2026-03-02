#include <iostream>
#include <string>

#include <re2/re2.h>
#include <libsais.h>

int main() {
    // ── RE2 ───────────────────────────────────────────────────────────────────
    // FullMatch
    if (!re2::RE2::FullMatch("needle-0.1.0", R"(needle-\d+\.\d+\.\d+)")) {
        std::cerr << "[re2] FullMatch FAILED\n";
        return 1;
    }
    std::cout << "[re2]  FullMatch: OK\n";

    // FindAndConsume with capture group — extract all integers
    re2::RE2 digits(R"((\d+))");
    std::string input = "line 1: 42 hits, line 2: 7 hits";
    re2::StringPiece sp(input);
    std::string num;
    std::cout << "[re2]  Numbers :";
    while (re2::RE2::FindAndConsume(&sp, digits, &num)) {
        std::cout << " " << num;
    }
    std::cout << "\n";

    // PartialMatch with named group
    std::string version;
    if (!re2::RE2::PartialMatch("needle-0.1.0", R"(needle-(\S+))", &version)) {
        std::cerr << "[re2] PartialMatch FAILED\n";
        return 1;
    }
    std::cout << "[re2]  Version : " << version << "\n";

    // ── libsais ───────────────────────────────────────────────────────────────
    // Suffix array of "banana": expected SA = {5,3,1,0,4,2}
    //   "a", "ana", "anana", "banana", "na", "nana"
    const uint8_t text[] = "banana";
    const int32_t n = 6;
    int32_t sa[6];
    if (libsais(text, sa, n, 0, nullptr) != 0) {
        std::cerr << "[libsais] SA construction FAILED\n";
        return 1;
    }
    const int32_t expected[] = {5, 3, 1, 0, 4, 2};
    for (int i = 0; i < n; ++i) {
        if (sa[i] != expected[i]) {
            std::cerr << "[libsais] SA mismatch at " << i << "\n";
            return 1;
        }
    }
    std::cout << "[libsais] SA(\"banana\"): OK\n";

    std::cout << "\nAll checks passed.\n";
    return 0;
}
