#pragma once
#include "../Buffer/GpuResource.h"
#include <vector>
#include <queue>
#include <mutex>

#define DEFAULT_ALIGN 256

namespace AtomEngine
{
    // 様々な種類の割り当てにはNULLポインタが含まれる場合があります。不明な場合は、逆参照する前に確認する
    struct DynAlloc
    {
        DynAlloc(GpuResource& BaseResource, size_t ThisOffset, size_t ThisSize)
            : Buffer(BaseResource), Offset(ThisOffset), Size(ThisSize)
        {
        }

        GpuResource& Buffer;	                // このメモリに関連付けられた D3D バッファ。
        size_t Offset;			                // バッファリソースの先頭からのオフセット
        size_t Size;			                // この割り当ての予約サイズ
        void* DataPtr;			                // CPU 書き込み可能アドレス
        D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;	// GPU が認識できるアドレス
    };

    class LinearAllocationPage : public GpuResource
    {
    public:
        LinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Usage) : GpuResource()
        {
            mResource.Attach(pResource);
            mUsageState = Usage;
            mGpuVirtualAddress = mResource->GetGPUVirtualAddress();
            mResource->Map(0, nullptr, &mCpuVirtualAddress);
        }

        ~LinearAllocationPage()
        {
            Unmap();
        }

        void Map(void)
        {
            if (mCpuVirtualAddress == nullptr)
            {
                mResource->Map(0, nullptr, &mCpuVirtualAddress);
            }
        }

        void Unmap(void)
        {
            if (mCpuVirtualAddress != nullptr)
            {
                mResource->Unmap(0, nullptr);
                mCpuVirtualAddress = nullptr;
            }
        }

        void* mCpuVirtualAddress;
        D3D12_GPU_VIRTUAL_ADDRESS mGpuVirtualAddress;
    };

    enum LinearAllocatorType
    {
        kInvalidAllocator = -1,

        kGpuExclusive = 0,		// DEFAULT   GPU-writeable (via UAV)
        kCpuWritable = 1,		// UPLOAD CPU-writeable (but write combined)

        kNumAllocatorTypes
    };

    enum
    {
        kGpuAllocatorPageSize = 0x10000,	// 64K
        kCpuAllocatorPageSize = 0x200000	// 2MB
    };

    class LinearAllocatorPageManager
    {
    public:

        LinearAllocatorPageManager();
        LinearAllocationPage* RequestPage(void);
        LinearAllocationPage* CreateNewPage(size_t PageSize = 0);

        // 破棄されたページはリサイクルされます。これは固定サイズのページの場合です。
        void DiscardPages(uint64_t FenceID, const std::vector<LinearAllocationPage*>& Pages);

        // 解放されたページは、フェンスを通過すると破棄されます。これは、1回限りの「大きな」ページ用。
        void FreeLargePages(uint64_t FenceID, const std::vector<LinearAllocationPage*>& Pages);

        void Destroy(void) { mPagePool.clear(); }

    private:

        static LinearAllocatorType sAutoType;

        LinearAllocatorType m_AllocationType;
        std::vector<std::unique_ptr<LinearAllocationPage> > mPagePool;
        std::queue<std::pair<uint64_t, LinearAllocationPage*> > mRetiredPages;
        std::queue<std::pair<uint64_t, LinearAllocationPage*> > mDeletionQueue;
        std::queue<LinearAllocationPage*> mAvailablePages;
        std::mutex mMutex;
    };

    class LinearAllocator
    {
    public:

        LinearAllocator(LinearAllocatorType Type) : mAllocationType(Type), mPageSize(0), mCurOffset(~(size_t)0), mCurPage(nullptr)
        {
            ASSERT(Type > kInvalidAllocator && Type < kNumAllocatorTypes);
            mPageSize = (Type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
        }

        DynAlloc Allocate(size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN);

        void CleanupUsedPages(uint64_t FenceID);

        static void DestroyAll(void)
        {
            sPageManager[0].Destroy();
            sPageManager[1].Destroy();
        }

    private:

        DynAlloc AllocateLargePage(size_t SizeInBytes);

        static LinearAllocatorPageManager sPageManager[2];

        LinearAllocatorType mAllocationType;
        size_t mPageSize;
        size_t mCurOffset;
        LinearAllocationPage* mCurPage;
        std::vector<LinearAllocationPage*> mRetiredPage;
        std::vector<LinearAllocationPage*> mLargePageList;
    };

}
