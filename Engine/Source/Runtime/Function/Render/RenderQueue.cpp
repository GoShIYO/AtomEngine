#include "RenderQueue.h"
#include "RenderSystem.h"

#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Resource/Model.h"

#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"

namespace AtomEngine
{
	void RenderQueue::AddMesh(const Mesh& mesh, float distance,
		D3D12_GPU_VIRTUAL_ADDRESS meshCBV,
		D3D12_GPU_VIRTUAL_ADDRESS materialCBV,
		D3D12_GPU_VIRTUAL_ADDRESS bufferPtr,
		const Joint* skeleton)
	{
		SortKey key;
		key.value = mSortObjects.size();

		bool alphaBlend = (mesh.psoFlags & PSOFlags::kAlphaBlend) == PSOFlags::kAlphaBlend;
		bool alphaTest = (mesh.psoFlags & PSOFlags::kAlphaTest) == PSOFlags::kAlphaTest;
		bool skinned = (mesh.psoFlags & PSOFlags::kHasSkin) == PSOFlags::kHasSkin;
		uint64_t depthPSO = (skinned ? 2 : 0) + (alphaTest ? 1 : 0);

		union float_or_int { float f; uint32_t u; } dist;
		dist.f = std::max(distance, 0.0f);

		if (mBatchType == kShadows)
		{
			if (alphaBlend)
				return;

			key.typeID = kZPass;
			key.psoIdx = depthPSO + 4;
			key.key = dist.u;
			mSortKeys.push_back(key.value);
			mTypeCounts[kZPass]++;
		}
		else if (mesh.psoFlags & PSOFlags::kAlphaBlend)
		{
			key.typeID = kTransparent;
			key.psoIdx = mesh.pso;
			key.key = ~dist.u;
			mSortKeys.push_back(key.value);
			mTypeCounts[kTransparent]++;
		}
		else if (alphaTest)
		{
			key.typeID = kZPass;
			key.psoIdx = depthPSO;
			key.key = dist.u;
			mSortKeys.push_back(key.value);
			mTypeCounts[kZPass]++;

			key.typeID = kOpaque;
			key.psoIdx = mesh.pso + 1;
			key.key = dist.u;
			mSortKeys.push_back(key.value);
			mTypeCounts[kOpaque]++;
		}
		else
		{
			key.typeID = kOpaque;
			key.psoIdx = mesh.pso;
			key.key = dist.u;
			mSortKeys.push_back(key.value);
			mTypeCounts[kOpaque]++;
		}

		SortObject object = { &mesh, skeleton, meshCBV, materialCBV, bufferPtr };
		mSortObjects.push_back(object);
	}

	void RenderQueue::Sort()
	{
		std::sort(mSortKeys.begin(), mSortKeys.end(), [&](uint64_t a, uint64_t b) { return a < b; });
	}

