#pragma once

#include <cstdint>

namespace sm
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

    typedef		U8			Byte;

    #define KiB(i) i * 1024
    #define MiB(i) i * KiB(1024) 
    #define GiB(i) i * MiB(1024) 
}

#define SM_DLL_EXPORT extern "C" __declspec(dllexport) 

// Engine API
typedef void (*EngineLogFunction)(const char* format, ...);
namespace sm
{
    struct EngineApi
    {
        EngineLogFunction EngineLog;
    };
}

// Game API
typedef void (*GameLoadedFunction)(sm::EngineApi engineApi);
typedef void (*GameUpdateFunction)(void);
typedef void (*GameRenderFunction)(void);
