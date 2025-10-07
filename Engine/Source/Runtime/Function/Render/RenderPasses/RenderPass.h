#pragma once
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "../RenderQueue.h"

namespace AtomEngine
{
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void Setup(GraphicsContext& context) {}
        virtual void Execute(GraphicsContext& context, RenderQueue& queue, GlobalConstants& globals) = 0;
        virtual void Cleanup(GraphicsContext& context) {}

        void SetRenderTarget(ColorBuffer* rtv, DepthBuffer* dsv) { mRTV = rtv; mDSV = dsv; }
        void SetDepthStencilTarget(DepthBuffer* dsv) { mDSV = dsv; }
        void SetCamera(CameraBase* camera) { mCamera = camera; }

    protected:
        const CameraBase* mCamera = nullptr;
        ColorBuffer* mRTV = nullptr;
        DepthBuffer* mDSV = nullptr;
    };
}