#include <doctest/doctest.h>
#include <iostream>
#include <TBS.hpp>

using namespace TBS;

TEST_CASE("Pattern Parsing") {
	Pattern::ParseResult res;

	CHECK(Pattern::Parse("AA ? BB ? CC ? DD ? EE ? FF", res));
	CHECK_EQ(res.mCompareMask.size(), 11);
	CHECK_EQ(res.mPattern.size(), 11);
	CHECK(memcmp(res.mCompareMask.data(), "\xFF\x00\xFF\x00\xFF\x00\xFF\x00\xFF\x00\xFF", 11) == 0);
	CHECK(res.mFirstSolidOff == 0);

	CHECK(Pattern::Parse("48 ? ? ? ? ? ? 48 ? ? ? 48 ? ? 25", res));
	CHECK_EQ(res.mCompareMask.size(), 15);
	CHECK_EQ(res.mPattern.size(), 15);
	CHECK(memcmp(res.mCompareMask.data(), "\xFF\x00\x00\x00\x00\x00\x00\xFF\x00\x00\x00\xFF\x00\x00\xFF", 15) == 0);
	CHECK(res.mFirstSolidOff == 0);

	CHECK(Pattern::Parse("AA ?? BB ? CC ?? DD ? EE ?? FF ??", res));
	CHECK_EQ(res.mCompareMask.size(), 12);
	CHECK_EQ(res.mPattern.size(), 12);
	CHECK(memcmp(res.mCompareMask.data(), "\xFF\x00\xFF\x00\xFF\x00\xFF\x00\xFF\x00\xFF\x00", 11) == 0);
	CHECK(res.mFirstSolidOff == 0);

	CHECK(Pattern::Parse("? ? ?? ? ?? ? ?? ? ? ? ? ? ? BB CC ? DD EE ? FF", res)); // First Solid Offset, at 'BB' aka +13
	CHECK_EQ(res.mPattern.size(), 20);
	CHECK_EQ(res.mCompareMask.size(), 20);
	CHECK(memcmp(res.mCompareMask.data(), "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\x00\xFF\xFF\x00\xFF", 20) == 0);
	CHECK(res.mFirstSolidOff == 13);

	CHECK_FALSE(Pattern::Parse("AA ??? BB ? CC ?? DD? ? EE ?? FF ??", res));
	CHECK_EQ(res.mCompareMask.size(), 1 /*Just 1 valid byte*/);
	CHECK_EQ(res.mPattern.size(), 1 /*Just 1 valid byte*/);
	CHECK(memcmp(res.mCompareMask.data(), "\xFF", 1) == 0);
	CHECK(res.mFirstSolidOff == 0);


	CHECK_FALSE(Pattern::Parse("AA ? BB ?CC ? DD ?EE ? FF", res));
	CHECK_EQ(res.mPattern.size(), 3 /*Just 3 valid bytes*/);
	CHECK_EQ(res.mCompareMask.size(), 3 /*Just 3 valid bytes*/);
	CHECK(memcmp(res.mCompareMask.data(), "\xFF\x00\xFF", 3) == 0);
	CHECK(res.mFirstSolidOff == 0);
	

	const char testRawPattern[] = "\x10\xFF\x30\x40\xFF\x60";
	const char testRawPatternMask[] = "x?xx?x";
	CHECK(Pattern::Parse(testRawPattern, testRawPatternMask, res));
	CHECK_EQ(res.mPattern.size(), sizeof(testRawPattern) - 1);
	CHECK_EQ(res.mCompareMask.size(), sizeof(testRawPatternMask) - 1);
	CHECK(memcmp(res.mCompareMask.data(), "\xFF\x00\xFF\xFF\x00\xFF", 6) == 0);
	CHECK(res.mFirstSolidOff == 0);
}

