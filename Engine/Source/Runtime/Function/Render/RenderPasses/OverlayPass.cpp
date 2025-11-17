#include "OverlayPass.h"
#include "../SpriteRenderer.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"

namespace AtomEngine
{

	void OverlayPass::Render(GraphicsContext& gfxContext)
	{
		GraphicsContext& UiContext = GraphicsContext::Begin(L"Render UI");

		UiContext.TransitionResource(gOverlayBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		UiContext.ClearColor(gOverlayBuffer);
		UiContext.SetRenderTarget(gOverlayBuffer.GetRTV());
		UiContext.SetViewportAndScissor(0, 0, gOverlayBuffer.GetWidth(), gOverlayBuffer.GetHeight());

		SpriteRenderer::Render(UiContext);

		UiContext.SetRenderTarget(gOverlayBuffer.GetRTV());
		UiContext.SetViewportAndScissor(0, 0, gOverlayBuffer.GetWidth(), gOverlayBuffer.GetHeight());
		UiContext.Finish();

	}
}