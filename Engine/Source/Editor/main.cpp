#include "Runtime/Function/Render/WindowManager.h"
#include "Runtime/EngineCore.h"

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{

	AtomEngine::AtomEngine engine;
	engine.Initialize();
	engine.Update();
	
	return engine.Shutdown();
}