TEST_CASE("Memory Comparing Masked")
{
	struct MemoryCompareTest {
		TBS::String<> mPattern;
		const UByte* mTestCase;
		size_t mMaskExpectedLength;
		size_t mPatternExpectedLength;
	};

	MemoryCompareTest testCases[]{
		// Exact match
		{"AA BB CC DD EE FF", [] {static UByte testCase1[]{ 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }; return testCase1; }(), 6, 6},

		// Wildcard match
		{"A? ? ?C ? E? ? F?", [] {static UByte testCase2[]{ 0xAA, 0xDE, 0xCC, 0xAD, 0xEE, 0xBE, 0xFF }; return testCase2; }(), 7, 7},

		// Pattern longer than sequence
		{"AA BB CC DD EE FF", [] {static UByte testCase4[]{ 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }; return testCase4; }(), 6, 6},

		// Sequence longer than pattern
		{"AA BB CC DD EE FF", [] {static UByte testCase5[]{ 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11 }; return testCase5; }(), 6, 6},

		// Multiple wildcard matches
		{"A? ? ? ?D ? FF", [] {static UByte testCase6[]{ 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }; return testCase6; }(), 6, 6},

		// All wildcards
		{"? ? ? ? ? ?", [] {static UByte testCase7[]{ 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }; return testCase7; }(), 6, 6},

		// Single-byte pattern and sequence
		{"AA", [] {static UByte testCase8[]{ 0xAA }; return testCase8; }(), 1, 1},

		// Single-byte wildcarded
		{"?A", [] {static UByte testCase8[]{ 0xBA }; return testCase8; }(), 1, 1},
	};

	for (const auto& testCase : testCases)
	{
		Pattern::ParseResult res;

		CHECK(Pattern::Parse(testCase.mPattern, res));
		CHECK_EQ(res.mPattern.size(), testCase.mPatternExpectedLength);
		CHECK_EQ(res.mCompareMask.size(), testCase.mMaskExpectedLength);
		CHECK(Memory::CompareWithMask(testCase.mTestCase, res.mPattern.data(), res.mPattern.size(), res.mCompareMask.data()));
	}
}

TEST_CASE("Memory Slicing Testing")
{
	Memory::Slice<UPtr>::Container container(0x0, 0x2C, 0x10);
	Memory::Slice<UPtr>::Container::Iterator it = container.begin();

	CHECK((*it).mStart == 0x0);
	CHECK((*it).mEnd == 0x10);

	++it;

	CHECK((*it).mStart == 0x10);
	CHECK((*it).mEnd == 0x20);

	++it;

	CHECK((*it).mStart == 0x20);
	CHECK((*it).mEnd == 0x2C);

	++it;

	CHECK((*it).mStart == 0x2C);
	CHECK((*it).mEnd == 0x2C);

	++it;

	CHECK((*it).mStart == 0x2C);
	CHECK((*it).mEnd == 0x2C);
	CHECK(it == container.end());

	// Step overflow Test

	container = Memory::Slice<UPtr>::Container(0x0, 0x10, 0xFFFF);
	it = container.begin();

	CHECK((*it).mStart == 0x0);
	CHECK((*it).mEnd == 0x10);

	++it;

	CHECK(it == container.end());

	// Negative Range Test

	container = Memory::Slice<UPtr>::Container(0x10, 0x0, 0x10); // Should be defaulted to end
	it = container.begin();
	CHECK(it == container.end());
}

template<typename T>
auto PatternScanSliceBase(T buff, U64 level = 0)
{
	return (T)((UByte*)buff + (PATTERN_SEARCH_SLICE_SIZE * level));
}

TEST_CASE("Pattern Sliced Scan Test")
{
	// Lets define a big buffer of 2x PATTERN_SEARCH_SLICE_SIZE

	constexpr auto TESTBUFF_SIZE = PATTERN_SEARCH_SLICE_SIZE * 2;
	std::unique_ptr<UByte[]> buffSmart = std::make_unique<UByte[]>(TESTBUFF_SIZE);
	auto buff = buffSmart.get();

	// Lets clear everything

	memset(buff, 0x0, TESTBUFF_SIZE);
	memcpy(buff, "\xDE\xAD\xBE\xEF", 4); // Denotes beginning of buff, for debugging purposes

	// create the state

	State<10> state(buff, buff + TESTBUFF_SIZE);

	// lets add patterns guranteed not to be found (so we can emulate scannig over the entire `buff`)

	state
		.AddPattern(
		state.PatternBuilder()
		.setUID("Pattern 1")
		.setPattern("FF FF FF FF FF")
		.Build()
	)
		.AddPattern(
		state.PatternBuilder()
		.setUID("Pattern 2")
		.setPattern("EE EE EE EE EE EE EE EE EE EE EE EE EE")
		.Build()
	);

	Pattern::Description& pattern1 = state.mDescriptionts[0];
	Pattern::Description& pattern2 = state.mDescriptionts[1];

	auto pattern1Size = pattern1.mParsed.mPattern.size();
	auto pattern2Size = pattern2.mParsed.mPattern.size();

	// Expected to be at the beginning
	CHECK(pattern1.mLastSearchPos == buff);
	CHECK(pattern2.mLastSearchPos == buff);

	// Expected the pattern to keep wanting to find more (in subsecuent slices of search range)
	CHECK(Pattern::Scan(pattern1));
	CHECK(Pattern::Scan(pattern2));

	// Expected first slice scan exausted and .mLastSearchPosition to be = buff + PATTERN_SEARCH_SLICE_SIZE - patternXSize + 1 aka at the end of first slice
	CHECK_EQ(pattern1.mLastSearchPos, PatternScanSliceBase(buff, 1) - pattern1Size + 1);
	CHECK_EQ(pattern2.mLastSearchPos, PatternScanSliceBase(buff, 1) - pattern2Size + 1);

	// Same Again..., now we expect a false, since we will consume second and last scanning slice
	CHECK_FALSE(Pattern::Scan(pattern1));
	CHECK_FALSE(Pattern::Scan(pattern2));

	// Now at the end of second slice
	CHECK_EQ(pattern1.mLastSearchPos, PatternScanSliceBase(buff, 2) - pattern1Size + 1);
	CHECK_EQ(pattern2.mLastSearchPos, PatternScanSliceBase(buff, 2) - pattern2Size + 1);

	// Sanity Checks, expecting TBS::Scan to return false (patterns already at end and didnt found anything)
	CHECK_FALSE(Scan(state));
}

