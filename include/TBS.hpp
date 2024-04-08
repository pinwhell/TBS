#pragma once

#include <string>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <vector>

#ifdef TBS_MT
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#endif

#ifdef TBS_USE_SSE2
#ifndef TBS_IMPL_SSE2
#define TBS_IMPL_SSE2
#endif
#endif

#ifdef TBS_IMPL_SSE2
#include <emmintrin.h>
#endif

namespace TBS {
	using U64 = unsigned long long;
	using U32 = unsigned int;
	using UByte = unsigned char;
	using UShort = unsigned short;
	using ULong = unsigned long;
	using UPtr = uintptr_t;

#ifdef TBS_MT
	namespace Thread {
		class Pool {
		public:
			inline Pool(size_t threads = std::thread::hardware_concurrency()) : mbStopped(false)
			{
				auto workerTask = [this] {
					for (;;)
					{
						std::function<void()> task;
						{
							std::unique_lock<std::mutex> lock(mTasksMtx);
							mWorkersCondVar.wait(lock, [this] { return mbStopped || !mTasks.empty(); });
							if (mbStopped && mTasks.empty())
								return;
							task = std::move(mTasks.front());
							mTasks.pop();
						}
						task();
					}
					};

				for (size_t i = 0; i < threads; ++i)
					mWorkers.emplace_back(workerTask);
			}

			template<class F, class... Args>
			inline void enqueue(F&& f, Args&&... args) {
				{
					std::unique_lock<std::mutex> lock(mTasksMtx);
					mTasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
				}
				mWorkersCondVar.notify_one();
			}

			inline ~Pool() {
				{
					std::unique_lock<std::mutex> lock(mTasksMtx);
					mbStopped = true;
				}
				mWorkersCondVar.notify_all();
				for (std::thread& worker : mWorkers)
					worker.join();
			}

		private:
			std::vector<std::thread> mWorkers;
			std::queue<std::function<void()>> mTasks;
			std::mutex mTasksMtx;
			std::condition_variable mWorkersCondVar;
			bool mbStopped;
		};
	}
#endif

	namespace Memory {

		inline bool CompareWithMaskWord(const UByte* chunk1, const UByte* chunk2, size_t len, const UByte* wildCardMask) {
			// UPtr ==> Platform Word Length
			size_t wordLen = len / sizeof(UPtr); // Calculate length in words

			for (size_t i = 0; i < wordLen; i++) {
				// Convert byte index to word index
				UPtr wordMask = ~*((const UPtr*)wildCardMask + i);
				UPtr maskedChunk1 = *((const UPtr*)chunk1 + i) & wordMask;
				UPtr maskedChunk2 = *((const UPtr*)chunk2 + i) & wordMask;
				if (maskedChunk1 != maskedChunk2)
					return false;
			}

			size_t remainingBytes = len % sizeof(UPtr);
			if (remainingBytes > 0) {
				// Calculate the starting index of the last incomplete word
				size_t lastWordIndex = wordLen * sizeof(UPtr);
				for (size_t i = lastWordIndex; i < len; i++) {

					UByte b1 = chunk1[i] & ~wildCardMask[i];
					UByte b2 = chunk2[i] & ~wildCardMask[i];

					if (b1 != b2)
						return false;
				}
			}

			return true;
		}
#ifdef TBS_IMPL_SSE2
		namespace SSE2 {
			inline __m128i _mm_not_si128(__m128i value)
			{
				__m128i mask = _mm_set1_epi32(-1);
				return _mm_xor_si128(value, mask);
			}

			inline bool CompareWithMask(const UByte* chunk1, const UByte* chunk2, size_t len, const UByte* wildCardMask) {

				const size_t wordLen = len / sizeof(__m128i); // Calculate length in words

				for (size_t i = 0; i < wordLen; i++) {
					// Convert byte index to word index
					__m128i wordMask = _mm_not_si128(_mm_load_si128((const __m128i*)wildCardMask + i));
					__m128i maskedChunk1 = _mm_and_si128(_mm_load_si128((const __m128i*)chunk1 + i), wordMask);
					__m128i maskedChunk2 = _mm_and_si128(_mm_load_si128((const __m128i*)chunk2 + i), wordMask);

					if (_mm_movemask_epi8(_mm_cmpeq_epi32(maskedChunk1, maskedChunk2)) != 0xFFFF)
						return false;
				}

				const size_t lastWordIndex = wordLen * sizeof(__m128i);

				return CompareWithMaskWord(chunk1 + lastWordIndex, chunk2 + lastWordIndex, len % sizeof(__m128i), wildCardMask + lastWordIndex);
			}
		}
#endif

		inline bool CompareWithMask(const UByte* chunk1, const UByte* chunk2, size_t len, const UByte* wildCardMask)
		{
#ifdef TBS_USE_SSE2
			return SSE2::CompareWithMask(chunk1, chunk2, len, wildCardMask);
#endif
			return CompareWithMaskWord(chunk1, chunk2, len, wildCardMask);
		}
	}

