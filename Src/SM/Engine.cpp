#include "SM/Engine.h"
#include "SM/Platform.h"

#include "SM/Math.cpp"
#include "SM/Memory.cpp"
#include "SM/Renderer/VulkanRenderer.cpp"

#include <cstdio>

using namespace SM;

static EngineConfig s_engineConfig;
static bool s_bExit = false;
static Platform::Window* s_pWindow = nullptr;
static VulkanRenderer* s_renderer = nullptr;

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
