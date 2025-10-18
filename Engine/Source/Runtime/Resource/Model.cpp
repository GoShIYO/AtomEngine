#include "Model.h"
#include "AssetManager.h"

#include "Runtime/Function/Render/RenderQueue.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"

#include "../Core/Math/Frustum.h"

namespace AtomEngine
{
    void Model::Destroy()
    {
        vertexBuffer.Destroy();
        indexBuffer.Destroy();
        textures.clear();
    }
}