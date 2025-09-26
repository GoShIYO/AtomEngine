#pragma once
#include "../D3dUtility/d3dInclude.h"

namespace AtomEngine
{
	enum eResolution { k720p, k900p, k1080p, k1440p, k1800p, k2160p };

	class SwapChain
	{
	public:
		static void Initialize();

		static void Finalize();

		static void OnResize(uint32_t width, uint32_t height);

		static void Present();

		static uint32_t GetBackBufferWidth();

        static uint32_t GetBackBufferHeight();

		static DXGI_FORMAT GetBackBufferFormat();

	};
}

