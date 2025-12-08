#include "SM/Engine.h"
#include "SM/Math.h"
#include "SM/Memory.h"
#include "SM/Platform.h"
#include "SM/Renderer/VulkanRenderer.h"
#include <cstdio>

using namespace SM;

static EngineConfig s_engineConfig;
static bool s_bExit = false;
static Platform::Window* s_pWindow = nullptr;
static VulkanRenderer* s_renderer = nullptr;

void SM::Init(const EngineConfig& config)
{
    s_engineConfig = config;
    Platform::Init();

    SeedRng();
    InitBuiltInAllocators();

    s_pWindow = Platform::OpenWindow(config.m_windowName, config.m_windowWidth, config.m_windowHeight);

    s_renderer = new (SM::Alloc<VulkanRenderer>(kEngineGlobal)) VulkanRenderer;
    s_renderer->Init(s_pWindow);

    GameApi game = Platform::LoadGameDll(s_engineConfig.m_dllName);
    game.GameInit();
}

void SM::MainLoop()
{
    while(!s_bExit)
    {
        Platform::UpdateWindow(s_pWindow);

        GameApi game = Platform::LoadGameDll(s_engineConfig.m_dllName);
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
    return s_engineConfig.m_rawAssetsDir;
}

bool SM::IsRunningDebugBuild()
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
