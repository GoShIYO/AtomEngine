#pragma once
#include "Runtime/Core/Color/Color.h"
#include "../D3dUtility/d3dInclude.h"

namespace AtomEngine
{

	class SamplerDesc : public D3D12_SAMPLER_DESC
	{
	public:
		// これらのデフォルトは、HLSL定義のルート
		// シグネチャ静的サンプラーのデフォルト値と一致します。したがって、ここでオーバーライドしないことで、
		// HLSLで定義しなくても安全になります。
		SamplerDesc()
		{
			Filter = D3D12_FILTER_ANISOTROPIC;
			AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			MipLODBias = 0.0f;
			MaxAnisotropy = 16;
			BorderColor[0] = 1.0f;
			BorderColor[1] = 1.0f;
			BorderColor[2] = 1.0f;
			BorderColor[3] = 1.0f;
			MinLOD = 0.0f;
			MaxLOD = D3D12_FLOAT32_MAX;
		}

		void SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE AddressMode)
		{
			AddressU = AddressMode;
			AddressV = AddressMode;
			AddressW = AddressMode;
		}

		void SetBorderColor(Color Border)
		{
			BorderColor[0] = Border.r;
			BorderColor[1] = Border.g;
			BorderColor[2] = Border.b;
			BorderColor[3] = Border.a;
		}

		// 必要に応じて新しい記述子を割り当て、可能な場合は既存の記述子へのハンドルを返します。
		D3D12_CPU_DESCRIPTOR_HANDLE CreateDescriptor(void);

		// 記述子をその場で作成する（重複排除なし）。ハンドルは事前に割り当てる必要がある。
		void CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE Handle);
	};

}
