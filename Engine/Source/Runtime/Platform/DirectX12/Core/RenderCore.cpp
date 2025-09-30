#include "RenderCore.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Core/Utility/Utility.h"

#include "imgui.h"

namespace AtomEngine
{
	DescriptorHeap RenderCore::gTextureHeap;
	DescriptorHeap RenderCore::gSamplerHeap;

	TextureRef test;
	DescriptorHandle gpuHandle;

	void RenderCore::Initialize()
	{
		gTextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);

		gSamplerHeap.Create(L"Scene Sampler Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);

		TextureManager::Initialize(L"");

		test = TextureManager::LoadDDSFromFile(L"Asset/textures/uvChecker.dds");
		gpuHandle = gTextureHeap.Alloc();

		DX12Core::gDevice->CopyDescriptorsSimple(
			1,
			gpuHandle,
			test.GetSRV(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
	}

	void RenderCore::Render(float deltaTime)
	{
		
		ImGui::ShowDemoWindow();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = test.GetSRV();

		ImGui::Image(gpuHandle.GetGpuPtr(), ImVec2(512, 512));
	}

	void RenderCore::Shutdown()
	{
		gTextureHeap.Destroy();
		gSamplerHeap.Destroy();

		TextureManager::Finalize();
	}
}