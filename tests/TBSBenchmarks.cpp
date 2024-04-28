#include <doctest/doctest.h>
#include <random>
#include <iostream>

#define TBS_IMPL_SSE2
#define TBS_IMPL_AVX
#define TBS_IMPL_ARCH_WORD_SIMD

#include <TBS.hpp>

using namespace TBS;

/*
	wildCardMask expected to be 0xFF for byte that are wild carded and 0x0 for non-wildcarded bytes
*/
inline bool MemoryCompareWithMaskByteByte(const UByte* chunk1, const UByte* chunk2, size_t len, const UByte* wildCardMask) {
	for (size_t i = 0; i < len; i++) {
		if (wildCardMask[i] != 0xFF && chunk1[i] != chunk2[i])
			return false;
	}

	return true;
}

TEST_CASE("Benchmark MemoryCompares")
{
	// Define the size of the chunk
	constexpr size_t chunkSize = 10000000; // a million bytes
	constexpr size_t iterations = 50000000;
	constexpr double dIterations = double(iterations);

	// Allocate memory for the chunks and the wildcard mask
	UByte* chunk1 = new UByte[chunkSize];
	UByte* chunk2 = new UByte[chunkSize];
	UByte* wildCardMask = new UByte[chunkSize];

	// Fill the chunks and the wildcard mask with random data
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<ULong> distribution(0, 255); // Random byte values
	for (size_t i = 0; i < chunkSize; ++i) {
		chunk1[i] = UByte(distribution(gen));
		chunk2[i] = UByte(distribution(gen));
		wildCardMask[i] = distribution(gen) < 128 ? 0xFF : 0x00; // 50% chance of being a wildcard
	}

	std::chrono::microseconds elapsedMicrosecondsByteByte = std::chrono::milliseconds(0);
	std::chrono::microseconds elapsedMicrosecondsPlatformWord = std::chrono::milliseconds(0);
#ifdef TBS_IMPL_SSE2
	std::chrono::microseconds elapsedMicrosecondsSSE2 = std::chrono::milliseconds(0);
#endif
#ifdef TBS_IMPL_AVX
	std::chrono::microseconds elapsedMicrosecondsAVX = std::chrono::milliseconds(0);
#endif


	for (size_t i = 0; i < iterations; i++)
	{
		// Measure the time taken by MemoryCompareWithMaskByteByte
		auto startByteByte = std::chrono::high_resolution_clock::now();
		bool resultByteByte = MemoryCompareWithMaskByteByte(chunk1, chunk2, chunkSize, wildCardMask);
		auto endByteByte = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsedByteByte = endByteByte - startByteByte;
		elapsedMicrosecondsByteByte += std::chrono::duration_cast<std::chrono::microseconds>(elapsedByteByte);
	}
	auto elapsedMicrosecondsByteByteAv = double(elapsedMicrosecondsByteByte.count()) / dIterations;

	for (size_t i = 0; i < iterations; i++)
	{
		// Measure the time taken by CompareWithMaskWord
		auto start = std::chrono::high_resolution_clock::now();
		bool result = Memory::SIMD::Platform::CompareWithMask(chunk1, chunk2, chunkSize, wildCardMask);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		elapsedMicrosecondsPlatformWord += std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
	}

	auto elapsedMicrosecondsPlatformWordAv = double(elapsedMicrosecondsPlatformWord.count()) / dIterations;

#ifdef TBS_IMPL_SSE2
	for (size_t i = 0; i < iterations; i++)
	{
		// Measure the time taken by Memory::SSE2::CompareWithMask
		auto start = std::chrono::high_resolution_clock::now();
		bool result = Memory::SIMD::SSE2::CompareWithMask(chunk1, chunk2, chunkSize, wildCardMask);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		elapsedMicrosecondsSSE2 += std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
	}

	auto elapsedMicrosecondsSSE2Av = double(elapsedMicrosecondsSSE2.count()) / dIterations;
#endif

#ifdef TBS_IMPL_AVX
	for (size_t i = 0; i < iterations; i++)
	{
		// Measure the time taken by Memory::AVX::CompareWithMask
		auto start = std::chrono::high_resolution_clock::now();
		bool result = Memory::SIMD::AVX::CompareWithMask(chunk1, chunk2, chunkSize, wildCardMask);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		elapsedMicrosecondsAVX += std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
	}

	auto elapsedMicrosecondsAVXAv = double(elapsedMicrosecondsAVX.count()) / dIterations;
#endif



	std::cout << elapsedMicrosecondsByteByteAv << " microseconds. took MemoryCompareWithMaskByteByte()" << std::endl;
	std::cout << elapsedMicrosecondsPlatformWordAv << " microseconds. took Memory::CompareWithMaskWord()" << std::endl;
#ifdef TBS_IMPL_SSE2
	std::cout << elapsedMicrosecondsSSE2Av << " microseconds. took Memory::SSE2::CompareWithMask()" << std::endl;
#endif
#ifdef TBS_IMPL_AVX
	std::cout << elapsedMicrosecondsAVXAv << " microseconds. took Memory::AVX::CompareWithMask()" << std::endl;
#endif

	// Clean up
	delete[] chunk1;
	delete[] chunk2;
	delete[] wildCardMask;
}