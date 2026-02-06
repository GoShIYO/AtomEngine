#pragma once
#include <cstdint>

namespace AtomEngine
{
	class CommandContext;
	class ComputeContext;
	class ColorBuffer;
	class Camera;
	class TemporalAA
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Update(uint64_t frameIndex);
		
		static void GetJitterOffset(float& JitterX, float& JitterY);
		static void ClearHistory(CommandContext& Context);
		static void ResolveImage(CommandContext& Context);
		static void ImGuiSettingsWindow();
	private:
		static void ApplyTemporalAA(ComputeContext& Context);
		static void SharpenImage(ComputeContext& Context, ColorBuffer& TemporalColor);
	};
}


