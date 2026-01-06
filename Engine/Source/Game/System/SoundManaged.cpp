#include "SoundManaged.h"

namespace Sound
{
	std::unordered_map<std::string, uint32_t> gSoundMap;
	const std::string kBaseSoundPath = "Asset/Sounds/";
	void LoadSounds()
	{
		auto* audio = Audio::GetInstance();
		gSoundMap["bgm"] = audio->Load(kBaseSoundPath + "bgm.mp3");
		gSoundMap["button"] = audio->Load(kBaseSoundPath + "button.mp3");
		gSoundMap["goal"] = audio->Load(kBaseSoundPath + "goal.mp3");
		gSoundMap["pickup"] = audio->Load(kBaseSoundPath + "pickup.wav");
		gSoundMap["moveSe"] = audio->Load(kBaseSoundPath + "moveSe.mp3");
	}
}
