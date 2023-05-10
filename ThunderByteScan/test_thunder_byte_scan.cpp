
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


void TestMultiThreadedSimultaneousScan()
{
    printf("\n");
    constexpr size_t pattern_size = 8;
    uint8_t pattern[pattern_size] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xDD, 0xCC, 0xBB, 0xAA };

    size_t buffSz = 1000000;
    char* buffer = (char*)malloc(buffSz);

    // Insert pattern in the middle
    const size_t pattern_offset = buffSz / 2 - pattern_size / 2;
    std::memcpy(&buffer[pattern_offset], pattern, pattern_size);

    // Perform search on buffer
    ThunderByteScan::BATCHPATTERNSSCANRESULTSFIRST bpsrf;

    uintptr_t result;
    MEASURE(SimultaneousPatterns, std::cout << std::boolalpha << ThunderByteScan::LocalFindPatternBatchFirst({
        "AA BB ? DD",
        "DD ? BB AA",
        "CC ? ? CC"
        }, (uintptr_t)buffer, (uintptr_t)buffer + buffSz, bpsrf) << std::endl;);

    if ((uintptr_t)buffer + pattern_offset == bpsrf["AA BB ? DD"])
        std::cout << "Worked 1\n";

    if ((uintptr_t)buffer + pattern_offset + 4 == bpsrf["DD ? BB AA"])
        std::cout << "Worked 2\n";

    if ((uintptr_t)buffer + pattern_offset + 2 == bpsrf["CC ? ? CC"])
        std::cout << "Worked 3\n";

    free(buffer);
    printf("\n");
}

void TestMultiThreadedSimultaneousCachedScan()
{
    printf("\n");
    constexpr size_t buffer_size = 0x1000;

    char* buffer1 = (char*)malloc(buffer_size);
    char* buffer2 = (char*)malloc(buffer_size);
    char* buffer3 = (char*)malloc(buffer_size);

    // Zeroing Out the buffers
    memset(buffer1, 0x0, buffer_size);
    memset(buffer2, 0x0, buffer_size);
    memset(buffer3, 0x0, buffer_size);

    // Purposly putting the to find values in multiple places
    memcpy(buffer1 + buffer_size / 2, "\xAA\xBB\xCC\xDD", 4);
    memcpy(buffer2 + buffer_size / 2, "\xDD\xCC\xBB\xAA", 4);
    memcpy(buffer3 + buffer_size / 2, "\xCC\xDD\xDD\xCC", 4);

    uintptr_t result1 = (uintptr_t)(buffer1 + buffer_size / 2);
    uintptr_t result2 = (uintptr_t)(buffer2 + buffer_size / 2);
    uintptr_t result3 = (uintptr_t)(buffer3 + buffer_size / 2);

    ThunderByteScan::BATCHPATTERNSSCANRESULTSFIRST bpsrf; // Keeping track of result here

    MEASURE(CachedPatterns, 

    // First Face Here
    std::cout << std::boolalpha << ThunderByteScan::LocalFindPatternBatchFirst({
        "AA BB ? DD",
        "DD ? BB AA",
        "CC ? ? CC"
        }, (uintptr_t)buffer1, (uintptr_t)buffer1 + buffer_size, bpsrf) << std::endl; // False (not found all yet)

    // Second Face Here
    std::cout << std::boolalpha << ThunderByteScan::LocalFindPatternBatchFirst({
        "AA BB ? DD",
        "DD ? BB AA",
        "CC ? ? CC"
        }, (uintptr_t)buffer2, (uintptr_t)buffer2 + buffer_size, bpsrf) << std::endl; // False (not found all yet)

    // Third Face Here
    std::cout << std::boolalpha << ThunderByteScan::LocalFindPatternBatchFirst({
        "AA BB ? DD",
        "DD ? BB AA",
        "CC ? ? CC"
        }, (uintptr_t)buffer3, (uintptr_t)buffer3 + buffer_size, bpsrf) << std::endl; // True (Found All)


    );


    // Testing Results

    if (result1 == bpsrf["AA BB ? DD"])
        std::cout << "Worked 1\n";

    if (result2 == bpsrf["DD ? BB AA"])
        std::cout << "Worked 2\n";

    if (result3 == bpsrf["CC ? ? CC"])
        std::cout << "Worked 3\n";


    free(buffer3);
    free(buffer2);
    free(buffer1);
    printf("\n");
}

int main() {
    TestMultiThreadedSimultaneousScan();
    TestMultiThreadedSimultaneousCachedScan();

    

    return 0;
}