TEST_CASE("Pattern Scan #1")
{
	UByte testCase[] = {
		0xAA, 0x00, 0xBB, 0x11, 0xCC, 0x22, 0xDD, 0x33, 0xEE, 0x44, 0xFF
	};

	State<> state(testCase, testCase + sizeof(testCase));

	state.AddPattern(
		state
		.PatternBuilder()
		.setPattern("AA ? BB ? CC ? DD ? EE ? FF")
		.setUID("TestUID")
		.AddTransformer([](Pattern::Description& desc, U64 res) -> U64 {
			return U64(((*(U32*)(res + 6))) & 0x00FF00FFull); // Just Picking 0xEE and 0xFF
			})
		.AddTransformer([](Pattern::Description& desc, U64 res) -> U64 {
				// expected to be 0xXXEEXXDD comming from previous transform
				CHECK(res == 0x00EE00DDull);
				return res | 0xFF00FF00ull; // expected to be 0xFFEEFFDD
			})
				.Build()
				);

	CHECK(Scan(state));
	CHECK(state["TestUID"].ResultsGet().size() == 1);
	CHECK(state["TestUID"] == 0xFFEEFFDD);
}

/*
	This test case demonstrates how the pattern scanning library efficiently searches for specific patterns within data buffers.

	- Setup: Two strings are stored in separate buffers along with additional data.
	- Pattern Definition: Two patterns are defined to search for specific byte sequences within the buffers, using a shared identifier.
	- Transformers: Functions are applied to matched results to adjust their positions within the buffer.
	- Scan Execution: The library scans the buffers for the defined patterns, stopping as soon as a match is found.
	- Result Verification: The test verifies that a single match is found and retrieves the matched string from the result.

	This example illustrates the library's simplicity and effectiveness in handling complex pattern scanning tasks.
*/
TEST_CASE("Pattern Scan #2")
{
	const char string1[] = "Old Version String";
	const char string2[] = "New Version String";

	UByte buffer1[] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
		0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
		0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
		0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
		0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
		0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40
	};

	UByte buffer2[] = {
		0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
		0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
		0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
		0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60,
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
		0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
		0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
		0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80
	};

	static size_t strOff1 = sizeof(buffer1) - sizeof(string1) - 5;
	static size_t strOff2 = sizeof(buffer2) - sizeof(string2) - 12;
	static size_t strRelOff1 = 16;
	static size_t strRelOff2 = 18;
	static size_t pattern1TostrRelOff1 = strRelOff1 - 2;
	static size_t pattern2TostrRelOff2 = strRelOff2 - 5;

	strcpy((char*)(buffer1 + strOff1), string1);
	strcpy((char*)(buffer2 + strOff2), string2);
	*(size_t*)(buffer1 + strRelOff1) = strOff1 - strRelOff1;
	*(size_t*)(buffer2 + strRelOff2) = strOff2 - strRelOff2;

	State<> state{};
	{
		auto builder = state
			.PatternBuilder()
			.setUID("String")
			.stopOnFirstMatch();

		state.AddPattern(
			builder
			.Clone()
			.setPattern("? 04 ? ?? ?7 0?")
			.setScanStart(buffer1)
			.setScanEnd(buffer1 + sizeof(buffer1))
			.AddTransformer([](auto& desc, U64 res) {
				return res + pattern1TostrRelOff1;
			})
			.AddTransformer([](auto& desc, U64 res) {
				return res + *(size_t*)res;
			})
			.Build()
		);

		state.AddPattern(
			builder
			.Clone()
			.setPattern("4? ? ?8 49 ? 4B 4?")
			.setScanStart(buffer2)
			.setScanEnd(buffer2 + sizeof(buffer2))
			.AddTransformer([](auto& desc, U64 res) {
				return res + pattern2TostrRelOff2;
			})
			.AddTransformer([](auto& desc, U64 res) {
				return res + *(size_t*)res;
			})
			.Build()
		);
	}

	CHECK(Scan(state));
	CHECK(state["String"].ResultsGet().size() == 1);
	const char* pStr = (const char*)(U64)state["String"];
	CHECK_FALSE(pStr == nullptr);
	std::string str = std::string(pStr);
	CHECK((str == std::string(string1) || str == std::string(string2)));
	std::cout << "Magic: " << str << std::endl;
}

