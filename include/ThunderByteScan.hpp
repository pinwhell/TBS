#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <thread>
#include <unordered_set>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <string.h>

// Multi-threading enabled by default

#define TBS_MT

#ifdef TBS_NO_MT
#undef TBS_MT
#endif

namespace ThunderByteScan {
#ifdef TBS_MT
	class ThreadPool {
	public:
		inline ThreadPool(size_t threads = std::thread::hardware_concurrency()) : stop(false) 
		{
			auto workerTask = [this] {
				for (;;) 
				{
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(this->queue_mutex);
						this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
						if (this->stop && this->tasks.empty())
							return;
						task = std::move(this->tasks.front());
						this->tasks.pop();
					}
					task();
				}
			};

			for (size_t i = 0; i < threads; ++i)
				workers.emplace_back(workerTask);
		}

		template<class F, class... Args>
		inline void enqueue(F&& f, Args&&... args) {
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
			}
			condition.notify_one();
		}

		inline ~ThreadPool() {
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				stop = true;
			}
			condition.notify_all();
			for (std::thread& worker : workers)
				worker.join();
		}

	private:
		std::vector<std::thread> workers;
		std::queue<std::function<void()>> tasks;
		std::mutex queue_mutex;
		std::condition_variable condition;
		bool stop;
	};

