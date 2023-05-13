#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <memory>
#include "ThreadPool.hpp"
#include <unordered_set>

namespace ThunderByteScan {

	class BatchPatternsScanResults {
	private:
		std::unordered_map<std::string, std::vector<uintptr_t>> mResults;

	public:
		
		uintptr_t getFirst(const std::string& uid)
		{
			// Checking to see if uid has even registered a vector
			if (mResults.find(uid) == mResults.end())
				return 0;

			// Lets check if that vector has at least more than one results
			if (mResults[uid].size() < 1)
				return 0;

			// If So Lets Return it
			return mResults[uid][0];
		}

		void setFirst(const std::string& uid, uintptr_t result)
		{
			// Checking to see if uid has even registered a vector
			if (mResults.find(uid) == mResults.end())
				mResults[uid] = std::vector<uintptr_t>();

			// Lets check if that vector has at least more than one results
			if (mResults[uid].size() < 1)
				mResults[uid].push_back(result);
			else
				mResults[uid][0] = result;
		}

		bool HasResult(const std::string& uid, bool checkResult = false)
		{
			return getFirst(uid) != 0;
		}

		std::vector<uintptr_t>& getResults(const std::string& uid)
		{
			if (mResults.find(uid) == mResults.end())
				mResults[uid] = std::vector<uintptr_t>();

			return (mResults[uid]);
		}
	};

	struct PatternTaskInfo {
		PatternTaskInfo()
			: atomStopped(nullptr)
		{}

		bool reportResults = true;
		std::vector<unsigned char> rawMask;
		std::vector<unsigned char> ignoreMask;
		std::string uid;
		std::string pattern;
		size_t patternSize;
		bool foundAtLeastOne = false;

		// it will hold the atomic boolean that will decide if alredy found uid (used in find first)
		std::atomic_bool* atomStopped;
	};

	struct PatternDesc {

		PatternDesc(const char* _pattern)
			: PatternDesc(_pattern, _pattern) // Using the pattern as UID
		{}

		PatternDesc(const std::string& _pattern)
			: PatternDesc(_pattern, _pattern) // Using the pattern as UID
		{}

		PatternDesc(const std::string& _pattern, const std::string& _uid)
			: pattern(_pattern)
			, uid(_uid)
		{}

		std::string pattern;
		std::string uid;
		std::atomic_bool* atomStopped;
	};

	class BatchPatternsScanResultFirst : public BatchPatternsScanResults {
	private:
		std::unordered_map<std::string, std::atomic_bool> stoppedMap;
		std::unordered_map<std::string, std::unique_ptr<PatternDesc>> resultExMap;

	public:

		std::atomic_bool* getAtomicBoolStoppedFor(const std::string& uid)
		{
			if (stoppedMap.find(uid) == stoppedMap.end())
				stoppedMap[uid] = false;

			return &(stoppedMap[uid]);
		}

		uintptr_t operator[] (const std::string& uid)
		{
			return getFirst(uid);
		}

		// Null Ptr when not result

		PatternDesc* getResultDescEx(const std::string& uid)
		{
			if (resultExMap.find(uid) == resultExMap.end())
				return nullptr;

			return resultExMap[uid].get();
		}

		void setResultDescEx(const std::string& _pattern, const std::string& _uid)
		{
			resultExMap[_uid] = std::make_unique<PatternDesc>(_pattern, _uid);
		}

		void setResultDescExFrom(const std::string& uid, const PatternTaskInfo& from)
		{
			setResultDescEx(from.pattern, from.uid);
		}

		bool ResultWasFoundByPattern(const std::string& uid, const std::string& pattern)
		{
			PatternDesc* resultFoundByInfo = getResultDescEx(uid);

			if (resultFoundByInfo == nullptr)
				return false;

			return resultFoundByInfo->pattern == pattern;
		}
	};

#ifdef USE_SSE2
#include <emmintrin.h> // Include SSE2 intrinsics header

	inline bool sse2_memcmp_with_mask(const void* ptr1, const void* ptr2, size_t num_bytes, unsigned char* mask)
	{
		// Cast input pointers to byte pointers
		const unsigned char* s1 = static_cast<const unsigned char*>(ptr1);
		const unsigned char* s2 = static_cast<const unsigned char*>(ptr2);

		// Number of bytes to compare in each iteration
		const size_t block_size = 16;

		// Loop through input buffers, comparing 16 bytes at a time
		size_t i = 0;

		for (; i + block_size <= num_bytes; i += block_size)
		{
			// Load 16 bytes from input buffers into SSE2 registers
			__m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s1 + i));
			__m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s2 + i));
			__m128i mask_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(mask + i));

			// Perform mask comparison using SSE2 instructions
			__m128i cmp = _mm_cmpeq_epi8(v1, v2);
			cmp = _mm_or_si128(cmp, mask_v);
			__m128i result = _mm_andnot_si128(cmp, _mm_set1_epi8(0xFF));

			// Check if any byte is non-zero in the result register
			if (_mm_movemask_epi8(result) != 0x0)
			{
				return false;
			}
		}

		// Compare remaining bytes using regular C++ code
		const unsigned char* p1 = s1 + i;
		const unsigned char* p2 = s2 + i;
		for (; i < num_bytes; i++)
		{
			if ((mask[i] == 0xFF) || (*p1 == *p2))
			{
				p1++;
				p2++;
				continue;
			}
			else
			{
				return false;
			}
		}

		return true;
	}
