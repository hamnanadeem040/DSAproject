/*
 * main_cli.cpp — FileZip CLI  (Huffman + LZW + RLE)
 *
 * Build:
 *   g++ -std=c++17 -O2 src/main_cli.cpp src/huffman.cpp src/lzw.cpp src/rle.cpp \
 *       -I include -o filezip
 *
 * Usage:
 *   ./filezip demo                            full 3-algorithm demo
 *   ./filezip compress   huffman input output
 *   ./filezip compress   lzw     input output
 *   ./filezip compress   rle     input output
 *   ./filezip decompress huffman input output
 */

#include "huffman.h"
#include "lzw.h"
#include "rle.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <chrono>

static void banner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════╗
║         FileZip  —  Multi-Algorithm Compressor       ║
║   Huffman Coding  |  LZW  |  Run-Length Encoding     ║
║              DSA Final Project  v2.0                 ║
╚══════════════════════════════════════════════════════╝
)" << '\n';
}

static void print_row(const std::string& label, const std::string& val) {
    std::cout << "  " << std::left << std::setw(22) << label << ": " << val << '\n';
}

static void print_stats_huff(const CompressStats& s, const std::string& op) {
    if (!s.success) { std::cerr << "  ERROR: " << s.error_msg << '\n'; return; }
    print_row("Operation",       op);
    print_row("Original size",   std::to_string(s.original_bytes)   + " bytes");
    print_row("Result size",     std::to_string(s.compressed_bytes) + " bytes");
    print_row("Space saved",     std::to_string((int)s.ratio_pct)   + " %");
    print_row("Time",            std::to_string((int)s.time_ms)     + " ms");
    print_row("Status",          "SUCCESS ✓");
}

static void print_stats_lzw(const LZWStats& s, const std::string& op) {
    if (!s.success) { std::cerr << "  ERROR: " << s.error_msg << '\n'; return; }
    print_row("Operation",       op);
    print_row("Original size",   std::to_string(s.original_bytes)   + " bytes");
    print_row("Result size",     std::to_string(s.compressed_bytes) + " bytes");
    print_row("Space saved",     std::to_string((int)s.ratio_pct)   + " %");
    print_row("Dictionary size", std::to_string(s.dict_size)        + " entries");
    print_row("Time",            std::to_string((int)s.time_ms)     + " ms");
    print_row("Status",          "SUCCESS ✓");
}

static void print_stats_rle(const RLEStats& s, const std::string& op) {
    if (!s.success) { std::cerr << "  ERROR: " << s.error_msg << '\n'; return; }
    print_row("Operation",       op);
    print_row("Original size",   std::to_string(s.original_bytes)   + " bytes");
    print_row("Result size",     std::to_string(s.compressed_bytes) + " bytes");
    print_row("Space saved",     std::to_string((int)s.ratio_pct)   + " %");
    print_row("Runs compressed", std::to_string(s.runs_found));
    print_row("Time",            std::to_string((int)s.time_ms)     + " ms");
    print_row("Status",          "SUCCESS ✓");
}

static bool verify(const std::string& a, const std::string& b) {
    std::ifstream fa(a, std::ios::binary), fb(b, std::ios::binary);
    std::string da((std::istreambuf_iterator<char>(fa)), {});
    std::string db((std::istreambuf_iterator<char>(fb)), {});
    return da == db;
}

static void sep(const std::string& title) {
    std::cout << "\n┌──────────────────────────────────────────────┐\n";
    std::cout << "│  " << std::left << std::setw(44) << title << "│\n";
    std::cout << "└──────────────────────────────────────────────┘\n";
}

