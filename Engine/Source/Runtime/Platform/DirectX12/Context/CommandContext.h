#pragma once
#include "../Pipline/RootSignature.h"
#include "../Pipline/PiplineState.h"
#include "../Core/DynamicDescriptorHeap.h"
#include "../Core/LinearAllocator.h"
#include "../Core/GraphicsCommon.h"
#include "../Core/CommandSignature.h"
#include "../Buffer/GpuBuffer.h"
#include "../Buffer/PixelBuffer.h"
#include "../Core/CommandListManager.h"

namespace AtomEngine
{
	class ColorBuffer;
	class DepthBuffer;
	class Texture;
	class GraphicsContext;
	class ComputeContext;
	class UploadBuffer;
	class ReadbackBuffer;

	struct DWParam
	{
		DWParam(FLOAT f) : Float(f) {}
		DWParam(UINT u) : Uint(u) {}
		DWParam(INT i) : Int(i) {}

		void operator= (FLOAT f) { Float = f; }
		void operator= (UINT u) { Uint = u; }
		void operator= (INT i) { Int = i; }

		union
		{
			FLOAT Float;
			UINT Uint;
			INT Int;
		};
	};

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

	class ContextManager
	{
	public:
		ContextManager(void) {}

		CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE Type);
		void FreeContext(CommandContext*);
		void DestroyAllContexts();

