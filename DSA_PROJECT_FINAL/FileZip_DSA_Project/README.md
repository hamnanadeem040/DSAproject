# FileZip — Huffman Compression Tool
**DSA Final Project** | C++ Core + Web Frontend

---

## Project Structure

```
filezip/
├── include/
│   └── huffman.h          ← Node, Codec class declarations
├── src/
│   ├── huffman.cpp        ← Full Huffman implementation
│   ├── main_cli.cpp       ← CLI demo (zero dependencies)
│   └── server.cpp         ← Crow REST API (needs Crow installed)
├── web/
│   └── index.html         ← Complete web UI (drag & drop)
├── CMakeLists.txt
└── README.md
```

---

## Build & Run (Zero Dependencies)

```bash
# Option A: g++ direct
g++ -std=c++17 -O2 src/main_cli.cpp src/huffman.cpp -I include -o filezip

# Option B: CMake
mkdir build && cd build
cmake .. && cmake --build .

# Run the built-in demo
./filezip demo

# Compress a file
./filezip compress myfile.txt myfile.huff

# Decompress
./filezip decompress myfile.huff restored.txt
```

---

## Web Interface

Just open `web/index.html` in any browser. For full API integration:

```bash
# Install Crow (header-only C++ web framework)
# https://crowcpp.org

g++ -std=c++17 -O2 src/server.cpp src/huffman.cpp \
    -I include -I /path/to/crow/include \
    -lpthread -o filezip_server

./filezip_server
# Open: http://localhost:18080
```

---

## Algorithm: Huffman Coding

### Data Structures

| Structure | Where Used | Complexity |
|---|---|---|
| **Min-Heap** (priority_queue) | Building the Huffman tree | O(log n) per insert/extract |
| **Binary Tree** (shared_ptr nodes) | Huffman tree itself | O(n) traversal |
| **Hash Table** (unordered_map) | char→freq, char→bitcode | O(1) average lookup |
| **Dynamic Array** (vector) | Raw byte buffers, bit strings | O(1) amortized append |

### Algorithm Steps

```
1. Build frequency table    O(n)
   → scan file, count occurrences of each byte

2. Build min-heap           O(k log k)   k = unique chars ≤ 256
   → insert every char as a leaf node

3. Build Huffman tree       O(k log k)
   → repeat: pop 2 smallest, merge, push internal node

4. Generate codes           O(k)
   → DFS on tree, assign 0=left / 1=right at each branch

5. Encode file              O(n)
   → replace each byte with its variable-length bit-code

6. Serialise tree           O(k)
   → pre-order walk, store in file header for decompression

7. Decompress               O(m)   m = compressed bits
   → follow tree from root, emit char when leaf reached
```

### Overall Complexity

| Phase | Time | Space |
|---|---|---|
| Compress | **O(n log n)** | O(n + k) |
| Decompress | **O(m)** where m = compressed bits | O(k) for tree |

### Compressed File Format (.huff)

```
[4 bytes]  Magic:  0x48 0x55 0x46 0x46  ("HUFF")
[4 bytes]  Original file size  (uint32_t LE)
[4 bytes]  Tree serialisation length  (uint32_t LE)
[N bytes]  Serialised Huffman tree  (pre-order)
[1 byte ]  Padding bits count  (0–7)
[M bytes]  Compressed bit-stream
```

---

## Sample Output

```
╔═══════════════════════════════════════════════╗
║          FileZip  –  Huffman Codec            ║
║     DSA Final Project  |  filezip v1.0        ║
╚═══════════════════════════════════════════════╝

[ STEP 1 ]  Compressing demo_input.txt → demo_output.huff
  Operation       : Compress
  Original size   : 74000 bytes
  Result size     : 47846 bytes
  Ratio           : 35.34 %
  Time taken      : 8.84 ms
  Status          : SUCCESS

[ STEP 2 ]  Decompressing demo_output.huff → demo_restored.txt
  Operation       : Decompress
  Original size   : 47846 bytes
  Result size     : 74000 bytes
  Time taken      : 1.94 ms
  Status          : SUCCESS

[ STEP 3 ]  Integrity check (original == restored): ✓ PASSED

  Space saved : 35.34 %
  Compress time  : 8.84 ms
  Decompress time: 1.94 ms
```

---

## Why Huffman?

- **Lossless** — perfect reconstruction guaranteed
- **Optimal** — provably minimum-length prefix code for given frequencies
- **Entropy-based** — shorter codes = more frequent chars → exploits skew in real data
- Text files typically achieve **30–60% compression**
- Binary/already-compressed files may see smaller gains (~5–15%)

---

## Advisor Demo Script

1. Run `./filezip demo` — shows all 3 phases with integrity check
2. Open `web/index.html` — drag in a text file, click Compress
3. Point out the **Data Structures panel** on the right (Min-Heap, Binary Tree, Hash Table)
4. Show Dark/Light mode toggle
5. Compress the same file again — history panel updates with timestamps

---

## References

- Huffman, D.A. (1952). *A Method for the Construction of Minimum-Redundancy Codes*. IRE.
- Cormen et al., *Introduction to Algorithms* (CLRS), Chapter 16.3.
- Crow C++ web framework: https://crowcpp.org