#endif


	/**

	@brief Compares two memory blocks while taking into account a mask indicating which bytes to compare.
	This function compares two memory blocks pointed to by ptr1 and ptr2, each of size num_bytes.
	It uses a mask to determine which bytes to compare and which to ignore. The mask is a pointer to
	an array of bytes of the same size as num_bytes. If a byte in the mask is non-zero, the corresponding
	byte in the memory blocks is compared; otherwise, it is ignored. The function returns true if all
	compared bytes match or were ignored according to the mask, false otherwise.
	@param ptr1 A pointer to the first memory block to compare.
	@param ptr2 A pointer to the second memory block to compare.
	@param num_bytes The number of bytes to compare in the memory blocks.
	@param mask A pointer to an array of bytes indicating which bytes to compare and which to ignore.
	@return A boolean value indicating whether the memory blocks match or not according to the mask.
	*/
    inline bool fast_memcmp_with_mask(const void* ptr1, const void* ptr2, size_t num_bytes, unsigned char* mask) {

#ifdef USE_SSE2
		return sse2_memcmp_with_mask(ptr1, ptr2, num_bytes, mask);
#endif

		const unsigned char* p1 = (const unsigned char*)ptr1;
		const unsigned char* p2 = (const unsigned char*)ptr2;
		size_t i;
		for (i = 0; i < num_bytes; i++) {
			if ((mask[i] == 0xFF) || (p1[i] == p2[i])) {
				continue;
			}
			else {
				return false;
			}
		}
		return true;
    }

	/**      
	@brief Parses a pattern string to generate a rawMask and an ignoreMask vectors.

	@param pattern A string containing a pattern to parse.      
	@param rawMask A vector of unsigned chars to be filled with the raw values extracted from the pattern.      
	@param ignoreMask A vector of unsigned chars to be filled with the same values as rawMask but with all bits set to 1 in the positions where there is a value in rawMask. 

	@return void    
	*/
	inline void ParsePattern(const std::string& pattern, std::vector<unsigned char>& rawMask, std::vector<unsigned char>& ignoreMask)
	{
		std::stringstream ss(pattern);
		std::string str; ss >> str;

		do
		{
			if (str.find("?") != std::string::npos)
			{
				rawMask.push_back(0x00);
				ignoreMask.push_back(0xFF);
			}
			else
			{
				auto c = (unsigned char)strtoull(str.c_str(), nullptr, 16);

				rawMask.push_back(c);
				ignoreMask.push_back(0x00);
			}

			if (ss.eof())
				break;
		} while (ss >> str);
	}

	inline void LocalFindPatternInfoTask(PatternTaskInfo* pattern, uintptr_t startAddr, uintptr_t endAddr, std::function<bool(PatternTaskInfo* _patternTaskInfo, uintptr_t _rslt)> foundCallback)
	{
		bool bAtLeastOneFound = false;

		if (startAddr == 0 || endAddr == 0 || startAddr >= endAddr)
			return;

		for (size_t i = startAddr; i <= (endAddr - pattern->patternSize) && pattern->reportResults; i++)
		{
			if (pattern->atomStopped && (*pattern->atomStopped) == true)
				break;

			if (fast_memcmp_with_mask(pattern->rawMask.data(), (unsigned char*)i, (size_t)pattern->patternSize, pattern->ignoreMask.data()))
			{
				pattern->foundAtLeastOne = true;
				pattern->reportResults = foundCallback(pattern, (uintptr_t)i);
			}
		}
	}


