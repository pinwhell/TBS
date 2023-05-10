
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


    constexpr size_t pattern_size = 8;
    uint8_t pattern[pattern_size] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xDD, 0xCC, 0xBB, 0xAA };

    char * buffer = (char*)malloc(1000000);

    // Insert pattern in the middle
    const size_t pattern_offset = 1000000 / 2 - pattern_size / 2;
    std::memcpy(&buffer[pattern_offset], pattern, pattern_size);

    // Perform search on buffer
    ThunderByteScan::BATCHPATTERNSSCANRESULTSFIRST bpsrf;

    uintptr_t result;
    MEASURE(LocalFindPatternFirst, ThunderByteScan::LocalFindPatternBatchFirst({
        "AA BB CC DD",
        "DD CC BB AA"
        }, (uintptr_t)buffer, (uintptr_t)buffer + 1000000, bpsrf));

    if ((uintptr_t)buffer + pattern_offset == bpsrf["AA BB CC DD"])
        std::cout << "Worked";

    return 0;
}