TEST_CASE("Pattern Scan #3")
{
	UByte testCase[] = {
		0xAA, 0x00, 0xBB, 0x11, 0xCC, 0x22, 0xDD, 0x33, 0xEE, 0x44, 0xFF
	};

	State<25> state(testCase, testCase + sizeof(testCase));

	state.AddPattern(
		state
		.PatternBuilder()
		.setPatternRaw("\xAA\x00\xBB\x00\xCC\x00\xDD\x00\xEE\x00\xFF")
		.setMask("x?x?x?x?x?x")
		.setUID("TestUID")
		.AddTransformer([](Pattern::Description& desc, U64 res) -> U64 {
			return U64(((*(U32*)(res + 6))) & 0x00FF00FFull); // Just Picking 0xEE and 0xFF
			})
		.AddTransformer([](Pattern::Description& desc, U64 res) -> U64 {
				// expected to be 0xXXEEXXDD comming from previous transform
				CHECK(res == 0x00EE00DDull);
				return res | 0xFF00FF00ull; // expected to be 0xFFEEFFDD
			})
				.Build()
				);

	CHECK(Scan(state));
	CHECK(state["TestUID"].ResultsGet().size() == 1);
	CHECK(state["TestUID"] == 0xFFEEFFDD);
}

TEST_CASE("Pattern Scan #4")
{
	UByte testCase[] = {
		0xAA, 0x00, 0xBB, 0x11, 0xCC, 0x22, 0xDD, 0x33, 0xEE, 0x44, 0xFF
	};

	State<> state(testCase, testCase + sizeof(testCase));

	auto builder = state
		.PatternBuilder()
		.setUID("TestUID")
		.AddTransformer([](Pattern::Description& desc, U64 res) -> U64 {
		return U64(((*(U32*)(res + 6))) & 0x00FF00FFull); // Just Picking 0xEE and 0xFF
			})
		.AddTransformer([](Pattern::Description& desc, U64 res) -> U64 {
				// expected to be 0xXXEEXXDD comming from previous transform
				CHECK(res == 0x00EE00DDull);
				return res | 0xFF00FF00ull; // expected to be 0xFFEEFFDD
			});

	state.AddPattern(
		builder
		.Clone()
		.setPattern("AA FF BB EE CC ? DD ? EE AA FF")
		.Build()
				); // Negative (Should Fail)

	state.AddPattern(
		builder
		.Clone()
		.setPattern("AA ? BB ? CC ? DD ? EE ? FF")
		.Build()
				); // Positive Should Find

	CHECK(Scan(state));
	CHECK(state["TestUID"].ResultsGet().size() == 1);
	CHECK(state["TestUID"] == 0xFFEEFFDD);
}

TEST_CASE("Pattern Scan #5")
{
	UByte testCase[] = {
		0xCC, 0x00, 0x00, 0x00, 0x00
	};

	State<> state(testCase, testCase + sizeof(testCase));

	state.AddPattern(
		state
		.PatternBuilder()
		.stopOnFirstMatch()
		.setUID("TestUID1")
		.setPatternRaw("\xCC")
		.setMask("x")
		.Build()
	);

	state.AddPattern(
		state
		.PatternBuilder()
		.stopOnFirstMatch()
		.setUID("TestUID2")
		.setPatternRaw("\xCC\xFF\xFF\xFF")
		.setMask("x???")
		.Build()
	);

	CHECK(Scan(state));
	CHECK(state["TestUID1"].ResultsGet().size() == 1);
	CHECK(state["TestUID2"].ResultsGet().size() == 1);
	auto tstCase0Addr = &testCase[0];
	CHECK_EQ(tstCase0Addr, (decltype(tstCase0Addr))(TBS::Pattern::Result)state["TestUID1"]);
	CHECK_EQ(tstCase0Addr, (decltype(tstCase0Addr))(TBS::Pattern::Result)state["TestUID2"]);
}