#include "ForwardRenderingPass.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"
#include "imgui.h"

namespace AtomEngine
{
	ForwardRenderingPass::ForwardRenderingPass()
	{
		mSkybox.Initialize();
		mSkybox.SetEnvironmentMap(L"Asset/Textures/EnvironmentMaps/PaperMill/PaperMill_E_3kEnvHDR.dds");
		mSkybox.SetBRDF_LUT(L"Asset/Textures/EnvironmentMaps/PaperMill/PaperMill_E_3kBrdf.dds");
		mSkybox.SetIBLTextures(
			L"Asset/Textures/EnvironmentMaps/PaperMill/PaperMill_E_3kDiffuseHDR.dds",
			L"Asset/Textures/EnvironmentMaps/PaperMill/PaperMill_E_3kSpecularHDR.dds");
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

		ImGui::DragFloat("SunLightIntensity", &mSunLightIntensity, 0.01f, 0.0f, 1000.0f);
		ImGui::DragFloat3("SunDirection", mSunDirection.ptr(), 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("mIBLBias", &mIBLBias, 0.1f, 0.0f, 10.0f);
		mSunDirection.Normalize();
		mShadowCamera.UpdateMatrix(-mSunDirection, Vector3(0, -500.0f, 0), Vector3(5000, 3000, 3000),
			(uint32_t)gShadowBuffer.GetWidth(), (uint32_t)gShadowBuffer.GetHeight(), 16);
		mSkybox.SetIBLBias(mIBLBias);

		GlobalConstants globals;
		globals.ViewProjMatrix = mCamera->GetViewProjMatrix();
		globals.SunShadowMatrix = mShadowCamera.GetShadowMatrix();
		globals.CameraPos = mCamera->GetPosition();
		globals.SunDirection = mSunDirection;
		globals.SunIntensity = Vector3(mSunLightIntensity);
		globals.IBLRange = mSkybox.GetIBLRange();
		globals.IBLBias = mSkybox.GetIBLBias();
		
		gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
		gfxContext.ClearDepth(gSceneDepthBuffer);

		RenderQueue queue(RenderQueue::kDefault);
		queue.SetCamera(*mCamera);
		queue.SetViewportAndScissor(viewport, scissor);
		queue.SetDepthStencilTarget(gSceneDepthBuffer);
		queue.AddRenderTarget(gSceneColorBuffer);

		auto view = mWorld->GetRegistry().view<MeshComponent>();

		for (auto entity : view)
		{
			auto& mesh = view.get<MeshComponent>(entity);
			mesh.Render(queue);
		}

		queue.Sort();

		queue.RenderMeshes(RenderQueue::kZPass, gfxContext, globals);

		{
			RenderQueue shadowSorter(RenderQueue::kShadows);
			shadowSorter.SetCamera(mShadowCamera);
			shadowSorter.SetDepthStencilTarget(gShadowBuffer);

			for (auto entity : view)
			{
				auto& mesh = view.get<MeshComponent>(entity);
				mesh.Render(shadowSorter);
			}

			shadowSorter.Sort();
			shadowSorter.RenderMeshes(RenderQueue::kZPass, gfxContext, globals);
		}

		gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		gfxContext.ClearColor(gSceneColorBuffer);
		{
			gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
			gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV_DepthReadOnly());
			gfxContext.SetViewportAndScissor(viewport, scissor);

			mSkybox.Render(gfxContext, mCamera, viewport, scissor);

			queue.RenderMeshes(RenderQueue::kOpaque, gfxContext, globals);
		}

		queue.RenderMeshes(RenderQueue::kTransparent, gfxContext, globals);
	}
}