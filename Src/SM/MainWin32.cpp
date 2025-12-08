// platform specific includes
#include "SM/PlatformWin32.cpp"

// common cpps to include
#include "SM/Engine.cpp"
#include "SM/Math.cpp"
#include "SM/Memory.cpp"
#include "SM/Renderer/VulkanRenderer.cpp"

int WINAPI WinMain(HINSTANCE app, 
				   HINSTANCE prevApp, 
				   LPSTR args, 
				   int show)
{
    EngineConfig config {
        .m_dllName = "Workbench.dll",
        .m_windowName = "Workbench",
        .m_windowWidth = 1920,
        .m_windowHeight = 1080,
        .m_rawAssetsDir = "..\\..\\..\\RawAssets\\"
    };

    SM::Init(config);
    SM::MainLoop();

    return 0;
}