	namespace Pattern
	{
		using Result = U64;
		using Results = std::vector<U64>;

		struct ParseResult {
			ParseResult()
				: mParseSuccess(false)
			{}

			operator bool()
			{
				return mParseSuccess;
			}

			std::vector<UByte> mPattern;
			std::vector<UByte> mWildcardMask;
			bool mParseSuccess;
		};

		inline bool Parse(const std::string& pattern, ParseResult& result)
		{
			result = ParseResult();

			if (pattern.empty())
				return true;

			std::stringstream ss(pattern);

			while (!ss.eof())
			{
				std::string str;
				ss >> str;

				if (str.size() > 2)
					return false;

				// At this point, pattern structure good so far

				bool bAnyWildCard = str.find("?") != std::string::npos;

				if (!bAnyWildCard)
				{
					// Not Wilcarding this byte
					result.mPattern.emplace_back(UByte(strtoull(str.c_str(), nullptr, 16)));
					result.mWildcardMask.emplace_back(UByte(0x00ull));
					continue;
				}

				// At this point, current Byte is wildcarded

				bool bFullByteWildcard = str.find("??") != std::string::npos || (bAnyWildCard && str.size() == 1);

				if (bFullByteWildcard)
				{
					// At this point we are dealing with Full Byte Wildcard Case
					result.mPattern.emplace_back(UByte(0x00u));
					result.mWildcardMask.emplace_back(UByte(0xFFu));
					continue;
				}

				// At this point we are dealing with Half Byte Wildcard Case, "X?" or "?X"

				if (str[0] == '?')
				{
					// At this point, we are dealing with High Part of Byte wildcarding "?X"
					str[0] = '0';
					result.mPattern.emplace_back(UByte(strtoull(str.c_str(), nullptr, 16)));
					result.mWildcardMask.emplace_back(UByte(0xF0u));
					continue;
				}

				// At this point, we are dealing with Low Part of Byte wildcarding "X?"
				str[1] = '0';
				result.mPattern.emplace_back(UByte(strtoull(str.c_str(), nullptr, 16)));
				result.mWildcardMask.emplace_back(UByte(0x0Fu));
			}

			return result.mParseSuccess = true;
		}

		inline bool Valid(const std::string& pattern)
		{
			ParseResult res;

			return Parse(pattern, res) && res;
		}

		struct Description;

		enum class EScan {
			SCAN_ALL,
			SCAN_FIRST
		};

		struct Description {
			struct Shared {
				struct ResultAccesor {
					ResultAccesor(Shared& sharedDesc)
						: mSharedDesc(sharedDesc)
					{}

					operator const Results& () const {
						return Results();
					}

					operator Result () const {
						if (mSharedDesc.mResult.size() < 1)
							return 0;

						return mSharedDesc.mResult[0];
					}

					const Results& Results() const
					{
						return mSharedDesc.mResult;
					}

					Shared& mSharedDesc;
				};

				Shared(EScan scanType)
					: mScanType(scanType)
					, mResultAccesor(*this)
				{}

#ifdef TBS_MT
				std::mutex mMutex;
				std::atomic<bool> mFinished;
#else
				bool mFinished;
#endif
				EScan mScanType;
				Results mResult;
				ResultAccesor mResultAccesor;
			};

			Description(Shared& shared, const std::string& uid, const std::string& pattern, const UByte* searchStart, const UByte* searchEnd, const std::vector<std::function<U64(Description&, U64)>>& transformers)
				: mShared(shared)
				, mUID(uid)
				, mPattern(pattern)
				, mStart(searchStart)
				, mEnd(searchEnd)
				, mTransforms(transformers)
			{
				Parse(mPattern, mParsed);
			}

			operator bool()
			{
				return mParsed;
			}

			std::string mPattern;
			std::string mUID;
			std::vector<std::function<U64(Description&, U64)>> mTransforms;
			ParseResult mParsed;
			Shared& mShared;
			const UByte* mStart;
			const UByte* mEnd;
		};

		inline bool Scan(Description& desc)
		{
			if (desc.mShared.mFinished)
				return desc.mShared.mResult.empty() == false;

			if (!desc.mStart || !desc.mEnd)
				return false;

			const size_t patternLen = desc.mParsed.mPattern.size();

			for (const UByte* i = desc.mStart; i + patternLen - 1 < desc.mEnd && !desc.mShared.mFinished; i++)
			{
				if (Memory::CompareWithMask(i, desc.mParsed.mPattern.data(), patternLen, desc.mParsed.mWildcardMask.data()) == false)
					continue;

				U64 currMatch = (U64)i;

				// At this point, we found a match

				for (auto transform : desc.mTransforms)
					currMatch = transform(desc, currMatch);

				// At this point, match is properly user transformed
				// lets report it
#ifdef TBS_MT
				{
					std::lock_guard<std::mutex> resultReportLck(desc.mShared.mMutex);
#endif

					if (desc.mShared.mFinished)
						break;

					// At this point, we have the lock & we havent finished!
					// Lets directly push it

					desc.mShared.mResult.push_back(currMatch);

					if (desc.mShared.mScanType != EScan::SCAN_FIRST)
						continue;

					// At this point seems we are searching for a single result
					// lets break turn off the searching & break

					desc.mShared.mFinished = true;
					return true;
#ifdef TBS_MT
				}
#endif
			}

			return desc.mShared.mResult.empty() == false;
		}

