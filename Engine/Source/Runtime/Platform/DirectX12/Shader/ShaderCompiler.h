#pragma once
#include "../D3dUtility/d3dInclude.h"

namespace AtomEngine
{
	class ShaderCompiler
	{
	public:
		static Microsoft::WRL::ComPtr<IDxcBlob> CompileBlob(
			const std::wstring& filePath,
			const wchar_t* profile,
			const wchar_t* entryPoint = L"main"
		);

	};
}

