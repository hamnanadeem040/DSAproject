/*
 * rle.cpp — Run-Length Encoding (RLE)
 *
 * Algorithm:
 * ──────────
 * ENCODE (2-pointer scan):
 *   Collect literal bytes until a run of 3+ identical bytes is found.
 *   Flush any pending literals with LIT_ESC prefix.
 *   Emit RUN_ESC + count + byte for runs of length >= 3.
 *
 * DECODE:
 *   Read escape byte:
 *     RUN_ESC → read count, read byte, output byte × count
 *     LIT_ESC → read count, copy next `count` bytes literally
 *     anything else → output as-is (safety fallback)
 *
 * Complexity: O(n) both encode and decode
 * Best case:  highly repetitive data (BMP images, log files)
 * Worst case: random data may slightly expand (~1–2%)
 */

#include "rle.h"
#include <chrono>
#include <cstring>
#include <iostream>

std::vector<unsigned char> RLECodec::read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open: " + path);
    return { std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>() };
}

// ── ENCODE ────────────────────────────────────────────────────────
RLEStats RLECodec::compress(const std::string& src, const std::string& dst) {
    RLEStats stats;
    auto t0 = std::chrono::high_resolution_clock::now();

    try {
        auto data = read_file(src);
        if (data.empty()) throw std::runtime_error("Empty file");
        stats.original_bytes = data.size();

        std::vector<unsigned char> out;
        out.reserve(data.size());

        // Header
        out.push_back('R'); out.push_back('L');
        out.push_back('E'); out.push_back('F');
        uint32_t orig = static_cast<uint32_t>(data.size());
        for (int i = 0; i < 4; ++i)
            out.push_back((orig >> (i * 8)) & 0xFF);

        size_t i = 0;
        size_t n = data.size();

        while (i < n) {
            // Count run of identical bytes
            unsigned char cur = data[i];
            size_t run = 1;
            while (i + run < n && data[i + run] == cur && run < MAX_RUN)
                ++run;

            if (run >= 3) {
                // Emit compressed run
                out.push_back(RUN_ESC);
                out.push_back(static_cast<unsigned char>(run));
                out.push_back(cur);
                stats.runs_found++;
                i += run;
            } else {
                // Collect literal span (up to MAX_RUN bytes with no long run inside)
                size_t lit_start = i;
                size_t lit_len   = 0;
                while (i < n && lit_len < MAX_RUN) {
                    // Look-ahead: stop before a run of 3+
                    size_t peek = 1;
                    while (i + peek < n && data[i + peek] == data[i] && peek < MAX_RUN)
                        ++peek;
                    if (peek >= 3) break;
                    ++i; ++lit_len;
                }
                if (lit_len > 0) {
                    out.push_back(LIT_ESC);
                    out.push_back(static_cast<unsigned char>(lit_len));
                    for (size_t k = lit_start; k < lit_start + lit_len; ++k)
                        out.push_back(data[k]);
                }
            }
        }

        // Write output file
        std::ofstream f(dst, std::ios::binary);
        if (!f) throw std::runtime_error("Cannot create: " + dst);
        f.write(reinterpret_cast<const char*>(out.data()), out.size());
        f.close();

        stats.compressed_bytes = out.size();
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
RLEStats RLECodec::decompress(const std::string& src, const std::string& dst) {
    RLEStats stats;
    auto t0 = std::chrono::high_resolution_clock::now();

    try {
        auto raw = read_file(src);
        if (raw.size() < 8) throw std::runtime_error("File too small");
        if (raw[0]!='R'||raw[1]!='L'||raw[2]!='E'||raw[3]!='F')
            throw std::runtime_error("Not a .rle file");

        stats.original_bytes = raw.size();

        uint32_t orig_size = 0;
        std::memcpy(&orig_size, &raw[4], 4);

        std::vector<unsigned char> output;
        output.reserve(orig_size);

        size_t pos = 8;
        while (pos < raw.size()) {
            unsigned char esc = raw[pos++];
            if (esc == RUN_ESC) {
                if (pos + 1 >= raw.size()) break;
                unsigned char count = raw[pos++];
                unsigned char byte  = raw[pos++];
                for (int k = 0; k < count; ++k) output.push_back(byte);
                stats.runs_found++;
            } else if (esc == LIT_ESC) {
                if (pos >= raw.size()) break;
                unsigned char count = raw[pos++];
                for (int k = 0; k < count && pos < raw.size(); ++k)
                    output.push_back(raw[pos++]);
            } else {
                output.push_back(esc);   // raw byte (shouldn't happen with our encoder)
            }
        }

        std::ofstream f(dst, std::ios::binary);
        if (!f) throw std::runtime_error("Cannot create: " + dst);
        f.write(reinterpret_cast<const char*>(output.data()), output.size());
        f.close();

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
