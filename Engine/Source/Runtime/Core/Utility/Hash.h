#pragma once
#include <cstddef>
#include <functional>
#include "Runtime/Core/Math/Math.h"

namespace AtomEngine
{
// これには Intel Nehalem (2008年11月) および AMD Bulldozer (2011年10月) プロセッサに搭載されている SSE4.2 が必要です。
// ランタイムチェックを実装することもできますが、Windows 10 で DirectX 12 を使用する人は比較的新しいマシンを使用していると想定します。
#ifdef _M_X64
#define ENABLE_SSE_CRC32 1
#else
#define ENABLE_SSE_CRC32 0
#endif

#if ENABLE_SSE_CRC32
#pragma intrinsic(_mm_crc32_u32)
#pragma intrinsic(_mm_crc32_u64)
#endif

	inline size_t HashRange(const uint32_t* const Begin, const uint32_t* const End, size_t Hash)
	{
#if ENABLE_SSE_CRC32
		const uint64_t* Iter64 = (const uint64_t*)AlignUp(Begin, 8);
		const uint64_t* const End64 = (const uint64_t* const)AlignDown(End, 8);

		// 64ビットに揃えていない場合は、単一のu32から始める
		if ((uint32_t*)Iter64 > Begin)
			Hash = _mm_crc32_u32((uint32_t)Hash, *Begin);

		// 連続するu64値をループ処理する
		while (Iter64 < End64)
			Hash = _mm_crc32_u64((uint64_t)Hash, *Iter64++);

		// 32ビットの余りがある場合はそれを累算する
		if ((uint32_t*)Iter64 < End)
			Hash = _mm_crc32_u32((uint32_t)Hash, *(uint32_t*)Iter64);
#else
		//SSE4.2 を搭載していない CPU 向けの安価なハッシュ
		for (const uint32_t* Iter = Begin; Iter < End; ++Iter)
			Hash = 16777619U * Hash ^ *Iter;
#endif

		return Hash;
	}

	template <typename T> inline size_t HashState(const T* StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
	{
		static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
		return HashRange((uint32_t*)StateDesc, (uint32_t*)(StateDesc + Count), Hash);
	}

}


