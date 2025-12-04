#pragma once

#include <cstdint>

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

    typedef		U8			Byte;

    #define KiB(i) (i * 1024)
    #define MiB(i) (i * KiB(1024))
    #define GiB(i) (i * MiB(1024))
    #define TiB(i) (i * GiB(1024))
}
