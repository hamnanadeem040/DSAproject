/*
 * huffman.cpp  –  Full Huffman Coding implementation
 *
 * Data Structures used
 * ────────────────────
 *   • Min-Heap (std::priority_queue)  → always pop lowest-frequency node
 *   • Binary Tree (shared_ptr nodes)  → Huffman tree structure
 *   • Hash Table (unordered_map)      → char → frequency, char → bit-code
 *   • std::vector<unsigned char>      → raw byte buffers
 *   • std::string (bit string)        → variable-length codes during encoding
 *
 * File format of a .huff file
 * ────────────────────────────
 *   [4 bytes]  Magic number  0x48 0x55 0x46 0x46  ("HUFF")
 *   [4 bytes]  Original file size (little-endian uint32_t)
 *   [4 bytes]  Tree serialisation length in bytes (uint32_t)
 *   [N bytes]  Serialised Huffman tree  (pre-order, see below)
 *   [1 byte ]  Padding bits count (0-7) added at end of bit-stream
 *   [M bytes]  Compressed bit-stream (last byte may be padded)
 *
 * Tree serialisation (pre-order walk)
 *   Internal node → byte 0x00
 *   Leaf node     → byte 0x01 followed by the character byte
 */

#include "huffman.h"
#include <chrono>
#include <stdexcept>
#include <cstring>
#include <iostream>

// ════════════════════════════════════════════════════════════════════
//  FILE I/O
// ════════════════════════════════════════════════════════════════════
std::vector<unsigned char>
HuffmanCodec::read_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    return { std::istreambuf_iterator<char>(f),
             std::istreambuf_iterator<char>() };
}

// ════════════════════════════════════════════════════════════════════
//  STEP 1  –  Frequency Table   O(n)
// ════════════════════════════════════════════════════════════════════
std::unordered_map<unsigned char, int>
HuffmanCodec::build_frequency_table(const std::vector<unsigned char>& data)
{
    std::unordered_map<unsigned char, int> freq;
    for (unsigned char byte : data) freq[byte]++;
    return freq;
}

// ════════════════════════════════════════════════════════════════════
//  STEP 2  –  Build Huffman Tree   O(n log n)  via Min-Heap
// ════════════════════════════════════════════════════════════════════
std::shared_ptr<HuffNode>
HuffmanCodec::build_tree(const std::unordered_map<unsigned char, int>& freq)
{
    // Push every unique character as a leaf onto the min-heap
    std::priority_queue<
        std::shared_ptr<HuffNode>,
        std::vector<std::shared_ptr<HuffNode>>,
        NodeCmp> pq;

    for (auto& [ch, f] : freq)
        pq.push(std::make_shared<HuffNode>(ch, f));

    // Edge case: single unique character
    if (pq.size() == 1) {
        auto only = pq.top(); pq.pop();
        auto root = std::make_shared<HuffNode>(only, std::make_shared<HuffNode>(only->ch, 0));
        return root;
    }

    // Greedy merge: always combine the two smallest-frequency nodes
    while (pq.size() > 1) {
        auto left  = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        pq.push(std::make_shared<HuffNode>(left, right));
    }
    return pq.top();
}

// ════════════════════════════════════════════════════════════════════
//  STEP 3  –  Generate Codes  (DFS on tree)   O(n)
// ════════════════════════════════════════════════════════════════════
void HuffmanCodec::build_codes(const std::shared_ptr<HuffNode>& node,
                                const std::string& prefix,
                                std::unordered_map<unsigned char, std::string>& codes)
{
    if (!node) return;
    if (node->is_leaf) {
        codes[node->ch] = prefix.empty() ? "0" : prefix;   // handle 1-symbol files
        return;
    }
    build_codes(node->left,  prefix + "0", codes);
    build_codes(node->right, prefix + "1", codes);
}

// ════════════════════════════════════════════════════════════════════
//  TREE SERIALISATION  (pre-order, compact binary)
// ════════════════════════════════════════════════════════════════════
void HuffmanCodec::serialise_tree(const std::shared_ptr<HuffNode>& node,
                                   std::vector<unsigned char>& buf)
{
    if (!node) return;
    if (node->is_leaf) {
        buf.push_back(0x01);
        buf.push_back(node->ch);
    } else {
        buf.push_back(0x00);
        serialise_tree(node->left,  buf);
        serialise_tree(node->right, buf);
    }
}

std::shared_ptr<HuffNode>
HuffmanCodec::deserialise_tree(const std::vector<unsigned char>& buf, size_t& pos)
{
    if (pos >= buf.size())
        throw std::runtime_error("Corrupt tree data");

    unsigned char tag = buf[pos++];
    if (tag == 0x01) {
        unsigned char ch = buf[pos++];
        return std::make_shared<HuffNode>(ch, 0);
    }
    // internal node
    auto left  = deserialise_tree(buf, pos);
    auto right = deserialise_tree(buf, pos);
    return std::make_shared<HuffNode>(left, right);
}

// ════════════════════════════════════════════════════════════════════
//  BIT-STREAM HELPERS
// ════════════════════════════════════════════════════════════════════
void HuffmanCodec::write_bits(std::ofstream& out,
                               const std::string& bitstr,
                               unsigned char& cur_byte,
                               int& bit_count)
{
    for (char b : bitstr) {
        cur_byte = (cur_byte << 1) | (b == '1' ? 1 : 0);
        bit_count++;
        if (bit_count == 8) {
            out.put(static_cast<char>(cur_byte));
            cur_byte  = 0;
            bit_count = 0;
        }
    }
}

