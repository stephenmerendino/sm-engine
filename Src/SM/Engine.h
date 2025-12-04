#pragma once

namespace SM
{
    //------------------------------------------------------------------------------------------------------------------
    // Engine API
    //------------------------------------------------------------------------------------------------------------------
    typedef void (*EngineLogFunction)(const char* format, ...);
    struct EngineApi
    {
        EngineLogFunction Log;
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
    void EngineInit(const char* dllName);
    void EngineMainLoop();
    void EngineExit();

    inline constexpr bool IsRunningDebugBuild()
    {
        #if NDEBUG
        return false;
        #else
        return true;
        #endif
    }

    #define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))
    #define UNUSED(x) (void*)&x

    template<typename T>
    inline T Min(T a, T b)
    {
        return (a < b) ? a : b;
    }

    template<typename T>
    inline T Max(T a, T b)
    {
        return (a > b) ? a : b;
    }

    template<typename T>
    inline T Clamp(T value, T min, T max)
    {
        if(value < min) return min;
        if(value > max) return max;
        return value;
    }
}
