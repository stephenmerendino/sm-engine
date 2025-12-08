#include "SM/Engine.h"
#include "SM/Platform.h"

#include "SM/Util.cpp"
#include "SM/Math.cpp"
#include "SM/Memory.cpp"
#include "SM/Renderer/VulkanRenderer.cpp"

#include <cstdio>

using namespace SM;

static EngineConfig s_engineConfig;
static bool s_bExit = false;

void SM::Init(const EngineConfig& config)
{
    SM::Platform::Init();
    s_engineConfig = config;
    SeedRng();
    InitBuiltInAllocators();
}

void SM::Exit()
{
    s_bExit = true;
}

bool SM::ExitRequested()
{
    return s_bExit;
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
