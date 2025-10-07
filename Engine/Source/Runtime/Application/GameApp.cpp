#include "GameApp.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Function/Input/Input.h"

using namespace AtomEngine;

bool GameApp::Exit()
{
	return gContext.input->IsTriggerKey(VK_ESCAPE);
}
