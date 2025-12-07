#pragma once

#include "SM/Memory.h"
#include "SM/Renderer/VulkanRenderer.h"

namespace SM
{
    //------------------------------------------------------------------------------------------------------------------
    // Engine API
    //------------------------------------------------------------------------------------------------------------------
    typedef void (*EngineLogFunction)(const char* format, ...);
    typedef VulkanRenderer* (*EngineGetRendererFunction)();

    struct EngineApi
    {
        EngineLogFunction Log;
        EngineGetRendererFunction GetRenderer;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Game API
    //------------------------------------------------------------------------------------------------------------------
    #define SM_DLL_EXPORT extern "C" __declspec(dllexport) 

    typedef void (*GameBindEngineFunction)(SM::EngineApi engineApi);
    typedef void (*GameInitFunction)();
    typedef void (*GameUpdateFunction)();
    typedef void (*GameRenderFunction)();

    struct GameApi
    {
        GameBindEngineFunction GameBindEngine = nullptr;
        GameInitFunction GameInit = nullptr;
        GameUpdateFunction GameUpdate = nullptr;
        GameRenderFunction GameRender = nullptr;
    };

    #define GAME_BIND_ENGINE_FUNCTION_NAME GameBindEngine
    #define GAME_INIT_FUNCTION_NAME GameInit
    #define GAME_UPDATE_FUNCTION_NAME GameUpdate
    #define GAME_RENDER_FUNCTION_NAME GameRender
    
    #define GAME_BIND_ENGINE_FUNCTION_NAME_STRING "GameBindEngine" 
    #define GAME_INIT_FUNCTION_NAME_STRING "GameInit" 
    #define GAME_UPDATE_FUNCTION_NAME_STRING "GameUpdate"
    #define GAME_RENDER_FUNCTION_NAME_STRING "GameRender"
    
    #define GAME_BIND_ENGINE SM_DLL_EXPORT void GAME_BIND_ENGINE_FUNCTION_NAME(SM::EngineApi engineApi)
    #define GAME_INIT   SM_DLL_EXPORT void GAME_INIT_FUNCTION_NAME()
    #define GAME_UPDATE SM_DLL_EXPORT void GAME_UPDATE_FUNCTION_NAME()
    #define GAME_RENDER SM_DLL_EXPORT void GAME_RENDER_FUNCTION_NAME()

    //------------------------------------------------------------------------------------------------------------------
    // Common
    //------------------------------------------------------------------------------------------------------------------
    void Init(const char* dllName, const char* rawAssetsDir);
    void MainLoop();
    void Exit();
    VulkanRenderer* GetRenderer();
    const char* GetRawAssetsDir();
    constexpr bool IsRunningDebugBuild();

    #define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))
    #define UNUSED(x) (void*)&x

    template <typename T>
    inline void Swap(T& a, T& b)
    {
        T copy = a;
        a = b;
        b = copy;
    }

    char* ConcatenateStrings(const char* s1, const char* s2, LinearAllocator* allocator = GetCurrentAllocator());
}
