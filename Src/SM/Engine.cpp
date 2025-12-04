#include "SM/Engine.h"
#include "SM/Math.h"
#include "SM/Memory.h"
#include "SM/Platform.h"
#include "SM/Renderer/Renderer.h"

using namespace SM;

static const char* s_dllName = nullptr;
static bool s_bExit = false;
static Platform::Window* s_pWindow = nullptr;
static Renderer* s_renderer = nullptr;

void SM::EngineInit(const char* dllName)
{
    s_dllName = dllName;

    Platform::Init();

    SeedRng();
    InitAllocators();

    s_pWindow = Platform::OpenWindow("Workbench", 1600, 900);
    s_renderer = CreateRenderer(kVulkan);
    s_renderer->Init(s_pWindow);

    GameApi game = Platform::LoadGameDll(s_dllName);
    game.GameInit();
}

void SM::EngineMainLoop()
{
    while(!s_bExit)
    {
        Platform::UpdateWindow(s_pWindow);

        GameApi game = Platform::LoadGameDll(s_dllName);
        game.GameUpdate();
        game.GameRender();
    }
}

void SM::EngineExit()
{
    s_bExit = true;
}
