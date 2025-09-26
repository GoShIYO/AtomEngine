#pragma once
#include "../Utility/Utility.h"

namespace AtomEngine
{

	void InitLog();
	void FinalizeLog();

	void Log(const char* format, ...);
	void Log(const wchar_t* format, ...);
}
