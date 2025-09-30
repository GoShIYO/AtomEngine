#pragma once
#include "Runtime/Platform/DirectX12/Core/DescriptorHeap.h"
#include "Runtime/Resource/TextureManager.h"

namespace AtomEngine
{
	class RenderCore
	{
	public:

		static void Initialize();

		static void Render(float deltaTime);

		static void Shutdown();

	public:
		static DescriptorHeap gTextureHeap;
		static DescriptorHeap gSamplerHeap;
	};
}