/**
Searches for multiple patterns within the specified memory range and calls a callback function for each found occurrence.

@param patterns A vector of unique ptrs of PatternInfo representing the information patterns to search for.
@param startAddr The starting address of the memory range to search within.
@param endAddr The ending address of the memory range to search within.
@param foundCallback A callback function that is called for each occurrence of a pattern found. The function should have the signature "bool func(const std::string& patt, uintptr_t addr)", where "patt" is the name of the pattern found(pattern itself usually) and "addr" is the address of the found pattern.

@return Returns true if the search was successful, false otherwise.

@remarks The function assumes that the memory addresses being searched are valid and accessible.

@see fast_memcmp_with_mask, ParsePattern

*/
	inline bool LocalFindPatternBatch(const std::vector<PatternTaskInfo*>& patterns, uintptr_t startAddr, uintptr_t endAddr, std::function<bool(PatternTaskInfo* patternTaskInfo, uintptr_t rslt)> foundCallback)
	{
		bool bResult = true;

		{
			ThreadPool pattsTp;

			for (PatternTaskInfo* patternInfo : patterns)
			{
				pattsTp.enqueue([&](PatternTaskInfo* pattInf, uintptr_t startAddr, uintptr_t endAddr, std::function<bool(PatternTaskInfo* _patternTaskInfo, uintptr_t _rslt)> foundCallback) {

					LocalFindPatternInfoTask(pattInf, startAddr, endAddr, foundCallback);

					}, patternInfo, startAddr, endAddr, foundCallback);
			}
		}

		std::unordered_set<std::string> uidFoundAtLeastOne;
		std::vector<PatternTaskInfo*> sortedInfos = patterns;
		std::sort(sortedInfos.begin(), sortedInfos.end(), [&](PatternTaskInfo* toSort1, PatternTaskInfo* toSort2) {
			return toSort1->foundAtLeastOne > toSort2->foundAtLeastOne;
			});

		for (PatternTaskInfo* pCurr : sortedInfos)
		{
			// That Means another pattern, with same uid, found a result, so that means this pattern also is done
			if (uidFoundAtLeastOne.find(pCurr->uid) != uidFoundAtLeastOne.end())
				continue;

			if (pCurr->foundAtLeastOne == false)
			{
				bResult = false;
				break;
			}

			uidFoundAtLeastOne.insert(pCurr->uid);
		}

		return bResult;
	}



	/**
	Searches for multiple patterns within the specified memory range and calls a callback function for each found occurrence.

	@param patterns A vector of strings representing the patterns to search for.
	@param startAddr The starting address of the memory range to search within.
	@param endAddr The ending address of the memory range to search within.
	@param foundCallback A callback function that is called for each occurrence of a pattern found. The function should have the signature "bool func(const std::string& patt, uintptr_t addr)", where "patt" is the name of the pattern found(pattern itself usually) and "addr" is the address of the found pattern.

	@return Returns true if the search was successful, false otherwise.

	@remarks The function assumes that the memory addresses being searched are valid and accessible.

	@see fast_memcmp_with_mask, ParsePattern

	*/
	inline bool LocalFindPatternBatch(const std::vector<PatternDesc>& patterns, uintptr_t startAddr, uintptr_t endAddr, std::function<bool(PatternTaskInfo* patternTaskInfo, uintptr_t rslt)> foundCallback)
	{
		bool bAtLeastOneFound = false;

		if (startAddr == 0 || endAddr == 0 || startAddr >= endAddr)
			return false;

		std::vector<std::unique_ptr<PatternTaskInfo>> allPatternsInfo;

		for (size_t i = 0; i < patterns.size(); i++)
		{
			std::unique_ptr<PatternTaskInfo> currPi = std::make_unique<PatternTaskInfo>();

			currPi->ignoreMask = std::vector<unsigned char>();
			currPi->rawMask = std::vector<unsigned char>();
			currPi->uid = patterns[i].uid;
			currPi->atomStopped = patterns[i].atomStopped;
			currPi->pattern = patterns[i].pattern;

			ParsePattern(currPi->pattern, currPi->rawMask, currPi->ignoreMask);

			currPi->patternSize = currPi->rawMask.size();

			allPatternsInfo.push_back(std::move(currPi));
		}

		std::vector<ThunderByteScan::PatternTaskInfo*> allPatternsInfop;

		for (auto& curr : allPatternsInfo)
			allPatternsInfop.push_back(curr.get());

		bAtLeastOneFound = LocalFindPatternBatch(allPatternsInfop, startAddr, endAddr, foundCallback);

		for (auto& currPatInf : allPatternsInfo)
		{
			memset(currPatInf->rawMask.data(), 0x0, currPatInf->rawMask.size());
			memset(currPatInf->ignoreMask.data(), 0x0, currPatInf->ignoreMask.size());
		}

		return bAtLeastOneFound;
	}

	/**

	Searches for a given pattern in the specified address range.
	@param pattern The pattern to search for.
	@param startAddr The starting address of the range to search.
	@param endAddr The ending address of the range to search.
	@param foundCallback The callback function to be called for each found address. The function should return true to continue searching and false to stop.
	@return true if the search was completed successfully, false otherwise.
	*/
	inline bool LocalFindPattern(const std::string& pattern, uintptr_t startAddr, uintptr_t endAddr, std::function<bool(uintptr_t addr)> foundCallback)
	{
		return LocalFindPatternBatch({ pattern }, startAddr, endAddr, [&](PatternTaskInfo* pattTaskInf, uintptr_t result) {

			return foundCallback(result);

			});
	}

	/**

	@brief Searches for a given pattern within the specified address range and returns all matches.
	@param pattern The pattern to search for.
	@param startAddr The start address of the address range to search in.
	@param endAddr The end address of the address range to search in.
	@param results A vector to store the addresses of all matches found.
	@return True if the search was successful, false otherwise.
	*/
	inline bool LocalFindPattern(const std::string& pattern, uintptr_t startAddr, uintptr_t endAddr, std::vector<uintptr_t>& results)
	{
		return LocalFindPattern(pattern, startAddr, endAddr, [&](uintptr_t addrFound) {

			results.push_back(addrFound);

		return true;
			});
	}
	
	/**

	Searches for the first occurrence of a pattern in the given memory range.
	@param pattern The pattern to search for.
	@param startAddr The starting address of the memory range to search in.
	@param endAddr The ending address of the memory range to search in.
	@param result The reference to the first memory address where the pattern was found.
	@return True if the search was successful, false otherwise.
	*/
	inline bool LocalFindPatternFirst(const std::string& pattern, uintptr_t startAddr, uintptr_t endAddr, uintptr_t& result)
	{
		result = NULL;

		return LocalFindPattern(pattern, startAddr, endAddr, [&](uintptr_t addrFound) {

			result = addrFound;

		return false;
			});
	}


	/**

	Searches for the first occurrence of a batch of patterns in a specified memory range.
	Remarks: if one of the patterns alredy has a result in the BATCHPATTERNSSCANRESULTSFIRST, this function wont touch it, and will ignore searching for that pattern, this way it avoids overriding the alredy there results
	this is very useful for the case scenario where you want to search in multiple areas, for multiple patterns, and you are not sure if for example all patterns will be inside a specific memory block
	@param patterns A vector of strings containing the patterns to search for.
	@param startAddr The starting address of the memory range to search in.
	@param endAddr The ending address of the memory range to search in.
	@param results A map of pattern strings to uintptr_t values indicating the first occurrence of each pattern found.
	@return True if all patterns were found.
	*/
	inline bool LocalFindPatternBatchFirst(const std::vector<PatternDesc>& _patterns, uintptr_t startAddr, uintptr_t endAddr, BatchPatternsScanResultFirst& results)
	{
		std::vector<PatternDesc> patterns = _patterns;	// Copy of all patterns
														// So we can modify the array
		std::unordered_set<std::string> toRemove;

		for (auto& pattDesc : patterns)
		{
			pattDesc.atomStopped = results.getAtomicBoolStoppedFor(pattDesc.uid);

			if (results.HasResult(pattDesc.uid, true)) // If Result is zero, we will override
					toRemove.insert(pattDesc.uid);
			else 
				results.setFirst(pattDesc.uid, 0);
		}

		// Removing the ones who alredy have a result
		// We dont want to overwrite the cache
		patterns.erase(std::remove_if(patterns.begin(), patterns.end(), [&](const PatternDesc& pattDesc) {
			return toRemove.count(pattDesc.uid) > 0;
			}), patterns.end());

		return LocalFindPatternBatch(patterns, startAddr, endAddr, [&](PatternTaskInfo* patternTaskInfo, uintptr_t rslt) {

		// Checking if alredy stopped
		if (*(patternTaskInfo->atomStopped) == true)
			return false;

		*(patternTaskInfo->atomStopped) = true; // Now stopping it, since we found result

		results.setFirst(patternTaskInfo->uid, rslt);
		results.setResultDescExFrom(patternTaskInfo->uid, (*patternTaskInfo));

		return false;
			});
	}

	/**

	Finds all occurrences of multiple patterns in a given memory region.
	@param patterns The list of patterns to search for.
	@param startAddr The start address of the memory region to search in.
	@param endAddr The end address of the memory region to search in.
	@param results A reference to the output parameter that will stores the search results.
	@return True if the search was successful, false otherwise.

	*/
	inline bool LocalFindPatternBatch(const std::vector<PatternDesc>& patterns, uintptr_t startAddr, uintptr_t endAddr, BatchPatternsScanResults& results)
	{
		return LocalFindPatternBatch(patterns, startAddr, endAddr, [&](PatternTaskInfo* patternTaskInfo, uintptr_t rslt) {
			results.getResults(patternTaskInfo->uid).emplace_back(rslt);
		return true;
			});
	}
}



