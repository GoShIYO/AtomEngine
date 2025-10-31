#include "RenderQueue.h"
#include "RenderSystem.h"

#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Resource/Model.h"

#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"

namespace AtomEngine
{
    void RenderQueue::AddMesh(
        const Mesh& mesh, 
        const JointXform* skeleton,
        D3D12_VERTEX_BUFFER_VIEW vbv,
        D3D12_INDEX_BUFFER_VIEW ibv,
        D3D12_GPU_VIRTUAL_ADDRESS meshCBV, 
        D3D12_GPU_VIRTUAL_ADDRESS materialCBV, 
        float distance)
    {
      

        RenderObject object{ &mesh,skeleton, vbv, ibv, meshCBV, materialCBV};
        mRenderObjects.push_back(object);
    }

    void RenderQueue::Sort()
    {

    }

    void RenderQueue::RenderMeshes(DrawType type, GraphicsContext& context, GlobalConstants& globals)
    {

    }
}