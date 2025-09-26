#pragma once
#include "CommandAllocatorPool.h"

namespace AtomEngine
{
	class CommandQueue
	{
        friend class CommandListManager;
        friend class CommandContext;
    public:

        CommandQueue(D3D12_COMMAND_LIST_TYPE Type);
        ~CommandQueue();

        void Create(ID3D12Device* pDevice);
        void Shutdown();

        inline bool IsReady()
        {
            return mCommandQueue != nullptr;
        }

        uint64_t IncrementFence(void);
        bool IsFenceComplete(uint64_t FenceValue);
        void StallForFence(uint64_t FenceValue);
        void StallForProducer(CommandQueue& Producer);
        void WaitForFence(uint64_t FenceValue);
        void WaitForIdle(void) { WaitForFence(IncrementFence()); }

        ID3D12CommandQueue* GetCommandQueue() { return mCommandQueue; }

        uint64_t GetNextFenceValue() { return mNextFenceValue; }

    private:

        uint64_t ExecuteCommandList(ID3D12CommandList* List);
        ID3D12CommandAllocator* RequestAllocator(void);
        void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

        ID3D12CommandQueue* mCommandQueue;

        const D3D12_COMMAND_LIST_TYPE mType;

        CommandAllocatorPool mAllocatorPool;
        std::mutex mFenceMutex;
        std::mutex mEventMutex;

        //これらのオブジェクトの有効期間は記述子キャッシュによって管理されます
        ID3D12Fence* mFence;
        uint64_t mNextFenceValue;
        uint64_t mLastCompletedFenceValue;
        HANDLE mFenceEventHandle = nullptr;

	};
	class CommandListManager
	{
        friend class CommandContext;

    public:
        CommandListManager();
        ~CommandListManager();

        void Create(ID3D12Device* pDevice);
        void Shutdown();

        CommandQueue& GetGraphicsQueue(void) { return mGraphicsQueue; }
        CommandQueue& GetComputeQueue(void) { return mComputeQueue; }
        CommandQueue& GetCopyQueue(void) { return mCopyQueue; }

        CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
        {
            switch (Type)
            {
            case D3D12_COMMAND_LIST_TYPE_COMPUTE: return mComputeQueue;
            case D3D12_COMMAND_LIST_TYPE_COPY: return mCopyQueue;
            default: return mGraphicsQueue;
            }
        }

        ID3D12CommandQueue* GetCommandQueue()
        {
            return mGraphicsQueue.GetCommandQueue();
        }

        void CreateNewCommandList(
            D3D12_COMMAND_LIST_TYPE Type,
            ID3D12GraphicsCommandList** List,
            ID3D12CommandAllocator** Allocator);

        // フェンスにすでに到達しているかどうかを確認するテスト
        bool IsFenceComplete(uint64_t FenceValue)
        {
            return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
        }

        // GPUが空になるまで待つ
        void WaitForFence(uint64_t FenceValue);

        // CPUはすべてのコマンドキューが空になるまで待機します
        void IdleGPU(void)
        {
            mGraphicsQueue.WaitForIdle();
            mComputeQueue.WaitForIdle();
            mCopyQueue.WaitForIdle();
        }

    private:

        ID3D12Device* mDevice;

        CommandQueue mGraphicsQueue;
        CommandQueue mComputeQueue;
        CommandQueue mCopyQueue;
	};
}


