#include "ShaderCompiler.h"

#pragma comment(lib,"dxcompiler.lib")

namespace AtomEngine
{
	struct DxcCompilerResources
	{
		~DxcCompilerResources()
		{
			Release();
		}
		IDxcUtils* utils = nullptr;
		IDxcCompiler3* compiler = nullptr;
		IDxcIncludeHandler* includeHandler = nullptr;
		void Release();
	};
	void DxcCompilerResources::Release()
	{
		utils->Release();
        compiler->Release();
        includeHandler->Release();
	}

	const wchar_t* root = L"./Asset/Shaders/";

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filePath, 
		const wchar_t* profile, 
		const wchar_t* entryPoint, 
		DxcCompilerResources& dxcResources)
	{

		ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcResources.utils)));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcResources.compiler)));
		// Shaderファイルの読み込み
		ThrowIfFailed(dxcResources.utils->CreateDefaultIncludeHandler(&dxcResources.includeHandler));

		//1.hlslファイルを読み込む
		Log(L"Begin CompilerShader, path:%ws, profile:%ws\n", filePath.c_str(), profile);
		// hlslファイルを読み込む
		IDxcBlobEncoding* shaderSource = nullptr;
		ThrowIfFailed(dxcResources.utils->LoadFile(filePath.c_str(), nullptr, &shaderSource));

		DxcBuffer shaderSourceBuffer{};
		shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
		shaderSourceBuffer.Size = shaderSource->GetBufferSize();
		shaderSourceBuffer.Encoding = DXC_CP_UTF8;

		//2.Compilerする
		LPCWSTR arguments[] = {
			filePath.c_str(),
			L"-E", entryPoint,
			L"-T", profile,
			L"-Zi", L"-Qembed_debug",
			L"-Od", L"-Zpr",
		};

		// 実際にShaderをコンパイルする
		IDxcResult* shaderResult = nullptr;
		ThrowIfFailed(dxcResources.compiler->Compile(
			&shaderSourceBuffer,				//読み込んだファイル
			arguments,							//コンパイルオプション
			_countof(arguments),				//コンパイルオプションの数
			dxcResources.includeHandler,		//includeが含まれた諸々
			IID_PPV_ARGS(&shaderResult)			//コンパイル結果
		));

		//3.警告・エラーがでていないか確認する
		IDxcBlobUtf8* shaderError = nullptr;
		ThrowIfFailed(shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr));
		if (shaderError != nullptr && shaderError->GetStringLength() != 0)
		{
			Log(shaderError->GetStringPointer());

			HRESULT hrStatus;
			ThrowIfFailed(shaderResult->GetStatus(&hrStatus));
			if (FAILED(hrStatus))
			{
				assert(false);
			}
		}

		//4.Compiler結果を受け取って返す
		IDxcBlob* shaderBlob = nullptr;
		ThrowIfFailed(shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr));

		Log(L"Compile Succeeded, path:%ws, profile%ws\n", filePath.c_str(), profile);
		// もう使わないリソースを解放
		shaderSource->Release();
		shaderResult->Release();
		if (shaderError)
		{
			shaderError->Release();
		}
		// 実行用のバイナリを返却
		return shaderBlob;
	}
	Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileBlob(const std::wstring& filePath, const wchar_t* profile, const wchar_t* entryPoint)
	{
		std::wstring fullPath = root + filePath;

		DxcCompilerResources dxcResources;
		Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = CompileShader(fullPath, profile, entryPoint, dxcResources);
		
		return shaderBlob;
	}
}