void HuffmanCodec::flush_bits(std::ofstream& out,
                               unsigned char cur_byte,
                               int bit_count)
{
    if (bit_count > 0) {
        // left-align remaining bits (right-pad with 0s)
        cur_byte <<= (8 - bit_count);
        out.put(static_cast<char>(cur_byte));
    }
}

// ════════════════════════════════════════════════════════════════════
//  COMPRESS
// ════════════════════════════════════════════════════════════════════
CompressStats HuffmanCodec::compress(const std::string& src_path,
                                      const std::string& dst_path)
{
    CompressStats stats;
    auto t0 = std::chrono::high_resolution_clock::now();

    try {
        // 1. Read input
        auto data = read_file(src_path);
        if (data.empty()) throw std::runtime_error("Source file is empty");
        stats.original_bytes = data.size();

        // 2. Frequency table
        auto freq  = build_frequency_table(data);

        // 3. Huffman tree
        auto root  = build_tree(freq);

        // 4. Code table
        std::unordered_map<unsigned char, std::string> codes;
        build_codes(root, "", codes);

        // 5. Serialise tree
        std::vector<unsigned char> tree_buf;
        serialise_tree(root, tree_buf);

        // 6. Build full bit-string then write
        std::ofstream out(dst_path, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot create output file: " + dst_path);

        // --- Header ---
        const unsigned char magic[4] = {0x48, 0x55, 0x46, 0x46};   // "HUFF"
        out.write(reinterpret_cast<const char*>(magic), 4);

        uint32_t orig_size  = static_cast<uint32_t>(data.size());
        uint32_t tree_size  = static_cast<uint32_t>(tree_buf.size());
        out.write(reinterpret_cast<const char*>(&orig_size),  4);
        out.write(reinterpret_cast<const char*>(&tree_size),  4);
        out.write(reinterpret_cast<const char*>(tree_buf.data()), tree_size);

        // Count padding bits needed (we'll fill in after encoding)
        // Reserve 1 byte for padding count
        std::streampos pad_pos = out.tellp();
        out.put(0x00);   // placeholder for padding byte count

        // --- Encode ---
        unsigned char cur_byte = 0;
        int           bit_cnt  = 0;

        for (unsigned char c : data)
            write_bits(out, codes.at(c), cur_byte, bit_cnt);

        unsigned char padding = (bit_cnt == 0) ? 0 : static_cast<unsigned char>(8 - bit_cnt);
        flush_bits(out, cur_byte, bit_cnt);

        // Go back and write actual padding count
        out.seekp(pad_pos);
        out.put(static_cast<char>(padding));
        out.close();

        // --- Stats ---
        stats.compressed_bytes = static_cast<size_t>(
            std::ifstream(dst_path, std::ios::ate | std::ios::binary).tellg());
        stats.ratio_pct = 100.0 * (1.0 - static_cast<double>(stats.compressed_bytes)
                                            / stats.original_bytes);
        stats.success = true;

    } catch (const std::exception& e) {
        stats.error_msg = e.what();
        stats.success   = false;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    stats.time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return stats;
}

// ════════════════════════════════════════════════════════════════════
//  DECOMPRESS
// ════════════════════════════════════════════════════════════════════
CompressStats HuffmanCodec::decompress(const std::string& src_path,
                                        const std::string& dst_path)
{
    CompressStats stats;
    auto t0 = std::chrono::high_resolution_clock::now();

    try {
        auto data = read_file(src_path);
        stats.original_bytes = data.size();

        size_t pos = 0;

        // Verify magic
        if (data.size() < 13 ||
            data[0] != 0x48 || data[1] != 0x55 ||
            data[2] != 0x46 || data[3] != 0x46)
            throw std::runtime_error("Not a valid .huff file");
        pos = 4;

        // Read orig size
        uint32_t orig_size = 0;
        std::memcpy(&orig_size, &data[pos], 4); pos += 4;

        // Read tree size + deserialise
        uint32_t tree_size = 0;
        std::memcpy(&tree_size, &data[pos], 4); pos += 4;

        std::vector<unsigned char> tree_buf(data.begin() + pos,
                                             data.begin() + pos + tree_size);
        pos += tree_size;

        size_t tp = 0;
        auto root = deserialise_tree(tree_buf, tp);

        // Padding count
        unsigned char padding = data[pos++];

        // Decode bit-stream
        std::vector<unsigned char> output;
        output.reserve(orig_size);

        auto cur = root;
        size_t total_bits = (data.size() - pos) * 8 - padding;
        size_t bits_read  = 0;

        for (size_t i = pos; i < data.size() && bits_read < total_bits; ++i) {
            unsigned char byte = data[i];
            for (int bit = 7; bit >= 0 && bits_read < total_bits; --bit, ++bits_read) {
                bool go_right = (byte >> bit) & 1;
                cur = go_right ? cur->right : cur->left;

                if (cur->is_leaf) {
                    output.push_back(cur->ch);
                    cur = root;
                    if (output.size() == orig_size) break;
                }
            }
        }

        // Write output
        std::ofstream out(dst_path, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot create output file: " + dst_path);
        out.write(reinterpret_cast<const char*>(output.data()), output.size());
        out.close();

        stats.compressed_bytes = output.size();
        stats.ratio_pct        = 100.0 * (1.0 - static_cast<double>(stats.original_bytes)
                                                / stats.compressed_bytes);
        stats.success = true;

    } catch (const std::exception& e) {
        stats.error_msg = e.what();
        stats.success   = false;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    stats.time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return stats;
}
