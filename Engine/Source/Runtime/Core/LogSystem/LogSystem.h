#pragma once
#include "../Utility/Utility.h"

namespace AtomEngine
{

	void StartLog();
	void CloseLog();

	void Log(const char* format, ...);
	void Log(const wchar_t* format, ...);
}