#endif

	/**

	@class BatchPatternsScanResults
	@brief This class is used to store the results of a batch pattern scan operation.
	This class is used to store the results of a batch pattern scan operation. It stores a map of pattern identifiers to a vector of uintptr_t values representing the memory addresses where the pattern was found.
	*/
	class BatchPatternsScanResults {
	private:
		std::unordered_map<std::string, std::vector<uintptr_t>> mResults;
		std::unordered_map<std::string, std::atomic_bool> stoppedMap;
	public:

		/**
* @brief Returns a pointer to the atomic bool value stored for a given UID indicating whether the scan for that UID has been stopped or not.
*
* @param uid A string representing the UID for which the atomic bool value is requested.
* @return A pointer to the atomic bool value for the given UID.
*/
		std::atomic_bool* getAtomicBoolStoppedFor(const std::string& uid)
		{
			if (stoppedMap.find(uid) == stoppedMap.end())
				stoppedMap[uid] = false;

			return &(stoppedMap[uid]);
		}

/**
* @brief Overloads the [] operator to return the first result for a given UID.
*
* @param uid A string representing the UID for which the first result is requested.
* @return The first result found for the given UID.
*/
		uintptr_t operator[] (const std::string& uid)
		{
			return getFirst(uid);
		}

		/**
* @brief Returns the first result of a specified scan.
*
* @param uid The unique identifier for the scan.
*
* @return The uintptr_t address of the first result of the specified scan.
*
* This function returns the uintptr_t address of the first result of a scan with
* the specified unique identifier(pattern itself by default). If the scan has not yet been registered, or if
* it has no results, the function returns 0.
*/
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

		/**
* @brief sets the first result of a specified scan.
*
* @param uid The unique identifier for the scan.
*
* @return void.
*
* This function assign the uintptr_t address of the first result of a scan with
* the specified unique identifier(pattern itself by default).
*/
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

		/**

	Checks if the given uid has any result(s) in the mResults unordered map.
	@param uid The unique identifier to check for results.
	@param checkResult A boolean parameter indicating whether to check the result value is zero(if so, consider non existing). Default value is false.
	@return A boolean value representing whether the given uid has any result(s) in the mResults unordered map.
	*/
		bool HasResult(const std::string& uid, bool checkResult = false)
		{
			return getFirst(uid) != 0;
		}

		/**

	Returns a reference to a std::vector of uintptr_t values containing the results associated with the given uid.
	If the uid has no results registered, an empty vector is created and returned.
	@param uid A std::string representing the unique identifier to retrieve the results for.
	@return A reference to a std::vector of uintptr_t values containing the results associated with the given uid.
	*/
		std::vector<uintptr_t>& getResults(const std::string& uid)
		{
			if (mResults.find(uid) == mResults.end())
				mResults[uid] = std::vector<uintptr_t>();

			return (mResults[uid]);
		}
	};

	struct PatternTaskInfo {
		PatternTaskInfo()
			: mpStopped(nullptr)
			, mStartFind(0)
			, mEndFind(0)
		{}

		bool mReportResults = true;
		std::vector<unsigned char> mMatchMask;
		std::vector<unsigned char> mIgnoreMask;
		std::string mUID;
		std::string mPattern;
		std::function<uint64_t(uint64_t)> mResolveCallback;
		size_t mPatternSize;
		bool mFound = false;
		uint64_t mStartFind;
		uint64_t mEndFind;

		// it will hold the atomic boolean that will decide if alredy found uid (used in find first)
		std::atomic_bool* mpStopped;
	};

	struct PatternDesc {

		PatternDesc(const char* _pattern, std::function<uint64_t(uint64_t)> resolveCallback)
			: PatternDesc(_pattern, _pattern, resolveCallback, 0, 0) // Using the pattern as UID
		{}

		PatternDesc(const std::string& _pattern, std::function<uint64_t(uint64_t)> resolveCallback)
			: PatternDesc(_pattern, _pattern, resolveCallback, 0, 0) // Using the pattern as UID
		{}

		PatternDesc(const char* _pattern)
			: PatternDesc(_pattern, _pattern, 0, 0, 0) // Using the pattern as UID
		{}

		PatternDesc(const std::string& _pattern)
			: PatternDesc(_pattern, _pattern, 0, 0, 0) // Using the pattern as UID
		{}

		PatternDesc(const std::string& _pattern, const std::string& _uid)
			: PatternDesc(_pattern, _uid, 0, 0, 0)
		{}

		PatternDesc(const std::string& _pattern, const std::string& _uid, std::function<uint64_t(uint64_t)> resolveCallback)
			: PatternDesc(_pattern, _uid, resolveCallback, 0, 0)
		{}

		PatternDesc(const std::string& _pattern, const std::string& _uid, std::function<uint64_t(uint64_t)> resolveCallback, uint64_t startFind, uint64_t endFind)
			: mPattern(_pattern)
			, mUID(_uid)
			, mpStopped(nullptr)
			, mResolve(resolveCallback)
			, mStartFind(startFind)
			, mEndFind(endFind)
		{}

		std::string mPattern;
		std::string mUID;
		std::atomic_bool* mpStopped;
		std::function<uint64_t(uint64_t)> mResolve;
		uint64_t mStartFind;
		uint64_t mEndFind;
	};

	struct PatternDescBuilder {
		inline PatternDescBuilder() 
			: mPattern("")
			, mUID("")
			, mResolve(0)
			, mStartFind(0)
			, mEndFind(0)
		{}

		inline PatternDescBuilder& setPattern(const std::string& pattern)
		{
			mPattern = pattern;

			if (mUID.empty())
				mUID = pattern;

			return *this;
		}

		inline PatternDescBuilder& setUID(const std::string& uid)
		{
			mUID = uid;

			return *this;
		}

		inline PatternDescBuilder& setResolveCallback(std::function<uint64_t(uint64_t)> callback)
		{
			mResolve = callback;

			return *this;
		}

		template<typename T>
		inline PatternDescBuilder& setStartFind(T startFind)
		{
			mStartFind = (uint64_t)startFind;

			return *this;
		}

		template<typename T>
		inline PatternDescBuilder& setEndFind(T endFind)
		{
			mEndFind = (uint64_t)endFind;

			return *this;
		}

		inline PatternDesc Build()
		{
			PatternDesc result(mPattern, mUID, mResolve, mStartFind, mEndFind);

			*this = PatternDescBuilder();

			return result;
		}

		std::string mPattern;
		std::string mUID;
		std::function<uint64_t(uint64_t)> mResolve;
		uint64_t mStartFind;
		uint64_t mEndFind;
	};

	/**

	@class BatchPatternsScanResultFirst

	@brief A class that represents the result of a batch pattern scan where only the first result is stored for each UID.

	This class extends BatchPatternsScanResults and adds additional functionality to store atomic bool values,

	unique_ptr to PatternDesc and check if a specific pattern was found for a given UID.
	*/
	class BatchPatternsScanResultFirst : public BatchPatternsScanResults {
	private:
		std::unordered_map<std::string, std::unique_ptr<PatternDesc>> resultExMap;

	public:


		/**
  * @brief Returns a pointer to the PatternDesc object stored for a given UID, describing the pattern who found the result(if any), or nullptr if no such object exists.
  *
  * @param uid A string representing the UID for which the PatternDesc object is requested.
  * @return A pointer to the PatternDesc object for the given UID, or nullptr if no such object exists.
  */
		PatternDesc* getResultDescEx(const std::string& uid)
		{
			if (resultExMap.find(uid) == resultExMap.end())
				return nullptr;

			return resultExMap[uid].get();
		}

		/**
 * @brief Stores a new PatternDesc object for a given UID Describing the pattern who found the result.
 *
 * @param _pattern A string representing the pattern used to find the result.
 * @param _uid A string representing the UID for which the result was found.
 */
		void setResultDescEx(const std::string& _pattern, const std::string& _uid)
		{
			resultExMap[_uid] = std::make_unique<PatternDesc>(_pattern, _uid);
		}

		/**
		 * @brief Stores a new PatternDesc object for a given UID Describing the pattern who found the result using information from a PatternTaskInfo where the result was found from.
		 *
		 * @param uid A string representing the UID for which the result was found.
		 * @param from A PatternTaskInfo object containing information about the result found.
		 */
		void setResultDescExFrom(const std::string& uid, const PatternTaskInfo& from)
		{
			setResultDescEx(from.mPattern, from.mUID);
		}

		/**
  * @brief Checks whether the result found for a given UID was found using a specific pattern.
  *
  * @param uid A string representing the UID for which the result was found.
  * @param pattern A string representing the pattern used to find the result.
  * @return True if the result for the given UID was found using the given pattern, false otherwise.
  */
		bool ResultWasFoundByPattern(const std::string& uid, const std::string& pattern)
		{
			PatternDesc* resultFoundByInfo = getResultDescEx(uid);

			if (resultFoundByInfo == nullptr)
				return false;

			return resultFoundByInfo->mPattern == pattern;
		}
	};

