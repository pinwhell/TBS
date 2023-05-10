
#include <random>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <iostream>
#include <ThunderByteScan.hpp>

#define MEASURE(name, x) \
auto start_##name = std::chrono::high_resolution_clock::now(); \
x; \
auto end_##name = std::chrono::high_resolution_clock::now(); \
auto duration_##name = std::chrono::duration_cast<std::chrono::microseconds>(end_##name - start_##name).count(); \
std::cout << #name << " took " << duration_##name << " microseconds.\n";

int main() {
    

    constexpr size_t pattern_size = 4;
    uint8_t pattern[pattern_size] = { 0xAA, 0xBB, 0xCC, 0xDD };

    constexpr size_t buffer_size = 1024;
    uint8_t buffer[buffer_size];

    

    // Insert pattern in the middle
    const size_t pattern_offset = buffer_size / 2 - pattern_size / 2;
    std::memcpy(&buffer[pattern_offset], pattern, pattern_size);

    // Perform search on buffer
    uintptr_t result;
    MEASURE(LocalFindPatternFirst, ThunderByteScan::LocalFindPatternFirst("AA BB CC DD", (uintptr_t)buffer, (uintptr_t)buffer + buffer_size, result));

    if ((uintptr_t)buffer + pattern_offset == result)
        std::cout << "Worked";

    return 0;
}