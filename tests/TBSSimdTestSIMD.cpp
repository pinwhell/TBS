#include <doctest/doctest.h>

#define TBS_IMPL_SSE2
#define TBS_IMPL_AVX
#define TBS_IMPL_ARCH_WORD_SIMD

#include <TBS/TBS.hpp>

using namespace TBS;

TEST_CASE("Memory Searching First")
{
	UByte toFind = 0x1C;
	UByte testCase[] = {
	0x1, 0x2, 0x3, 0x4, 0x5, 0xFF, 0xFF, 0x1C, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C
	};

	auto result = SearchFirst(testCase, testCase + sizeof(testCase), toFind);
	CHECK(result == testCase + 7);

	result = SearchFirst(result + 1, testCase + sizeof(testCase), toFind);
	CHECK(result == testCase + 8);

	result = SearchFirst(result + 1, testCase + sizeof(testCase), toFind);
	CHECK(result == testCase + 31);
}