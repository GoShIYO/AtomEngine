#include "ForwardRenderingPass.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"
#include "Runtime/Function/Light/LightManager.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Game/Tag.h"
#include "imgui.h"

namespace AtomEngine
{
	ForwardRenderingPass::ForwardRenderingPass()
	{
		mSkybox.Initialize();
		//mSkybox.SetEnvironmentMap(L"Asset/Textures/EnvironmentMaps/SkyEnv/SkyEnvHDR.dds");
		mSkybox.SetBRDF_LUT(L"Asset/Textures/EnvironmentMaps/SkyEnv/SkyEnvBrdf.dds");
		mSkybox.SetIBLTextures(
			L"Asset/Textures/EnvironmentMaps/SkyEnv/SkyEnvDiffuseHDR.dds",
			L"Asset/Textures/EnvironmentMaps/SkyEnv/SkyEnvSpecularHDR.dds");
		mGpuHandle = Renderer::GetTextureHeap().Alloc();
		DX12Core::gDevice->CopyDescriptorsSimple(1, mGpuHandle, gShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	ForwardRenderingPass::~ForwardRenderingPass()
	{
		mSkybox.Shutdown();
	}

	void ForwardRenderingPass::Render(GraphicsContext& gfxContext)
	{
		ASSERT(mWorld != nullptr);

		const D3D12_VIEWPORT& viewport = mViewport;
		const D3D12_RECT& scissor = mScissor;
		float costheta = Math::cos(mSunOrientation);
		float sintheta = Math::sin(mSunOrientation);
		float cosphi = Math::cos(mSunInclination * Math::HalfPI);
		float sinphi = Math::sin(mSunInclination * Math::HalfPI);
		//mSunDirection = Math::Normalize(Vector3(costheta * cosphi, sinphi, sintheta * cosphi));
		ImGui::Begin("Forward Rendering");
		ImGui::DragFloat("SunLightIntensity", &mSunLightIntensity, 0.01f, 0.0f, 1000.0f);
		ImGui::DragFloat("SunOrientation", &mSunOrientation, 0.01f);
		ImGui::DragFloat("SunInclination", &mSunInclination, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("SunDirection", mSunDirection.ptr(), 0.01f);
		ImGui::DragFloat("IBLFactor", &mIBLFactor, 0.01f, 0.0f, 2.0f);
		ImGui::DragFloat("IBLBias", &mIBLBias, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat3("SunCenter", mShadowCenter.ptr(), 0.1f);
		ImGui::DragFloat3("ShadowDim", mShadowDim.ptr(), 0.1f, 0.1f);
		ImGui::Checkbox("EnableIBL", &mEnableIBL);
		ImGui::End();

		ImGui::Begin("Shadow Srv");
		ImGui::Image((ImTextureID)mGpuHandle.GetGpuPtr(), ImVec2(512, 512));
		ImGui::End();

		mSunDirection.Normalize();

		mShadowCamera.UpdateMatrix(mSunDirection, mCamera->GetPosition(), mShadowDim,
			(uint32_t)gShadowBuffer.GetWidth(), (uint32_t)gShadowBuffer.GetHeight());

		mSkybox.SetIBLBias(mIBLBias);

		GlobalConstants globals;
		globals.ViewProjMatrix = mCamera->GetViewProjMatrix();
		globals.SunShadowMatrix = mShadowCamera.GetShadowMatrix();
		globals.CameraPos = mCamera->GetPosition();
		globals.SunDirection = mSunDirection;
		globals.SunIntensity = Vector3(mSunLightIntensity);
		globals.IBLFactor = mIBLFactor;
		globals.IBLRange = mSkybox.GetIBLRange();
		globals.IBLBias = mSkybox.GetIBLBias();
		globals.ShadowTexelSize[0] = 1.0f / gShadowBuffer.GetWidth();
		globals.InvTileDim[0] = 1.0f / LightManager::LightGridDim;
		globals.InvTileDim[1] = 1.0f / LightManager::LightGridDim;
		globals.TileCount[0] = DivideByMultiple(gSceneColorBuffer.GetWidth(), LightManager::LightGridDim);
		globals.TileCount[1] = DivideByMultiple(gSceneColorBuffer.GetHeight(), LightManager::LightGridDim);

		gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
		gfxContext.ClearDepth(gSceneDepthBuffer);

		RenderQueue queue(RenderQueue::kDefault);
		queue.SetCamera(*mCamera);
		queue.SetViewportAndScissor(viewport, scissor);
		queue.SetDepthStencilTarget(gSceneDepthBuffer);
		queue.AddRenderTarget(gSceneColorBuffer);

		RenderObjects(queue);

		queue.Sort();

		queue.RenderMeshes(RenderQueue::kZPass, gfxContext, globals);

		{
			RenderQueue shadowSorter(RenderQueue::kShadows);
			shadowSorter.SetCamera(mShadowCamera);
			shadowSorter.SetDepthStencilTarget(gShadowBuffer);

			RenderObjects(shadowSorter);

			shadowSorter.Sort();
			shadowSorter.RenderMeshes(RenderQueue::kZPass, gfxContext, globals);
		}

		LightManager::FillLightGrid(gfxContext, *mCamera);

		gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		gfxContext.ClearColor(gSceneColorBuffer);
		{
			gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
			gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV_DepthReadOnly());
			gfxContext.SetViewportAndScissor(viewport, scissor);

			if (mEnableIBL)
				mSkybox.Render(gfxContext, mCamera, viewport, scissor);

			queue.RenderMeshes(RenderQueue::kOpaque, gfxContext, globals);
		}

		queue.RenderMeshes(RenderQueue::kTransparent, gfxContext, globals);
	}

	void ForwardRenderingPass::RenderObjects(RenderQueue& queue)
	{
		auto view = mWorld->GetRegistry().view<MeshComponent>();

		for (auto entity : view)
		{
			auto& mesh = view.get<MeshComponent>(entity);
			if (!mesh.IsRender())continue;
			mesh.Render(queue);
		}
	}
}