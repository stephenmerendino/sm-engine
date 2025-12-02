#pragma once

//------------------------------------------------------------------------------------------------------------------
// Engine API
//------------------------------------------------------------------------------------------------------------------
typedef void (*EngineLogFunction)(const char* format, ...);
namespace SM
{
    struct EngineApi
    {
        EngineLogFunction EngineLog;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Game API
    //------------------------------------------------------------------------------------------------------------------
    #define SM_DLL_EXPORT extern "C" __declspec(dllexport) 

    typedef void (*GameBindEngineFunction)(SM::EngineApi engineApi);
    typedef void (*GameInitFunction)();
    typedef void (*GameUpdateFunction)();
    typedef void (*GameRenderFunction)();

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
    #define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))
    #define UNUSED(x) (void*)&x
    
    #ifndef SM_DEBUG
    #define SM_DEBUG NDEBUG
    #endif

    inline constexpr bool IsRunningDebug()
    {
        #if SM_DEBUG
        return false;
        #else
        return true;
        #endif
    }

}
