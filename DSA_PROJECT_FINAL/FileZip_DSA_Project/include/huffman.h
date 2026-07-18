#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>
#include <fstream>
#include <stdexcept>

// ─────────────────────────────────────────────
//  Huffman Tree Node
//  Each leaf = one character with its frequency
//  Each internal node = sum of children freqs
// ─────────────────────────────────────────────
struct HuffNode {
    unsigned char ch;       // character (meaningful only at leaves)
    int           freq;     // frequency / combined weight
    bool          is_leaf;
    std::shared_ptr<HuffNode> left, right;

    // Leaf constructor
    HuffNode(unsigned char c, int f)
        : ch(c), freq(f), is_leaf(true), left(nullptr), right(nullptr) {}

    // Internal node constructor
    HuffNode(std::shared_ptr<HuffNode> l, std::shared_ptr<HuffNode> r)
        : ch(0), freq(l->freq + r->freq), is_leaf(false),
          left(l), right(r) {}
};

// ─────────────────────────────────────────────
//  Min-heap comparator  (lower freq = higher priority)
// ─────────────────────────────────────────────
struct NodeCmp {
    bool operator()(const std::shared_ptr<HuffNode>& a,
                    const std::shared_ptr<HuffNode>& b) const {
        return a->freq > b->freq;          // min-heap
    }
};

// ─────────────────────────────────────────────
//  Compression Statistics
// ─────────────────────────────────────────────
struct CompressStats {
    size_t  original_bytes   = 0;
    size_t  compressed_bytes = 0;
    double  ratio_pct        = 0.0;   // e.g. 45.3  means 45.3 % smaller
    double  time_ms          = 0.0;
    std::string error_msg;
    bool    success          = false;
};

// ─────────────────────────────────────────────
//  HuffmanCodec  –  main class
// ─────────────────────────────────────────────
class HuffmanCodec {
public:
    // Compress src → dst.  Returns stats.
    CompressStats compress(const std::string& src_path,
                           const std::string& dst_path);

    // Decompress src → dst.  Returns stats.
    CompressStats decompress(const std::string& src_path,
                             const std::string& dst_path);

private:
    // --- build phase ---
    std::unordered_map<unsigned char, int>
        build_frequency_table(const std::vector<unsigned char>& data);

    std::shared_ptr<HuffNode>
        build_tree(const std::unordered_map<unsigned char, int>& freq);

    void build_codes(const std::shared_ptr<HuffNode>& node,
                     const std::string& prefix,
                     std::unordered_map<unsigned char, std::string>& codes);

    // --- serialisation (tree stored in compressed file header) ---
    void  serialise_tree(const std::shared_ptr<HuffNode>& node,
                         std::vector<unsigned char>& buf);

    std::shared_ptr<HuffNode>
        deserialise_tree(const std::vector<unsigned char>& buf, size_t& pos);

    // --- bit-stream helpers ---
    void  write_bits(std::ofstream& out,
                     const std::string& bitstr,
                     unsigned char& cur_byte,
                     int& bit_count);

    void  flush_bits(std::ofstream& out,
                     unsigned char cur_byte,
                     int bit_count);

    // --- file I/O ---
    std::vector<unsigned char> read_file(const std::string& path);
};
