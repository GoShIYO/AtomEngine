#pragma once
#include "CommandContext.h"

namespace AtomEngine
{

	class GraphicsContext : public CommandContext
	{
	public:

		static GraphicsContext& Begin(const std::wstring& ID = L"")
		{
			return CommandContext::Begin(ID).GetGraphicsContext();
		}

		void ClearUAV(GpuBuffer& Target);
		void ClearUAV(ColorBuffer& Target);
		void ClearColor(ColorBuffer& Target, D3D12_RECT* Rect = nullptr);
		void ClearColor(ColorBuffer& Target, float Colour[4], D3D12_RECT* Rect = nullptr);
		void ClearDepth(DepthBuffer& Target);
		void ClearStencil(DepthBuffer& Target);
		void ClearDepthAndStencil(DepthBuffer& Target);

		void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
		void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
		void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset);

		void SetRootSignature(const RootSignature& RootSig);

		void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
		void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
		void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV) { SetRenderTargets(1, &RTV); }
		void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(1, &RTV, DSV); }
		void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(0, nullptr, DSV); }

		void SetViewport(const D3D12_VIEWPORT& vp);
		void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
		void SetScissor(const D3D12_RECT& rect);
		void SetScissor(UINT left, UINT top, UINT right, UINT bottom);
		void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
		void SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h);
		void SetStencilRef(UINT StencilRef);
		void SetBlendFactor(Color BlendFactor);
		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

		void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
		void SetConstant(UINT RootIndex, UINT Offset, DWParam Val);
		void SetConstants(UINT RootIndex, DWParam X);
		void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
		void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
		void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
		void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
		void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData);
		void SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset = 0);
		void SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset = 0);
		void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

		void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
		void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
		void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
		void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

		void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView);
		void SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView);
		void SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[]);
		void SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData);
		void SetDynamicIB(size_t IndexCount, const uint16_t* IBData);
		void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);

		void Draw(UINT VertexCount, UINT VertexStartOffset = 0);
		void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
		void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
			UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);
		void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
			INT BaseVertexLocation, UINT StartInstanceLocation);
		void DrawIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);
		void ExecuteIndirect(CommandSignature& CommandSig, GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
			uint32_t MaxCommands = 1, GpuBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);

	private:
	};


	inline void GraphicsContext::SetRootSignature(const RootSignature& RootSig)
	{
		if (RootSig.GetSignature() == mCurGraphicsRootSignature)
			return;

		mCommandList->SetGraphicsRootSignature(mCurGraphicsRootSignature = RootSig.GetSignature());

		mDynamicViewDescriptorHeap.ParseGraphicsRootSignature(RootSig);
		mDynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(RootSig);
	}


	inline void GraphicsContext::SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h)
	{
		SetViewport((float)x, (float)y, (float)w, (float)h);
		SetScissor(x, y, x + w, y + h);
	}

	inline void GraphicsContext::SetScissor(UINT left, UINT top, UINT right, UINT bottom)
	{
		SetScissor(CD3DX12_RECT(left, top, right, bottom));
	}

	inline void GraphicsContext::SetStencilRef(UINT ref)
	{
		mCommandList->OMSetStencilRef(ref);
	}

	inline void GraphicsContext::SetBlendFactor(Color BlendFactor)
	{
		mCommandList->OMSetBlendFactor(BlendFactor.GetPtr());
	}

	inline void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
	{
		mCommandList->IASetPrimitiveTopology(Topology);
	}

	inline void GraphicsContext::SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
	{
		mCommandList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
	}

	inline void GraphicsContext::SetConstant(UINT RootEntry, UINT Offset, DWParam Val)
	{
		mCommandList->SetGraphicsRoot32BitConstant(RootEntry, Val.Uint, Offset);
	}

	inline void GraphicsContext::SetConstants(UINT RootIndex, DWParam X)
	{
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
	}

	inline void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y)
	{
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
	}

	inline void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
	{
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
	}

	inline void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
	{
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
		mCommandList->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
	}

	inline void GraphicsContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
	{
		mCommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
	}

	inline void GraphicsContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
	{
		if (!BufferData || BufferSize == 0)
		{
			ASSERT(false && "Invalid buffer data passed to SetDynamicConstantBufferView");
			return;
		}

		ASSERT(IsAligned(BufferData, 16));

		DynAlloc cb = mCpuLinearAllocator.Allocate(BufferSize);
		memcpy(cb.DataPtr, BufferData, BufferSize);
		mCommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
	}


	inline void GraphicsContext::SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VertexData)
	{
		ASSERT(VertexData != nullptr && IsAligned(VertexData, 16));

		size_t BufferSize = AlignUp(NumVertices * VertexStride, 16);
		DynAlloc vb = mCpuLinearAllocator.Allocate(BufferSize);

		SIMDMemCopy(vb.DataPtr, VertexData, BufferSize >> 4);

		D3D12_VERTEX_BUFFER_VIEW VBView;
		VBView.BufferLocation = vb.GpuAddress;
		VBView.SizeInBytes = (UINT)BufferSize;
		VBView.StrideInBytes = (UINT)VertexStride;

		mCommandList->IASetVertexBuffers(Slot, 1, &VBView);
	}

	inline void GraphicsContext::SetDynamicIB(size_t IndexCount, const uint16_t* IndexData)
	{
		ASSERT(IndexData != nullptr && IsAligned(IndexData, 16));

		size_t BufferSize = AlignUp(IndexCount * sizeof(uint16_t), 16);
		DynAlloc ib = mCpuLinearAllocator.Allocate(BufferSize);

		SIMDMemCopy(ib.DataPtr, IndexData, BufferSize >> 4);

		D3D12_INDEX_BUFFER_VIEW IBView;
		IBView.BufferLocation = ib.GpuAddress;
		IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
		IBView.Format = DXGI_FORMAT_R16_UINT;

		mCommandList->IASetIndexBuffer(&IBView);
	}

	inline void GraphicsContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
	{
		ASSERT(BufferData != nullptr && IsAligned(BufferData, 16));
		DynAlloc cb = mCpuLinearAllocator.Allocate(BufferSize);
		SIMDMemCopy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16) >> 4);
		mCommandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress);
	}


	inline void GraphicsContext::SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset)
	{
		ASSERT((SRV.mUsageState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
		mCommandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
	}

	inline void GraphicsContext::SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset)
	{
		ASSERT((UAV.mUsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
		mCommandList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
	}

	inline void GraphicsContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
	{
		SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
	}

	inline void GraphicsContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		mDynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
	}

	inline void GraphicsContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
	{
		SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
	}

	inline void GraphicsContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		mDynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
	}

	inline void GraphicsContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
	{
		mCommandList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
	}

	inline void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
	{
		mCommandList->IASetIndexBuffer(&IBView);
	}

	inline void GraphicsContext::SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
	{
		SetVertexBuffers(Slot, 1, &VBView);
	}

	inline void GraphicsContext::SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
	{
		mCommandList->IASetVertexBuffers(StartSlot, Count, VBViews);
	}

	inline void GraphicsContext::Draw(UINT VertexCount, UINT VertexStartOffset)
	{
		DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
	}

	inline void GraphicsContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
	{
		DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
	}

	inline void GraphicsContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
		UINT StartVertexLocation, UINT StartInstanceLocation)
	{
		FlushResourceBarriers();
		mDynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(mCommandList);
		mDynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(mCommandList);
		mCommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	inline void GraphicsContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
		INT BaseVertexLocation, UINT StartInstanceLocation)
	{
		FlushResourceBarriers();
		mDynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(mCommandList);
		mDynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(mCommandList);
		mCommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}

	inline void GraphicsContext::ExecuteIndirect(CommandSignature& CommandSig,
		GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
		uint32_t MaxCommands, GpuBuffer* CommandCounterBuffer, uint64_t CounterOffset)
	{
		FlushResourceBarriers();
		mDynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(mCommandList);
		mDynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(mCommandList);
		mCommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
			ArgumentBuffer.GetResource(), ArgumentStartOffset,
			CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
	}

	inline void GraphicsContext::DrawIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
	{
		ExecuteIndirect(DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
	}

}


