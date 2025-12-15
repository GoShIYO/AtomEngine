#pragma once
#include<xaudio2.h>
#include<fstream>
#include<unordered_map>
#include<mutex>
#include "Runtime/Platform/DirectX12/D3dUtility/d3dInclude.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

namespace AtomEngine
{
	class Audio
	{
	public:

		struct ChunkHeader
		{
			char id[4];
			uint32_t size;
		};

		struct RiffHeader
		{
			ChunkHeader chunk;
			char type[4];
		};
		struct FormatChunk
		{
			ChunkHeader chunk;
			WAVEFORMATEX fmt;
		};
		struct SoundData
		{
			std::string filename;
			// 波形フォーマット
			WAVEFORMATEX wfex;
			// バッファの先頭アドレス
			std::unique_ptr<BYTE[]> pBuffer;
			// バッファのサイズ
			uint32_t bufferSize;
			IXAudio2SourceVoice* pSourceVoice = nullptr;
			bool isPlaying = false;
			bool isPaused = false;
		};

		static Audio* GetInstance();
		static uint32_t Load(const std::string& filename);
		void Play(uint32_t soundHandle, bool loop = false);

		bool IsPlaying(uint32_t soundHandle);
		bool Pause(uint32_t soundHandle);
		bool Stop(uint32_t soundHandle);
		void SetVolume(uint32_t soundHandle, float volume);
		float GetVolume(uint32_t soundHandle);

		void Initialize();

		void Finalize();

	private:
		Audio() = default;
		~Audio() = default;
		Audio(const Audio&) = delete;
		Audio& operator=(const Audio&) = delete;

	private:
		//xAudio2
		Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
		IXAudio2MasteringVoice* masterVoice = nullptr;

		static std::unordered_map<uint32_t, SoundData> soundDatas;
		static uint32_t soundHandles;
		std::mutex audioMutex;

	private:
		static SoundData SoundLoadWave(const std::string& filename);
		static SoundData SoundLoadMediaFoundation(const std::string& filename);
	};
}
