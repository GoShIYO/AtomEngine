#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Framework/ECS/GameObject.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Function/Input/Input.h"

#include "Runtime/Platform/DirectX12/D3dUtility/d3dInclude.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Platform/DirectX12/Pipline/RootSignature.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"

class GameApp
{
public:
	virtual ~GameApp() = default;

	virtual void Initialize() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;
	virtual void Shutdown() = 0;
	virtual bool Exit();
};