#ifdef USE_SSE2
#include <emmintrin.h> // Include SSE2 intrinsics header

	/**

	@brief Compares two memory regions of a given size using SSE2 instructions and a mask.
	@param ptr1 Pointer to the first memory region.
	@param ptr2 Pointer to the second memory region.
	@param num_bytes Size of the memory regions in bytes.
	@param mask Mask array that specifies which bytes should be ignored during comparison.
	@return true if the memory regions are equal, false otherwise.
	*/
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
			__m128i result = _mm_andnot_si128(cmp, _mm_set1_epi8((char)0xFF));

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

	/**

	@brief Performs a pattern scan on a specific range of memory.

	@param pattern A pointer to a PatternTaskInfo struct that holds information about the pattern to scan.

	@param startAddr The starting address of the memory range to scan.

	@param endAddr The ending address of the memory range to scan.

	@param foundCallback A function to be called when a pattern is found. The function takes a PatternTaskInfo pointer and the address where the pattern was found as arguments and returns a boolean indicating whether to continue scanning or not.
	*/
	inline void LocalFindPatternInfoTask(PatternTaskInfo* pattern, uintptr_t startAddr, uintptr_t endAddr, std::function<bool(PatternTaskInfo* _patternTaskInfo, uintptr_t _rslt)> foundCallback)
	{
		bool bAtLeastOneFound = false;

		if (startAddr == 0 || endAddr == 0 || startAddr >= endAddr)
			return;

		for (size_t i = startAddr; i <= (endAddr - pattern->mPatternSize) && pattern->mReportResults; i++)
		{
			if (pattern->mpStopped && (*pattern->mpStopped) == true)
				break;

			if (fast_memcmp_with_mask(pattern->mMatchMask.data(), (unsigned char*)i,
				(size_t)pattern->mPatternSize,
				pattern->mIgnoreMask.data()) == false)
				continue;

			pattern->mFound = true;
			
			if ((pattern->mReportResults = foundCallback(pattern, (uintptr_t)i)) == false)
				break;
		}
	}


	/**
	Searches for multiple patterns within the specified memory range and calls a callback function for each found occurrence.

	@param patterns A vector of unique ptrs of PatternInfo representing the information patterns to search for.
	@param startAddr The starting address of the memory range to search within.
	@param endAddr The ending address of the memory range to search within.
	@param foundCallback A callback function that is called for each occurrence of a pattern found. The function should have the signature "bool func(const std::string& patt, uintptr_t addr)", where "patt" is the name of the pattern found(pattern itself usually) and "addr" is the address of the found pattern.

	@return Returns true if every single pattern uid did found a result, false otherwise.

	@remarks The function assumes that the memory addresses being searched are valid and accessible.

	@see fast_memcmp_with_mask, ParsePattern

	*/
	inline bool LocalFindPatternBatch(const std::vector<PatternTaskInfo*>& patterns, std::function<bool(PatternTaskInfo* patternTaskInfo, uintptr_t rslt)> foundCallback)
	{
		{
#ifdef TBS_MT
			ThreadPool pattsTp;
#endif

			for (PatternTaskInfo* patternInfo : patterns)
			{
#ifdef TBS_MT
				// Lets Delegate the Pattern Scan
				pattsTp.enqueue([&](PatternTaskInfo* patternInfo, std::function<bool(PatternTaskInfo* _patternTaskInfo, uintptr_t _rslt)> foundCallback) {
#endif
					LocalFindPatternInfoTask(patternInfo, patternInfo->mStartFind, patternInfo->mEndFind, foundCallback);

#ifdef TBS_MT
				}, patternInfo, foundCallback);
#endif
			}
		}

		// At this point, all the pattern scan tasks
		// finished doing its work!

		std::unordered_set<std::string> uidFoundAtLeastOne;
		std::vector<PatternTaskInfo*> foundSorted = patterns;

		std::sort(foundSorted.begin(), foundSorted.end(), [&](PatternTaskInfo* toSort1, PatternTaskInfo* toSort2) {
			return toSort1->mFound > toSort2->mFound;
			});

		// sorting foundSorted on the criteria of found first
		// so the process of searching the UIDs with alredy a result is simpler

		for (PatternTaskInfo* currPatternTaskInfo : foundSorted)
		{
			if (uidFoundAtLeastOne.find(currPatternTaskInfo->mUID) != uidFoundAtLeastOne.end())
			{
				// At this point, another pattern 
				// with same UID found a result
				// meaning that this Pattern Task Info with 
				// same UID should be ingnored

				continue;
			}

			// At this point, currPatternTaskInfo 
			// UID was not found yet
				
			if (currPatternTaskInfo->mFound == false)
			{
				// At this point, we know that at least 1 or more 
				// pattern from the batch didnt found its results
				// Lets notify to the caller, so caller can re-call
				// again with cache and potentially find results else-where
				// in another memory range

				return false;
			}

			// At this point, this currPatternTaskInfo was indeed found!, 
			// and its other pattern task friends with same UID wont need 
			// to find/work/worry anymore, lets add it to the ignore list

			uidFoundAtLeastOne.insert(currPatternTaskInfo->mUID);
		}

		// At this point, seems that all Patterns UIDs were 
		// satisfied with a corresponding result
		// Lets report to the caller

		return true;
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

		if (startAddr == 0 || endAddr == 0 || startAddr >= endAddr)
			return false;

		std::vector<std::unique_ptr<PatternTaskInfo>> allPatternsInfo;

		for (size_t i = 0; i < patterns.size(); i++)
		{
			allPatternsInfo.emplace_back(std::make_unique<PatternTaskInfo>());
			PatternTaskInfo& currPatternTaskInf = *allPatternsInfo.back();

			currPatternTaskInf.mUID			= patterns[i].mUID;
			currPatternTaskInf.mPattern		= patterns[i].mPattern;
			currPatternTaskInf.mpStopped	= patterns[i].mpStopped;
			currPatternTaskInf.mMatchMask	= std::vector<unsigned char>();
			currPatternTaskInf.mIgnoreMask	= std::vector<unsigned char>();
			currPatternTaskInf.mStartFind	= patterns[i].mStartFind == 0 ? startAddr : patterns[i].mStartFind;
			currPatternTaskInf.mEndFind		= patterns[i].mEndFind == 0 ? endAddr : patterns[i].mEndFind;
			currPatternTaskInf.mResolveCallback = patterns[i].mResolve;

			ParsePattern(currPatternTaskInf.mPattern, currPatternTaskInf.mMatchMask, currPatternTaskInf.mIgnoreMask);

			currPatternTaskInf.mPatternSize = currPatternTaskInf.mMatchMask.size();
		}

		std::vector<ThunderByteScan::PatternTaskInfo*> allPatternsInfop;

		for (auto& curr : allPatternsInfo)
			allPatternsInfop.push_back(curr.get());

		bool bFoundAll = LocalFindPatternBatch(allPatternsInfop, foundCallback);

		for (auto& currentPatternInfo : allPatternsInfo)
		{
			// Erasing, so next scan with similar patterns, 
			// dont find this garbage patterns as a result

			memset(currentPatternInfo->mMatchMask.data(), 0x0, currentPatternInfo->mPatternSize);
			memset(currentPatternInfo->mIgnoreMask.data(), 0x0, currentPatternInfo->mPatternSize);
		}

		return bFoundAll;
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
		std::vector<ThunderByteScan::PatternDesc> pattDesc{ pattern };

		return LocalFindPatternBatch(pattDesc, startAddr, endAddr, [&](PatternTaskInfo* pattTaskInf, uintptr_t result) {

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

		for (auto& patternDesc : patterns)
		{
			patternDesc.mpStopped = results.getAtomicBoolStoppedFor(patternDesc.mUID);

			if (results.HasResult(patternDesc.mUID, true)) // If Result is zero, we will override
				toRemove.insert(patternDesc.mUID);
			else
				results.setFirst(patternDesc.mUID, 0);
		}

		// Removing the ones who alredy have a result
		// We dont want to overwrite the cache
		patterns.erase(std::remove_if(patterns.begin(), patterns.end(), [&](const PatternDesc& pattDesc) {
			return toRemove.count(pattDesc.mUID) > 0;
			}), patterns.end());

		return LocalFindPatternBatch(patterns, startAddr, endAddr, [&](PatternTaskInfo* patternTaskInfo, uintptr_t rslt) {
			// At this point, this callback was called 
			// becouse a result for patternTaskInfo was found

			if (results.HasResult(patternTaskInfo->mUID) == true)
			{
				// Seems we alredy found it, we dont care about another result
				// lets simply ignore it

				return false;
			}

			// at this point, a result for patternTaskInfo
			// was found, and it wasnt saved, lets call the callback to resolve it(if any) and save it

			if (patternTaskInfo->mResolveCallback)
				rslt = patternTaskInfo->mResolveCallback(rslt);

			results.setFirst(patternTaskInfo->mUID, rslt);
			results.setResultDescExFrom(patternTaskInfo->mUID, (*patternTaskInfo));

			// Since finding just first ocurrences, 
			// we dont want to find anymore
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
	inline bool LocalFindPatternBatch(const std::vector<PatternDesc>& _patterns, uintptr_t startAddr, uintptr_t endAddr, BatchPatternsScanResults& results)
	{
		std::vector<PatternDesc> patterns = _patterns;

		for (PatternDesc& pd : patterns)
			pd.mpStopped = results.getAtomicBoolStoppedFor(pd.mUID);

		return LocalFindPatternBatch(patterns, startAddr, endAddr, [&](PatternTaskInfo* patternTaskInfo, uintptr_t rslt) {
			if (patternTaskInfo->mResolveCallback)
				rslt = patternTaskInfo->mResolveCallback(rslt);

			results.getResults(patternTaskInfo->mUID).emplace_back(rslt);
			return true;
			});
	}

	template<typename T>
	uintptr_t VecStart(const std::vector<T>& vec)
	{
		if (vec.size() < 1)
			return 0x0;

		return (uintptr_t)vec.data();
	}

	template<typename T>
	uintptr_t VecEnd(const std::vector<T>& vec)
	{
		if (vec.size() < 1)
			return 0x0;

		return (uintptr_t)(vec.data() + vec.size());
	}
}
