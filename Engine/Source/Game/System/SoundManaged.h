#pragma once
#include "Runtime/Function/Audio/Audio.h"
namespace Sound
{
	using namespace AtomEngine;
	
	void LoadSounds();

	extern std::unordered_map<std::string, uint32_t> gSoundMap;
}
