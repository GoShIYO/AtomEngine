#include "RenderCore.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

namespace AtomEngine
{
	DescriptorHeap RenderCore::gTextureHeap;
	DescriptorHeap RenderCore::gSamplerHeap;

	void RenderCore::Initialize()
	{
		gTextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);

		gSamplerHeap.Create(L"Scene Sampler Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);
	}

	void RenderCore::Render(float deltaTime)
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		DX12Core::Present();
	}

	void RenderCore::Shutdown()
	{
		gTextureHeap.Destroy();
		gSamplerHeap.Destroy();
	}
}