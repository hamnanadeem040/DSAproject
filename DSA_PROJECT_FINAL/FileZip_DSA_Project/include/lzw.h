#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <stdexcept>

/*
 * LZW (Lempel-Ziv-Welch) Compression
 *
 * Data Structures:
 *   Encode: Hash Table (string → code)   — O(1) lookup per symbol
 *   Decode: Dynamic Array (code → string) — O(1) index access
 *
 * Key idea: build a dictionary of repeated byte sequences on-the-fly.
 * No dictionary is stored in the file — decoder rebuilds it identically.
 *
 * File format (.lzw):
 *   [4 bytes] Magic  "LZWF"
 *   [4 bytes] Original size (uint32_t LE)
 *   [4 bytes] Code count    (uint32_t LE)
 *   [N*3 bytes] 24-bit codes packed (3 bytes each, little-endian)
 */

struct LZWStats {
    size_t  original_bytes   = 0;
    size_t  compressed_bytes = 0;
    double  ratio_pct        = 0.0;
    double  time_ms          = 0.0;
    size_t  dict_size        = 0;      // final dictionary entries
    std::string error_msg;
    bool    success = false;
};

class LZWCodec {
public:
    LZWStats compress  (const std::string& src, const std::string& dst);
    LZWStats decompress(const std::string& src, const std::string& dst);

private:
    static constexpr int MAX_CODE  = (1 << 24) - 1;   // 24-bit codes
    static constexpr int INIT_SIZE = 256;

    std::vector<unsigned char> read_file(const std::string& path);
};
