// platform specific includes
#include "SM/PlatformWin32.cpp"

// common cpps to include
#include "SM/Engine.cpp"
#include "SM/Memory.cpp"
#include "SM/Renderer/Renderer.cpp"
#include "SM/Renderer/VulkanRenderer.cpp"

int WINAPI WinMain(HINSTANCE app, 
				   HINSTANCE prevApp, 
				   LPSTR args, 
				   int show)
{
    const char* dllName = "Workbench.dll";

    SM::EngineInit(dllName);
    SM::EngineMainLoop();

    return 0;
}
