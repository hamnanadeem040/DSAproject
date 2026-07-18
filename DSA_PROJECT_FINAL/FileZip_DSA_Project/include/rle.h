#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

/*
 * RLE (Run-Length Encoding) Compression
 *
 * Data Structures:
 *   Simple array (vector<unsigned char>) — O(n) linear scan
 *
 * Encoding scheme:
 *   For each run of identical bytes:
 *     If run length >= 3:  emit [0xFF escape] [count] [byte]
 *     Else:                emit literal bytes (no overhead)
 *   Literal runs (no repeat): emit [0xFE escape] [count] [bytes...]
 *
 * This two-pass scheme avoids worst-case expansion on random data.
 *
 * File format (.rle):
 *   [4 bytes] Magic  "RLEF"
 *   [4 bytes] Original size (uint32_t LE)
 *   [N bytes] Encoded stream
 */

struct RLEStats {
    size_t  original_bytes   = 0;
    size_t  compressed_bytes = 0;
    double  ratio_pct        = 0.0;
    double  time_ms          = 0.0;
    size_t  runs_found       = 0;    // number of compressed runs
    std::string error_msg;
    bool    success = false;
};

class RLECodec {
public:
    RLEStats compress  (const std::string& src, const std::string& dst);
    RLEStats decompress(const std::string& src, const std::string& dst);

private:
    static constexpr unsigned char RUN_ESC = 0xFF;   // marks a run
    static constexpr unsigned char LIT_ESC = 0xFE;   // marks literal block
    static constexpr int MAX_RUN = 255;

    std::vector<unsigned char> read_file(const std::string& path);
};
