cmake_minimum_required(VERSION 3.16)
project(FileZip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Compiler flags ─────────────────────────────────
if(MSVC)
  add_compile_options(/W4 /O2)
else()
  add_compile_options(-Wall -Wextra -O2)
endif()

# ── Include dirs ───────────────────────────────────
include_directories(include)

# ── Core library (shared between CLI and server) ───
add_library(huffman_lib STATIC
  src/huffman.cpp
)

# ── CLI executable (zero dependencies) ────────────
add_executable(filezip
  src/main_cli.cpp
)
target_link_libraries(filezip huffman_lib)

# ── Installation ───────────────────────────────────
install(TARGETS filezip DESTINATION bin)
install(DIRECTORY web/ DESTINATION share/filezip/web)

# ── Test target (quick self-test) ─────────────────
enable_testing()
add_test(NAME huffman_roundtrip
  COMMAND filezip demo
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

message(STATUS "")
message(STATUS "  FileZip build configured")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "  Targets:")
message(STATUS "    filezip    — CLI tool (zero deps)")
message(STATUS "")
message(STATUS "  Quick build:")
message(STATUS "    mkdir build && cd build")
message(STATUS "    cmake .. && cmake --build .")
message(STATUS "    ./filezip demo")
message(STATUS "")
