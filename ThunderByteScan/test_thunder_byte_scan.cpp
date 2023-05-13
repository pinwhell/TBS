
#define USE_SSE2
#define _CRT_SECURE_NO_WARNINGS

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
    ThunderByteScan::BatchPatternsScanResultFirst bpsrf;

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

/*
let imagine for a second we are scanning a kernel for offsets, as you know kernel have multiple version just like any other software (spececially linux), that means
that usually a single pattern of bytes "AA BB ? ? FF ? ..." its very unlikely that will work on all variants of the kernels version, unless same compiler, same function, 
and function hasnt changed from the beginning, which is rare for this conditions to meet, so usually there 
will be multiple patterns for multiple versions of the kernel to find the same offsets, eventually when implementing a solution with this specifications, its very likely
to mess up in the logic, so basicly this will put all this specification thogether in a single thihgt solution, as optimized as posible.

in other words, this solution will suit best when you dont know which pattern to use for which versions, maybe becouse there are to many versions to pattern them all 
or for any reason
*/

void TestCaseScenarioMultiThreadedSimultaneousUncertainScan()
{
    printf("\n");

#define KERN_BUFF_SIZE 0x10000

    char* kernelBuff = (char*)malloc(KERN_BUFF_SIZE);

    /*
    
    lets assume all of the next are the same, but diferent pattern, our goal is to find the {'1', '2', '3'}
    
    */

    // 6B 65 72 6E 65 6C 20 ? 20 6F 66 66 73 65 74 => kernel 1 offset 
    // 6B 65 72 6E 65 6C 20 6F 66 66 73 65 74 20 ? => kernel offset 2
    // ? 20 6F 66 66 73 65 74 20 6B 65 72 6E 65 6C => 3 offset kernel ...

    strcpy(kernelBuff + (size_t)std::round(KERN_BUFF_SIZE * 0.1), "kernel 1 offset");

    strcpy(kernelBuff + (size_t)std::round(KERN_BUFF_SIZE * 0.5), "kernel offset 1");

    strcpy(kernelBuff + (size_t)std::round(KERN_BUFF_SIZE * 0.8), "1 offset kernel");

    /* 
    Notice that it can be fuzzy some kernel version may contain multiple of the same pattern variation, for example
    kernel1Buff contains
    kernel 1 offset variation and kernel offset 2, our goal is to efficiently, find the 1, and stop when one of this two appear
    */

    // Perform search on buffer
    ThunderByteScan::BatchPatternsScanResultFirst bpsrf;

    char result;

    MEASURE( SimultaneousUncertainPatterns,
         std::cout << std::boolalpha << ThunderByteScan::LocalFindPatternBatchFirst({
        ThunderByteScan::PatternDesc("AA BB CC DD EE FF 11 22 33 44 55 66 77 88 99 00", "offsetMemberX"),   // Asuming this doesnt exist in current kernelBuff, but does on another kernel version
                                                                                                            // notice how unafected the scan is, and simply finds the others instead
        ThunderByteScan::PatternDesc("6B 65 72 6E 65 6C 20 ? 20 6F 66 66 73 65 74", "offsetMemberX"), // kernel 1 offset 
        ThunderByteScan::PatternDesc("6B 65 72 6E 65 6C 20 6F 66 66 73 65 74 20 ?", "offsetMemberX"), // kernel offset 2
        ThunderByteScan::PatternDesc("? 20 6F 66 66 73 65 74 20 6B 65 72 6E 65 6C", "offsetMemberX"), // 3 offset kernel
        }, (uintptr_t)kernelBuff, (uintptr_t)kernelBuff + KERN_BUFF_SIZE, bpsrf) << std::endl;
        );

    // each of the three pattern is valid to found, the trick is that when one of the three is found, all the other ones with the same UID will be stoped, in this case
    //  ThunderByteScan::PatternDesc("? 20 6F 66 66 73 65 74 20 6B 65 72 6E 65 6C", "offsetMemberX"), // 3 offset kernel, Was found first, so this way, we just stopped
    //         ThunderByteScan::PatternDesc("6B 65 72 6E 65 6C 20 ? 20 6F 66 66 73 65 74", "offsetMemberX"), // kernel 1 offset 
    // ThunderByteScan::PatternDesc("6B 65 72 6E 65 6C 20 6F 66 66 73 65 74 20 ?", "offsetMemberX"), // kernel offset 2
    // From Continue the search, avoiding unnecesary waiting

    char* pResultBase = (char*)(bpsrf["offsetMemberX"]);

    if (bpsrf.ResultWasFoundByPattern("offsetMemberX", "6B 65 72 6E 65 6C 20 ? 20 6F 66 66 73 65 74")) // Then offset is arround the middle from the base
        result = pResultBase[7]; // '1'
    else if (bpsrf.ResultWasFoundByPattern("offsetMemberX", "6B 65 72 6E 65 6C 20 6F 66 66 73 65 74 20 ?")) // Then offset is at the last byte from the base
        result = pResultBase[14]; // '1'
    else if (bpsrf.ResultWasFoundByPattern("offsetMemberX", "? 20 6F 66 66 73 65 74 20 6B 65 72 6E 65 6C")) // Then offset is at the first byte from the base
        result = pResultBase[0]; // '1'

    if (result == '1')
        std::cout << "Worked!\n";

    free(kernelBuff);
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

    ThunderByteScan::BatchPatternsScanResultFirst bpsrf; // Keeping track of result here
    
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
    TestCaseScenarioMultiThreadedSimultaneousUncertainScan();

    

    return 0;
}