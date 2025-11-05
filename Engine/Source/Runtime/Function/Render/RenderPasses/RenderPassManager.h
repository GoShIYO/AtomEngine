#pragma once
#include "RenderPass.h"

namespace AtomEngine
{
    class RenderPassManager
    {
    public:
        static RenderPassManager* GetInstance();

        void RegisterRenderPass(std::unique_ptr<RenderPass> pass);
        void SetUpRenderPass();

        void RenderAll(GraphicsContext& context);

        void SetRenderTarget(ColorBuffer* rtv, DepthBuffer* dsv);
        void SetDepthStencilTarget(DepthBuffer* dsv);
        void SetCamera(Camera* camera);
        void SetViewportAndScissor(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissor);
        void SetWorld(World& world);

        void Shutdown();
    private:
        std::vector<std::unique_ptr<RenderPass>> mAllPasses;
        std::vector<RenderPass*> mCurrentPasses;

    private:
        RenderPassManager() = default;
        ~RenderPassManager() = default;
        RenderPassManager(const RenderPassManager&) = delete;
        RenderPassManager& operator=(const RenderPassManager&) = delete;
    };

}


