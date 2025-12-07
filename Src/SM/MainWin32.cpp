// platform specific includes
#include "SM/PlatformWin32.cpp"

// common cpps to include
#include "SM/Engine.cpp"
#include "SM/Math.cpp"
#include "SM/Memory.cpp"
#include "SM/Renderer/Renderer.cpp"
#include "SM/Renderer/VulkanRenderer.cpp"

int WINAPI WinMain(HINSTANCE app, 
				   HINSTANCE prevApp, 
				   LPSTR args, 
				   int show)
{
    const char* dllName = "Workbench.dll";
    const char* rawAssetsDir = "..\\..\\..\\RawAssets\\";

    SM::EngineInit(dllName);
    SM::EngineMainLoop();

    return 0;
}