		using SharedDescription = Description::Shared;
		using SharedResultAccesor = SharedDescription::ResultAccesor;
	}

	namespace Pattern {
		struct DescriptionBuilder {

			DescriptionBuilder(std::unordered_map<std::string, std::unique_ptr<Pattern::SharedDescription>>& sharedDescriptions)
				: mSharedDescriptions(sharedDescriptions)
				, mScanStart(0)
				, mScanEnd(0)
				, mScanType(EScan::SCAN_ALL)
			{}

			DescriptionBuilder& setPattern(const std::string& pattern)
			{
				mPattern = pattern;

				if (!mUID.empty())
					return *this;

				return setUID(pattern);
			}

			DescriptionBuilder& setUID(const std::string& uid)
			{
				mUID = uid;
				return *this;
			}

			template<typename T>
			DescriptionBuilder& setScanStart(T start)
			{
				mScanStart = (const UByte*)start;
				return *this;
			}

			template<typename T>
			DescriptionBuilder& setScanEnd(T end)
			{
				mScanEnd = (const UByte*)end;
				return *this;
			}

			DescriptionBuilder& AddTransformer(const std::function<U64(Description&, U64)>& transformer)
			{
				mTransformers.emplace_back(transformer);
				return *this;
			}

			DescriptionBuilder& setScanType(EScan type)
			{
				mScanType = type;
				return *this;
			}

			DescriptionBuilder& EnableScanFirst()
			{
				return setScanType(EScan::SCAN_FIRST);
			}

			DescriptionBuilder& EnableScanAll()
			{
				return setScanType(EScan::SCAN_FIRST);
			}

			DescriptionBuilder Clone()
			{
				return DescriptionBuilder(*this);
			}

			Description Build()
			{
				if (Pattern::Valid(mPattern) == false)
				{
					static SharedDescription nullSharedDesc(EScan::SCAN_ALL);
					static Description nullDescription(nullSharedDesc, "", "", 0, 0, {});
					return nullDescription;
				}

				if (mSharedDescriptions.find(mUID) == mSharedDescriptions.end())
					mSharedDescriptions[mUID] = std::make_unique<SharedDescription>(mScanType);

				return Description(*mSharedDescriptions[mUID], mUID, mPattern, mScanStart, mScanEnd, mTransformers);
			}

		private:
			std::unordered_map<std::string, std::unique_ptr<Pattern::SharedDescription>>& mSharedDescriptions;
			EScan mScanType;
			std::string mPattern;
			std::string mUID;
			const UByte* mScanStart;
			const UByte* mScanEnd;
			std::vector<std::function<U64(Description&, U64)>> mTransformers;
		};
	}

	struct State {

		State()
			: State(nullptr, nullptr)
		{}

		template<typename T, typename K>
		State(T defScanStart = (T)0, K defScanEnd = (K)0)
			: mDefaultScanStart((const UByte*)defScanStart)
			, mDefaultScanEnd((const UByte*)defScanEnd)
		{}

		State& AddPattern(Pattern::Description&& pattern)
		{
			mDescriptionts.emplace_back(pattern);
			return *this;
		}

		Pattern::DescriptionBuilder PatternBuilder()
		{
			return Pattern::DescriptionBuilder(mSharedDescriptions)
				.setScanStart(mDefaultScanStart)
				.setScanEnd(mDefaultScanEnd);
		}

		Pattern::SharedResultAccesor operator[](const std::string& uid) const
		{
			if (mSharedDescriptions.find(uid) != mSharedDescriptions.end())
				return *(mSharedDescriptions.at(uid));

			static Pattern::SharedDescription nullSharedDesc(Pattern::EScan::SCAN_ALL);

			return nullSharedDesc;
		}

		const UByte* mDefaultScanStart;
		const UByte* mDefaultScanEnd;
		std::unordered_map<std::string, std::unique_ptr<Pattern::SharedDescription>> mSharedDescriptions;
		std::vector<Pattern::Description> mDescriptionts;
	};

	bool Scan(State& state)
	{
		bool bAllFoundAny = true;

#ifdef TBS_MT
	{
		Thread::Pool threadPool;
#endif
		for (Pattern::Description& description : state.mDescriptionts)
		{
#ifdef TBS_MT
			threadPool.enqueue(
			[&](Pattern::Description& description)
			{
#endif
				Pattern::Scan(description);

#ifdef TBS_MT
			}, description);
#endif
		}
#ifdef TBS_MT
	}
#endif
		state.mDescriptionts.clear();

		for (auto& sharedDescKv : state.mSharedDescriptions)
			bAllFoundAny = bAllFoundAny && sharedDescKv.second->mResult.empty() == false;

		return bAllFoundAny;
	}
}