	void RenderQueue::RenderMeshes(
		DrawType type,
		GraphicsContext& context,
		GlobalConstants& globals)
	{
		ASSERT(mDSV != nullptr);

		auto textureHeap = RenderSystem::GetTextureHeap();
		auto samplerHeap = RenderSystem::GetSamplerHeap();
		auto commonSRVs = RenderSystem::GetCommonTextures();

		context.SetRootSignature(RenderSystem::GetRootSignature());
		context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textureHeap.GetHeapPointer());
		context.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerHeap.GetHeapPointer());

		// 共通テクスチャを設定
		context.SetDescriptorTable(kCommonSRVs, commonSRVs);

		// 共通コンストバッファを設定
		globals.ViewProjMatrix = mCamera->GetViewProjMatrix();
		globals.CameraPos = mCamera->GetPosition();
		globals.IBLRange = RenderSystem::GetIBLRange();
		globals.IBLBias = RenderSystem::GetIBLBias();
		context.SetDynamicConstantBufferView(kCommonCBV, sizeof(GlobalConstants), &globals);

		if (mBatchType == kShadows)
		{
			context.TransitionResource(*mDSV, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
			context.ClearDepth(*mDSV);
			context.SetDepthStencilTarget(mDSV->GetDSV());

			if (mViewport.Width == 0)
			{
				mViewport.TopLeftX = 0.0f;
				mViewport.TopLeftY = 0.0f;
				mViewport.Width = (float)mDSV->GetWidth();
				mViewport.Height = (float)mDSV->GetHeight();
				mViewport.MaxDepth = 1.0f;
				mViewport.MinDepth = 0.0f;

				mScissor.left = 1;
				mScissor.right = mDSV->GetWidth() - 2;
				mScissor.top = 1;
				mScissor.bottom = mDSV->GetHeight() - 2;
			}
		}
		else
		{

			ASSERT(mRTV != nullptr);
			ASSERT(mDSV->GetWidth() == mRTV->GetWidth());
			ASSERT(mDSV->GetHeight() == mRTV->GetHeight());

			if (mViewport.Width == 0)
			{
				mViewport.TopLeftX = 0.0f;
				mViewport.TopLeftY = 0.0f;
				mViewport.Width = (float)mDSV->GetWidth();
				mViewport.Height = (float)mDSV->GetHeight();
				mViewport.MaxDepth = 1.0f;
				mViewport.MinDepth = 0.0f;

				mScissor.left = 0;
				mScissor.right = mDSV->GetWidth();
				mScissor.top = 0;
				mScissor.bottom = mDSV->GetWidth();
			}
		}

		for (; mCurrentType <= type; mCurrentType = (DrawType)(mCurrentType + 1))
		{
			const uint32_t typeCount = mTypeCounts[mCurrentType];
			if (typeCount == 0)
				continue;

			if (mBatchType == kDefault)
			{
				switch (mCurrentType)
				{
				case kZPass:
					context.TransitionResource(*mDSV, D3D12_RESOURCE_STATE_DEPTH_WRITE);
					context.SetDepthStencilTarget(mDSV->GetDSV());
					break;
				case kOpaque:
					context.TransitionResource(*mDSV, D3D12_RESOURCE_STATE_DEPTH_READ);
					context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
					context.SetRenderTarget(gSceneColorBuffer.GetRTV(), mDSV->GetDSV_DepthReadOnly());
					break;
				case kTransparent:
					context.TransitionResource(*mDSV, D3D12_RESOURCE_STATE_DEPTH_READ);
					context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
					context.SetRenderTarget(gSceneColorBuffer.GetRTV(), mDSV->GetDSV_DepthReadOnly());
					break;
				}
			}

			context.SetViewportAndScissor(mViewport, mScissor);
			context.FlushResourceBarriers();

			const uint32_t lastDraw = mCurrentDraw + typeCount;

			while (mCurrentDraw < lastDraw)
			{
				SortKey key;
				key.value = mSortKeys[mCurrentDraw];
				const SortObject& object = mSortObjects[key.objectIdx];
				const Mesh& mesh = *object.mesh;

				context.SetConstantBuffer(kMeshConstants, object.meshCBV);
				context.SetConstantBuffer(kMaterialConstants, object.materialCBV);
				context.SetDescriptorTable(kMaterialSRVs, textureHeap[mesh.srvTable]);
				context.SetDescriptorTable(kMaterialSamplers, samplerHeap[mesh.samplerTable]);
				if (mesh.numJoints > 0)
				{
					ASSERT(object.skeleton != nullptr, "Unspecified joint matrix array");
					context.SetDynamicSRV(kSkinMatrices, sizeof(Joint) * mesh.numJoints, object.skeleton + mesh.startJoint);
				}
				const auto& pso = RenderSystem::GetPSO(key.psoIdx);
				context.SetPipelineState(pso);

				if (mCurrentType == kZPass)
				{
					bool alphaTest = (mesh.psoFlags & PSOFlags::kAlphaTest) == PSOFlags::kAlphaTest;
					uint32_t stride = alphaTest ? 16u : 12u;
					if (mesh.numJoints > 0)
						stride += 16;
					context.SetVertexBuffer(0, { object.bufferPtr + mesh.vbDepthOffset, mesh.vbDepthSize, stride });
				}
				else
				{
					context.SetVertexBuffer(0, { object.bufferPtr + mesh.vbOffset, mesh.vbSize, mesh.vbStride });
				}

				context.SetIndexBuffer({ object.bufferPtr + mesh.ibOffset, mesh.ibSize, (DXGI_FORMAT)mesh.ibFormat });

				for (uint32_t i = 0; i < mesh.numDraws; ++i)
					context.DrawIndexed(mesh.draw[i].primCount, mesh.draw[i].startIndex, mesh.draw[i].baseVertex);

				++mCurrentDraw;
			}
		}

		if (mBatchType == kShadows)
		{
			context.TransitionResource(*mDSV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}
}