static void demo() {
    // Create a mixed sample file
    const std::string text =
        "LZW excels at English text with repeated words and patterns.\n"
        "aaaaaaaabbbbbbbbbccccccccddddddddddeeeeeeeeeeeeffffggghh\n"
        "The quick brown fox jumps over the lazy dog. 0123456789\n"
        "Data Structures: Min-Heap, Binary Tree, Hash Table, Array.\n"
        "Compression algorithms reduce file size for storage & transfer.\n";

    std::ofstream f("demo_input.txt");
    for (int i = 0; i < 300; ++i) f << text;
    f.close();

    HuffmanCodec huff;
    LZWCodec     lzw;
    RLECodec     rle;

    // ── Huffman ──────────────────────────────────────────────────
    sep("ALGORITHM 1: Huffman Coding   O(n log n)");
    auto hs = huff.compress("demo_input.txt", "demo.huff");
    print_stats_huff(hs, "Compress");
    auto hd = huff.decompress("demo.huff", "demo_huff_out.txt");
    std::cout << "\n  Integrity check: "
              << (verify("demo_input.txt","demo_huff_out.txt") ? "✓ PASSED" : "✗ FAILED") << "\n";

    // ── LZW ──────────────────────────────────────────────────────
    sep("ALGORITHM 2: LZW   O(n)");
    auto ls = lzw.compress("demo_input.txt", "demo.lzw");
    print_stats_lzw(ls, "Compress");
    auto ld = lzw.decompress("demo.lzw", "demo_lzw_out.txt");
    std::cout << "\n  Integrity check: "
              << (verify("demo_input.txt","demo_lzw_out.txt") ? "✓ PASSED" : "✗ FAILED") << "\n";

    // ── RLE ──────────────────────────────────────────────────────
    sep("ALGORITHM 3: RLE   O(n)");
    auto rs = rle.compress("demo_input.txt", "demo.rle");
    print_stats_rle(rs, "Compress");
    auto rd = rle.decompress("demo.rle", "demo_rle_out.txt");
    std::cout << "\n  Integrity check: "
              << (verify("demo_input.txt","demo_rle_out.txt") ? "✓ PASSED" : "✗ FAILED") << "\n";

    // ── Comparison table ─────────────────────────────────────────
    sep("COMPARISON SUMMARY");
    std::cout << "\n  " << std::left
              << std::setw(12) << "Algorithm"
              << std::setw(14) << "Compressed"
              << std::setw(12) << "Ratio"
              << std::setw(12) << "Time"
              << "Complexity\n";
    std::cout << "  " << std::string(60, '-') << "\n";

    auto row = [](const std::string& a, size_t cb, double r, double t, const std::string& c) {
        std::cout << "  " << std::left
                  << std::setw(12) << a
                  << std::setw(14) << (std::to_string(cb) + " B")
                  << std::setw(12) << (std::to_string((int)r) + " %")
                  << std::setw(12) << (std::to_string((int)t) + " ms")
                  << c << "\n";
    };
    row("Huffman", hs.compressed_bytes, hs.ratio_pct, hs.time_ms, "O(n log n)");
    row("LZW",     ls.compressed_bytes, ls.ratio_pct, ls.time_ms, "O(n)");
    row("RLE",     rs.compressed_bytes, rs.ratio_pct, rs.time_ms, "O(n)");
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    banner();

    if (argc < 2 || std::string(argv[1]) == "demo") {
        demo();
        return 0;
    }

    std::string cmd  = argc > 1 ? argv[1] : "";
    std::string algo = argc > 2 ? argv[2] : "";
    std::string src  = argc > 3 ? argv[3] : "";
    std::string dst  = argc > 4 ? argv[4] : "";

    if (src.empty() || dst.empty()) {
        std::cerr << "Usage: filezip <compress|decompress> <huffman|lzw|rle> <input> <output>\n";
        return 1;
    }

    bool compress_mode = (cmd == "compress");

    if (algo == "huffman") {
        HuffmanCodec c;
        auto s = compress_mode ? c.compress(src, dst) : c.decompress(src, dst);
        print_stats_huff(s, compress_mode ? "Compress" : "Decompress");
        return s.success ? 0 : 1;
    } else if (algo == "lzw") {
        LZWCodec c;
        auto s = compress_mode ? c.compress(src, dst) : c.decompress(src, dst);
        print_stats_lzw(s, compress_mode ? "Compress" : "Decompress");
        return s.success ? 0 : 1;
    } else if (algo == "rle") {
        RLECodec c;
        auto s = compress_mode ? c.compress(src, dst) : c.decompress(src, dst);
        print_stats_rle(s, compress_mode ? "Compress" : "Decompress");
        return s.success ? 0 : 1;
    } else {
        std::cerr << "Unknown algorithm: " << algo << "  (use: huffman | lzw | rle)\n";
        return 1;
    }
}
