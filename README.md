File Compression Tool (DSA Project)

A command-line tool for lossless file compression and decompression using Huffman Coding. Built as a term project for the Data Structures and Algorithms course.
Features

- Compress text files to binary format.
- Decompress files back to original with 100% accuracy.
- Displays compression ratio, space saved, and execution time.
- Handles edge cases: empty files, single characters, large inputs.
Data Structures Used

| Structure | Purpose |
| :--- | :--- |
| Binary Tree | Huffman Tree for prefix code generation |
| Min-Heap (Priority Queue) | Extract two smallest frequency nodes |
| Hash Map | Store character frequencies and generated codes |
| Bitwise Operations | Pack binary codes into compact byte format |
Algorithm
Compression: 
1. Count frequency of each character in input file.
2. Build Huffman Tree using a Min-Heap.
3. Generate binary code for each character via tree traversal.
4. Replace characters with codes, pack bits into bytes, write to output file with tree header.
 Decompression: 
1. Read tree structure from compressed file header.
2. Traverse tree bit-by-bit (0 = left, 1 = right).
3. Output character when leaf is reached.
 Usage
 Compile (C++): 
```bash
g++ -o compressor src/main.cpp src/huffman.cpp -O2
```
 Compress: 
```bash
./compressor -c input.txt output.huf
```
 Decompress: 
```bash
./compressor -d output.huf restored.txt
```
Sample Output:
```
Original Size: 1024 bytes
Compressed Size: 580 bytes
Compression Ratio: 56.64%
Time: 0.023s
```
Project Structure

```
file-compression-dsa/
├── src/
│   ├── main.cpp          # CLI interface
│   ├── huffman.cpp/h     # Core compression logic
│   └── bitio.cpp         # Bit-level file operations
├── test/                 # Sample files
└── README.md
```
Complexity

| Operation | Time | Space |
| :--- | :--- | :--- |
| Frequency Count | O(n) | O(1) |
| Tree Construction | O(m log m) | O(m) |
| Encoding / Decoding | O(n) | O(n) |

n = file size in characters, m = number of unique characters (≤ 256).
Future Work

- GUI interface.
- Support for binary files (images, PDFs).
- LZW algorithm as an alternative.
- Multi-threading for large files.
Author

[Hamna] | [G1f24UBSCS040] | [DSA]
[Minahil] | [G1f24UBSCS048] | [DSA]
