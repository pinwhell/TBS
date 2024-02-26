#include <ThunderByteScan.hpp>
#include <doctest/doctest.h>
#include <future>
#include <functional>

namespace TBS = ThunderByteScan;

TEST_CASE("Batch Patterns Scan Results Test")
{
	TBS::BatchPatternsScanResults batchPattern;

	batchPattern.setFirst("UID_1", 0x10);

	CHECK(batchPattern.HasResult("UID_1"));
	CHECK_FALSE(batchPattern.HasResult("INVALID_UID"));

	CHECK(batchPattern.getResults("UID_1").size() == 1);
	CHECK(batchPattern.getResults("INVALID_UID").size() == 0);

	CHECK(batchPattern.getFirst("UID_1") == 0x10);
}

/*Dummy X86 Sample 1*/
std::vector<unsigned char> gSample1 {
    0x55,                           // push ebp
    0x89, 0xE5,                     // mov ebp, esp
    0xB8, 0x2A, 0x00, 0x00, 0x00,   // mov eax, 42
    0x83, 0xC0, 0x08,               // add eax, 8
    0x83, 0xE8, 0x05,               // sub eax, 5
    0xBB, 0x0A, 0x00, 0x00, 0x00,   // mov ebx, 10
    0xF7, 0xE3,                     // mul ebx
    0xF7, 0xF1,                     // div ecx
    0x5D,                           // pop ebp
    0xC3                            // ret
};


/*Dummy X86 Sample 2*/
std::vector<unsigned char> gSample2{
    0x55,                           // push ebp
    0x89, 0xE5,                     // mov ebp, esp
    0x83, 0xC0, 0x08,               // add eax, 8
    0xBB, 0x0A, 0x00, 0x00, 0x00,   // mov ebx, 10
    0x83, 0xE8, 0x05,               // sub eax, 5
    0xB8, 0x2A, 0x00, 0x00, 0x00,   // mov eax, 42
    0xB8, 0x08, 0x00, 0x00, 0x00,   // mov eax, 8
    0xF7, 0xE3,                     // mul ebx
    0xF7, 0xF1,                     // div ecx
    0x5D,                           // pop ebp
    0xC3                            // ret
};

TEST_CASE("Batch Pattern Search Test")
{
    TBS::BatchPatternsScanResults results;
    std::vector<TBS::PatternDesc> testCase = {
            {"55 89 ? B8 ? ? ? ? 83 ? ? 83 ? ? BB", "X86PatternTag", [](uint64_t result) { return *(uint32_t*)(result + 4); }},
            {"55 89 ? 83 ? ? BB ? ? ? ? 83 ? ? B8", "X86PatternTag", [](uint64_t result) { return *(uint8_t*)(result + 5); }},
            "F7 ? 5D C3"
    };

    bool foundAll = TBS::LocalFindPatternBatch(
        testCase,
        TBS::VecStart(gSample1),
        TBS::VecEnd(gSample1),
        results
    );

    CHECK(foundAll);

    CHECK(results.HasResult("X86PatternTag"));
    CHECK(results.HasResult("F7 ? 5D C3"));
    CHECK(results["X86PatternTag"] == 42);

    // Now with second sample

    results = TBS::BatchPatternsScanResults();

    foundAll = TBS::LocalFindPatternBatch(
        testCase,
        TBS::VecStart(gSample2),
        TBS::VecEnd(gSample2),
        results
    );

    CHECK(foundAll);

    CHECK(results.HasResult("X86PatternTag"));
    CHECK(results.HasResult("F7 ? 5D C3"));
    CHECK(results["X86PatternTag"] == 8);
}

TEST_CASE("Test Find Pattern")
{
    const std::string testCase = "55 89 ? B8 ? ? ? ? 83 ? ? 83 ? ? BB";
    std::vector<uintptr_t> results;

    bool found = TBS::LocalFindPattern(testCase,
        TBS::VecStart(gSample1),
        TBS::VecEnd(gSample1), results);

    CHECK(found);
    CHECK(results.size() == 1);
    CHECK(TBS::VecStart(gSample1) == results[0]);
}