	private:
		std::vector<std::unique_ptr<CommandContext> > sm_ContextPool[4];
		std::queue<CommandContext*> sm_AvailableContexts[4];
		std::mutex sm_ContextAllocationMutex;
	};

	class CommandContext
	{
		friend class ContextManager;
	private:
		CommandContext(D3D12_COMMAND_LIST_TYPE Type);
		
		void Reset(void);

		CommandContext(CommandContext&) = delete;
		CommandContext& operator=(CommandContext&) = delete;
		CommandContext(CommandContext&&) = delete;
		CommandContext&& operator=(CommandContext&&) = delete;
	public:
		virtual ~CommandContext(void);

		static void DestroyAllContexts(void);

		static CommandContext& Begin(const std::wstring ID = L"");

		//既存のコマンドをGPUにフラッシュしますが、コンテキストは維持。
		uint64_t Flush(bool WaitForCompletion = false);

		// 既存のコマンドをフラッシュし、現在のコンテキストを解放する
		uint64_t Finish(bool WaitForCompletion = false);

		// コマンドリストとコマンドアロケータを予約してレンダリングを準備する
		void Initialize(void);

		GraphicsContext& GetGraphicsContext()
		{
			ASSERT(mType != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics");
			return reinterpret_cast<GraphicsContext&>(*this);
		}

		ComputeContext& GetComputeContext()
		{
			return reinterpret_cast<ComputeContext&>(*this);
		}

		ID3D12GraphicsCommandList5* GetCommandList()
		{
			return mCommandList;
		}

		void CopyBuffer(GpuResource& Dest, GpuResource& Src);
		void CopyBufferRegion(GpuResource& Dest, size_t DestOffset, GpuResource& Src, size_t SrcOffset, size_t NumBytes);
		void CopySubresource(GpuResource& Dest, UINT DestSubIndex, GpuResource& Src, UINT SrcSubIndex);
		void CopyCounter(GpuResource& Dest, size_t DestOffset, StructuredBuffer& Src);
		void CopyTextureRegion(GpuResource& Dest, UINT x, UINT y, UINT z, GpuResource& Source, RECT& rect);
		void ResetCounter(StructuredBuffer& Buf, uint32_t Value = 0);

		// リードバックバッファを作成し、そこにテクスチャをコピー、
		// バイト単位で返す
		uint32_t ReadbackTexture(ReadbackBuffer& DstBuffer, PixelBuffer& SrcBuffer);

		DynAlloc ReserveUploadMemory(size_t SizeInBytes)
		{
			return mCpuLinearAllocator.Allocate(SizeInBytes);
		}

		static void InitializeTexture(GpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
		static void InitializeBuffer(GpuBuffer& Dest, const void* Data, size_t NumBytes, size_t DestOffset = 0);
		static void InitializeBuffer(GpuBuffer& Dest, const UploadBuffer& Src, size_t SrcOffset, size_t NumBytes = -1, size_t DestOffset = 0);
		static void InitializeTextureArraySlice(GpuResource& Dest, UINT SliceIndex, GpuResource& Src);

		void WriteBuffer(GpuResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes);
		void FillBuffer(GpuResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes);

		void TransitionResource(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
		void BeginResourceTransition(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
		void InsertUAVBarrier(GpuResource& Resource, bool FlushImmediate = false);
		void InsertAliasBarrier(GpuResource& Before, GpuResource& After, bool FlushImmediate = false);
		inline void FlushResourceBarriers(void);

		void InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx);
		void ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries);

		void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
		void SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);
		void SetPipelineState(const PSO& PSO);

		void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

	protected:

		void BindDescriptorHeaps(void);

		CommandListManager* mOwningManager;
		ID3D12GraphicsCommandList5* mCommandList;
		ID3D12CommandAllocator* mCurrentAllocator;

		ID3D12RootSignature* mCurGraphicsRootSignature;
		ID3D12RootSignature* mCurComputeRootSignature;
		ID3D12PipelineState* mCurPipelineState;

		DynamicDescriptorHeap mDynamicViewDescriptorHeap;		// HEAP_TYPE_CBV_SRV_UAV
		DynamicDescriptorHeap mDynamicSamplerDescriptorHeap;	// HEAP_TYPE_SAMPLER

		D3D12_RESOURCE_BARRIER mResourceBarrierBuffer[16];
		UINT mNumBarriersToFlush;

		ID3D12DescriptorHeap* mCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		LinearAllocator mCpuLinearAllocator;
		LinearAllocator mGpuLinearAllocator;

		std::wstring mID;
		void SetID(const std::wstring& ID) { mID = ID; }

		D3D12_COMMAND_LIST_TYPE mType;
	};


    inline void CommandContext::FlushResourceBarriers(void)
    {
		if (mNumBarriersToFlush > 0)
		{
			mCommandList->ResourceBarrier(mNumBarriersToFlush, mResourceBarrierBuffer);
			mNumBarriersToFlush = 0;
		}
    }

    inline void CommandContext::SetPipelineState(const PSO& PSO)
    {
        ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
        if (PipelineState == mCurPipelineState)
            return;

        mCommandList->SetPipelineState(PipelineState);
        mCurPipelineState = PipelineState;
    }

    inline void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr)
    {
        if (mCurrentDescriptorHeaps[Type] != HeapPtr)
        {
            mCurrentDescriptorHeaps[Type] = HeapPtr;
            BindDescriptorHeaps();
        }
    }

    inline void CommandContext::SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[])
    {
        bool AnyChanged = false;

        for (UINT i = 0; i < HeapCount; ++i)
        {
            if (mCurrentDescriptorHeaps[Type[i]] != HeapPtrs[i])
            {
                mCurrentDescriptorHeaps[Type[i]] = HeapPtrs[i];
                AnyChanged = true;
            }
        }

        if (AnyChanged)
            BindDescriptorHeaps();
    }

    inline void CommandContext::SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
    {
        mCommandList->SetPredication(Buffer, BufferOffset, Op);
    }

    inline void CommandContext::CopyBuffer(GpuResource& Dest, GpuResource& Src)
    {
        TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        mCommandList->CopyResource(Dest.GetResource(), Src.GetResource());
    }

    inline void CommandContext::CopyBufferRegion(GpuResource& Dest, size_t DestOffset, GpuResource& Src, size_t SrcOffset, size_t NumBytes)
    {
        TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        //TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        mCommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetResource(), SrcOffset, NumBytes);
    }

    inline void CommandContext::CopyCounter(GpuResource& Dest, size_t DestOffset, StructuredBuffer& Src)
    {
        TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(Src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        mCommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetCounterBuffer().GetResource(), 0, 4);
    }

    inline void CommandContext::CopyTextureRegion(GpuResource& Dest, UINT x, UINT y, UINT z, GpuResource& Source, RECT& Rect)
    {
        TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();

        D3D12_TEXTURE_COPY_LOCATION destLoc = CD3DX12_TEXTURE_COPY_LOCATION(Dest.GetResource(), 0);
        D3D12_TEXTURE_COPY_LOCATION srcLoc = CD3DX12_TEXTURE_COPY_LOCATION(Source.GetResource(), 0);

        D3D12_BOX box = {};
        box.back = 1;
        box.left = Rect.left;
        box.right = Rect.right;
        box.top = Rect.top;
        box.bottom = Rect.bottom;

        mCommandList->CopyTextureRegion(&destLoc, x, y, z, &srcLoc, &box);
    }

    inline void CommandContext::ResetCounter(StructuredBuffer& Buf, uint32_t Value)
    {
        FillBuffer(Buf.GetCounterBuffer(), 0, Value, sizeof(uint32_t));
        TransitionResource(Buf.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    inline void CommandContext::InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx)
    {
        mCommandList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
    }

    inline void CommandContext::ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries)
    {
        mCommandList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
    }

}


