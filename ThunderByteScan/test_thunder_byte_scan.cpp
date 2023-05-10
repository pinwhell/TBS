
#define USE_SSE2

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

    size_t buffSz = 1000000;
    char * buffer = (char*)malloc(buffSz);

    // Insert pattern in the middle
    const size_t pattern_offset = buffSz / 2 - pattern_size / 2;
    std::memcpy(&buffer[pattern_offset], pattern, pattern_size);

    // Perform search on buffer
    ThunderByteScan::BATCHPATTERNSSCANRESULTSFIRST bpsrf;

    uintptr_t result;
    MEASURE(LocalFindPatternFirst, ThunderByteScan::LocalFindPatternBatchFirst({
        "AA BB ? DD",
        "DD ? BB AA",
        "CC ? ? CC"
        }, (uintptr_t)buffer, (uintptr_t)buffer + buffSz, bpsrf));

    if ((uintptr_t)buffer + pattern_offset == bpsrf["AA BB ? DD"])
        std::cout << "Worked 1\n";

    if ((uintptr_t)buffer + pattern_offset + 4 == bpsrf["DD ? BB AA"])
        std::cout << "Worked 2\n";

    if ((uintptr_t)buffer + pattern_offset + 2 == bpsrf["CC ? ? CC"])
        std::cout << "Worked 3\n";

    return 0;
}