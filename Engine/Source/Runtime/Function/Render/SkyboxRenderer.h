#pragma once
#pragma once
#include "Runtime/Resource/TextureRef.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"

namespace AtomEngine
{
	class SkyboxRenderer
	{
	public:
		void Initialize();
		void Shutdown();

		void SetEnvironmentMap(TextureRef environmentMap);
		void SetIBLTextures(TextureRef specularIBL, TextureRef diffuseIBL);
		
		void SetEnvironmentMap(const std::wstring& file);
		void SetIBLTextures(const std::wstring& specularFile,const std::wstring& diffuseFile);

		void SetIBLBias(float LODBias);
		void Render(GraphicsContext& context, const Camera& camera, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissor);
		float GetIBLRange()const;
		float GetIBLBias()const;

	private:
		TextureRef mEnvironmentMap;
		TextureRef mRadianceCubeMap;
		TextureRef mIrradianceCubeMap;
		float mSpecularIBLRange;
		float mSpecularIBLBias;

		RootSignature mSkyboxRootSig;
		GraphicsPSO mSkyboxPSO;
	};
}
