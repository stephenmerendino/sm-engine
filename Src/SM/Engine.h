#pragma once

#include <cstdint>

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
}

//------------------------------------------------------------------------------------------------------------------
// Game API
//------------------------------------------------------------------------------------------------------------------
#define SM_DLL_EXPORT extern "C" __declspec(dllexport) 

typedef void (*GameLoadedFunction)(SM::EngineApi engineApi);
typedef void (*GameUpdateFunction)(void);
typedef void (*GameRenderFunction)(void);

#define GAME_LOADED_FUNCTION_NAME GameLoaded
#define GAME_UPDATE_FUNCTION_NAME GameUpdate
#define GAME_RENDER_FUNCTION_NAME GameRender

#define GAME_LOADED_FUNCTION_NAME_STRING "GameLoaded" 
#define GAME_UPDATE_FUNCTION_NAME_STRING "GameUpdate"
#define GAME_RENDER_FUNCTION_NAME_STRING "GameRender"

#define GAME_LOADED SM_DLL_EXPORT void GAME_LOADED_FUNCTION_NAME(SM::EngineApi engineApi)
#define GAME_UPDATE SM_DLL_EXPORT void GAME_UPDATE_FUNCTION_NAME()
#define GAME_RENDER SM_DLL_EXPORT void GAME_RENDER_FUNCTION_NAME()

namespace SM
{
    typedef		uint8_t		U8;
    typedef		uint16_t	U16;
    typedef		uint32_t	U32;
    typedef		uint64_t	U64;

    typedef		int8_t		I8;
    typedef		int16_t		I16;
    typedef		int32_t		I32;
    typedef		int64_t		I64;

    typedef		float		F32;
    typedef		double		F64;

    typedef		U8			Byte8;

    #define KiB(i) (i * 1024)
    #define MiB(i) (i * KiB(1024))
    #define GiB(i) (i * MiB(1024))
    #define TiB(i) (i * GiB(1024))
}
