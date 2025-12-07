#include "SM/Engine.h"
#include "SM/Math.h"
#include "SM/Memory.h"
#include "SM/Platform.h"
#include "SM/Renderer/VulkanRenderer.h"
#include <cstdio>

using namespace SM;

static const char* s_dllName = nullptr;
static bool s_bExit = false;
static Platform::Window* s_pWindow = nullptr;
static VulkanRenderer* s_renderer = nullptr;
static const char* s_rawAssetsDir = nullptr;

void SM::Init(const char* dllName, const char* rawAssetsDir)
{
    s_dllName = dllName;
    s_rawAssetsDir = rawAssetsDir;

    Platform::Init();

    SeedRng();
    InitBuiltInAllocators();

    s_pWindow = Platform::OpenWindow("Workbench", 1600, 900);
    s_renderer = new (SM::Alloc<VulkanRenderer>(kEngineGlobal)) VulkanRenderer;
    s_renderer->Init(s_pWindow);

    GameApi game = Platform::LoadGameDll(s_dllName);
    game.GameInit();
}

void SM::MainLoop()
{
    while(!s_bExit)
    {
        Platform::UpdateWindow(s_pWindow);

        GameApi game = Platform::LoadGameDll(s_dllName);
        game.GameUpdate();
        game.GameRender();
    }
}

void SM::Exit()
{
    s_bExit = true;
}

VulkanRenderer* SM::GetRenderer()
{
    return s_renderer;
}

const char* SM::GetRawAssetsDir()
{
    return s_rawAssetsDir;
}

constexpr bool SM::IsRunningDebugBuild()
{
    #if NDEBUG
    return false;
    #else
    return true;
    #endif
}

char* SM::ConcatenateStrings(const char* s1, const char* s2, LinearAllocator* allocator)
{
    size_t combinedSize = strlen(s1) + strlen(s2) + 1;
    char* combinedString = (char*)allocator->Alloc(combinedSize);
    sprintf_s(combinedString, combinedSize, "%s%s\0", s1, s2);
    return combinedString;
}
