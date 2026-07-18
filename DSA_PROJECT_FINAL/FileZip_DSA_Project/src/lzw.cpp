/*
 * lzw.cpp — LZW (Lempel-Ziv-Welch) Compression
 *
 * Algorithm Steps:
 * ─────────────────
 * ENCODE:
 *   1. Init dictionary: single-byte strings 0x00–0xFF (256 entries)
 *   2. Read symbol. If (current + symbol) in dict → extend current string
 *   3. Else → emit code for current, add (current + symbol) to dict, reset current
 *   4. Emit final code
 *
 * DECODE:
 *   1. Init same 256-entry dictionary
 *   2. For each code: look up string, output it, add new entry = prev_string + string[0]
 *
 * Complexity: O(n) encode and decode   (hash table gives O(1) lookup)
 */

#include "lzw.h"
#include <chrono>
#include <cstring>
#include <iostream>

std::vector<unsigned char> LZWCodec::read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open: " + path);
    return { std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>() };
}

// ── ENCODE ────────────────────────────────────────────────────────
LZWStats LZWCodec::compress(const std::string& src, const std::string& dst) {
    LZWStats stats;
    auto t0 = std::chrono::high_resolution_clock::now();

    try {
        auto data = read_file(src);
        if (data.empty()) throw std::runtime_error("Empty file");
        stats.original_bytes = data.size();

        // ── Build initial 256-entry dictionary ─────────────────────
        std::unordered_map<std::string, int> dict;
        dict.reserve(8192);
        for (int i = 0; i < INIT_SIZE; ++i)
            dict[std::string(1, static_cast<char>(i))] = i;
        int next_code = INIT_SIZE;

        // ── Encode ─────────────────────────────────────────────────
        std::vector<int> codes;
        codes.reserve(data.size());

        std::string cur(1, static_cast<char>(data[0]));
        for (size_t i = 1; i < data.size(); ++i) {
            std::string next = cur + static_cast<char>(data[i]);
            if (dict.count(next)) {
                cur = std::move(next);
            } else {
                codes.push_back(dict.at(cur));
                if (next_code <= MAX_CODE) {
                    dict[next] = next_code++;
                }
                cur = std::string(1, static_cast<char>(data[i]));
            }
        }
        codes.push_back(dict.at(cur));   // flush last

        stats.dict_size = static_cast<size_t>(next_code);

        // ── Write file ─────────────────────────────────────────────
        std::ofstream out(dst, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot create: " + dst);

        // Header
        const char magic[4] = {'L','Z','W','F'};
        out.write(magic, 4);
        uint32_t orig = static_cast<uint32_t>(data.size());
        uint32_t cnt  = static_cast<uint32_t>(codes.size());
        out.write(reinterpret_cast<const char*>(&orig), 4);
        out.write(reinterpret_cast<const char*>(&cnt),  4);

        // Codes as 3-byte (24-bit) little-endian values
        for (int code : codes) {
            unsigned char b[3] = {
                static_cast<unsigned char>(code & 0xFF),
                static_cast<unsigned char>((code >> 8)  & 0xFF),
                static_cast<unsigned char>((code >> 16) & 0xFF)
            };
            out.write(reinterpret_cast<const char*>(b), 3);
        }
        out.close();

        stats.compressed_bytes = 12 + codes.size() * 3;
        stats.ratio_pct = 100.0 * (1.0 - static_cast<double>(stats.compressed_bytes)
                                           / stats.original_bytes);
        stats.success = true;

    } catch (const std::exception& e) {
        stats.error_msg = e.what();
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    stats.time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return stats;
}

// ── DECODE ────────────────────────────────────────────────────────
LZWStats LZWCodec::decompress(const std::string& src, const std::string& dst) {
    LZWStats stats;
    auto t0 = std::chrono::high_resolution_clock::now();

    try {
        auto raw = read_file(src);
        if (raw.size() < 12) throw std::runtime_error("File too small");
        if (raw[0]!='L'||raw[1]!='Z'||raw[2]!='W'||raw[3]!='F')
            throw std::runtime_error("Not a .lzw file");

        stats.original_bytes = raw.size();

        uint32_t orig_size = 0, code_cnt = 0;
        std::memcpy(&orig_size, &raw[4], 4);
        std::memcpy(&code_cnt,  &raw[8], 4);

        // Read 24-bit codes
        std::vector<int> codes;
        codes.reserve(code_cnt);
        size_t pos = 12;
        for (uint32_t i = 0; i < code_cnt; ++i, pos += 3) {
            int c = raw[pos] | (raw[pos+1] << 8) | (raw[pos+2] << 16);
            codes.push_back(c);
        }

        // Rebuild dictionary
        std::vector<std::string> dict;
        dict.reserve(8192);
        for (int i = 0; i < INIT_SIZE; ++i)
            dict.push_back(std::string(1, static_cast<char>(i)));

        std::vector<unsigned char> output;
        output.reserve(orig_size);

        std::string prev = dict.at(codes[0]);
        for (unsigned char c : prev) output.push_back(c);

        for (size_t i = 1; i < codes.size(); ++i) {
            int code = codes[i];
            std::string entry;
            if (code < static_cast<int>(dict.size())) {
                entry = dict[code];
            } else if (code == static_cast<int>(dict.size())) {
                entry = prev + prev[0];   // special LZW case
            } else {
                throw std::runtime_error("Bad code in stream");
            }
            for (unsigned char c : entry) output.push_back(c);
            if (dict.size() <= static_cast<size_t>(MAX_CODE))
                dict.push_back(prev + entry[0]);
            prev = entry;
        }

        std::ofstream out(dst, std::ios::binary);
        out.write(reinterpret_cast<const char*>(output.data()), output.size());
        out.close();

        stats.compressed_bytes = output.size();
        stats.ratio_pct = 100.0 * (1.0 - static_cast<double>(raw.size()) / output.size());
        stats.success = true;

    } catch (const std::exception& e) {
        stats.error_msg = e.what();
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    stats.time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return stats;
}
