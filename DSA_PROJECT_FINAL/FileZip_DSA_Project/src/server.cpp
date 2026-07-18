/*
 * server.cpp  –  Crow REST API for FileZip
 *
 * Endpoints
 * ─────────
 *  POST /api/compress    multipart/form-data  field: "file"
 *  POST /api/decompress  multipart/form-data  field: "file"
 *  GET  /api/history     returns JSON array of operations
 *  GET  /                serves index.html (static)
 *
 * Build (after installing Crow + Asio):
 *   g++ -std=c++17 -O2 src/server.cpp src/huffman.cpp \
 *       -I include -I /usr/local/include \
 *       -lpthread -o filezip_server
 *
 * Then open:  http://localhost:18080
 */

// NOTE: This file shows the full server logic. Because Crow is a
// header-only library that must be installed separately, we include
// a standalone CLI demo (main_cli.cpp) that works with zero dependencies.
// For the web demo, the index.html uses fetch() against this server.

#define CROW_MAIN
// #include "crow.h"          // uncomment after: pip install conan && conan install crow
#include "huffman.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>  // header-only JSON (nlohmann)

namespace fs = std::filesystem;
using json   = nlohmann::json;

// ─── in-memory operation history ───────────────────────────────────
struct HistoryEntry {
    std::string filename;
    std::string operation;    // "compress" | "decompress"
    size_t      original_bytes;
    size_t      result_bytes;
    double      ratio_pct;
    double      time_ms;
    std::string timestamp;
};
static std::vector<HistoryEntry> g_history;

// ─── temp directory for uploaded/processed files ───────────────────
static const std::string TMP_DIR = "./tmp/";

// Helper: current timestamp string
static std::string now_str() {
    auto t  = std::time(nullptr);
    auto tm = *std::localtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

/*
// Uncomment when Crow is installed:

int main() {
    fs::create_directories(TMP_DIR);

    crow::SimpleApp app;
    HuffmanCodec codec;

    // ── Serve static files ──────────────────────────────────────────
    CROW_ROUTE(app, "/")([]() {
        std::ifstream f("web/index.html");
        if (!f) return crow::response(404, "Not found");
        std::ostringstream ss; ss << f.rdbuf();
        crow::response res(ss.str());
        res.set_header("Content-Type", "text/html");
        return res;
    });

    CROW_ROUTE(app, "/css/<string>")([](const std::string& fname) {
        std::ifstream f("web/css/" + fname);
        if (!f) return crow::response(404);
        std::ostringstream ss; ss << f.rdbuf();
        crow::response res(ss.str());
        res.set_header("Content-Type", "text/css");
        return res;
    });

    CROW_ROUTE(app, "/js/<string>")([](const std::string& fname) {
        std::ifstream f("web/js/" + fname);
        if (!f) return crow::response(404);
        std::ostringstream ss; ss << f.rdbuf();
        crow::response res(ss.str());
        res.set_header("Content-Type", "application/javascript");
        return res;
    });

    // ── POST /api/compress ──────────────────────────────────────────
    CROW_ROUTE(app, "/api/compress").methods("POST"_method)(
    [&codec](const crow::request& req) {
        crow::multipart::message msg(req);
        auto it = msg.part_map.find("file");
        if (it == msg.part_map.end())
            return crow::response(400, "{\"error\":\"No file uploaded\"}");

        auto& part     = it->second;
        auto  fname    = part.headers.count("Content-Disposition")
                         ? "uploaded_file" : "file";
        std::string src = TMP_DIR + fname;
        std::string dst = TMP_DIR + fname + ".huff";

        // write uploaded bytes to temp file
        std::ofstream tmp(src, std::ios::binary);
        tmp.write(part.body.data(), part.body.size());
        tmp.close();

        auto stats = codec.compress(src, dst);

        if (!stats.success) {
            return crow::response(500,
                json{{"error", stats.error_msg}}.dump());
        }

        // Read compressed file for download
        std::ifstream rf(dst, std::ios::binary);
        std::string body((std::istreambuf_iterator<char>(rf)),
                          std::istreambuf_iterator<char>());

        g_history.push_back({fname, "compress",
            stats.original_bytes, stats.compressed_bytes,
            stats.ratio_pct, stats.time_ms, now_str()});

        crow::response res(200, body);
        res.set_header("Content-Type",        "application/octet-stream");
        res.set_header("Content-Disposition", "attachment; filename=\"" + fname + ".huff\"");
        res.set_header("X-Original-Size",     std::to_string(stats.original_bytes));
        res.set_header("X-Compressed-Size",   std::to_string(stats.compressed_bytes));
        res.set_header("X-Ratio",             std::to_string(stats.ratio_pct));
        res.set_header("X-Time-Ms",           std::to_string(stats.time_ms));
        return res;
    });

    // ── POST /api/decompress ────────────────────────────────────────
    CROW_ROUTE(app, "/api/decompress").methods("POST"_method)(
    [&codec](const crow::request& req) {
        crow::multipart::message msg(req);
        auto it = msg.part_map.find("file");
        if (it == msg.part_map.end())
            return crow::response(400, "{\"error\":\"No file uploaded\"}");

        auto& part  = it->second;
        std::string src = TMP_DIR + "input.huff";
        std::string dst = TMP_DIR + "decompressed_output";

        std::ofstream tmp(src, std::ios::binary);
        tmp.write(part.body.data(), part.body.size());
        tmp.close();

        auto stats = codec.decompress(src, dst);

        if (!stats.success)
            return crow::response(500, json{{"error", stats.error_msg}}.dump());

        std::ifstream rf(dst, std::ios::binary);
        std::string body((std::istreambuf_iterator<char>(rf)),
                          std::istreambuf_iterator<char>());

        g_history.push_back({"input.huff", "decompress",
            stats.original_bytes, stats.compressed_bytes,
            stats.ratio_pct, stats.time_ms, now_str()});

        crow::response res(200, body);
        res.set_header("Content-Type",      "application/octet-stream");
        res.set_header("Content-Disposition","attachment; filename=\"decompressed\"");
        res.set_header("X-Original-Size",   std::to_string(stats.original_bytes));
        res.set_header("X-Result-Size",     std::to_string(stats.compressed_bytes));
        res.set_header("X-Time-Ms",         std::to_string(stats.time_ms));
        return res;
    });

    // ── GET /api/history ────────────────────────────────────────────
    CROW_ROUTE(app, "/api/history")([]() {
        json arr = json::array();
        for (auto& e : g_history) {
            arr.push_back({
                {"filename",       e.filename},
                {"operation",      e.operation},
                {"original_bytes", e.original_bytes},
                {"result_bytes",   e.result_bytes},
                {"ratio_pct",      e.ratio_pct},
                {"time_ms",        e.time_ms},
                {"timestamp",      e.timestamp}
            });
        }
        crow::response res(arr.dump(2));
        res.set_header("Content-Type", "application/json");
        return res;
    });

    std::cout << "FileZip server running on http://localhost:18080\n";
    app.port(18080).multithreaded().run();
}
*/

// ─── Standalone entry point (no Crow needed for compilation demo) ──
// See main_cli.cpp for the fully working CLI version.
int main() {
    std::cout << "FileZip server stub.\n"
              << "Install Crow (https://crowcpp.org) then uncomment the main() above.\n";
    return 0;
}
