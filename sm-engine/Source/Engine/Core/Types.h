#pragma once

#include <cstdint>

typedef int8_t		I8;
typedef int16_t 	I16;
typedef int32_t 	I32;
typedef int64_t 	I64;

typedef uint8_t		U8;
typedef uint16_t	U16;
typedef uint32_t	U32;
typedef uint64_t	U64;

typedef float		F32;
typedef double		F64;

typedef unsigned char Byte;

#define KiB(x) (x * 1024)
#define MiB(x) (KiB(x) * 1024)
#define GiB(x) (MiB(x) * 1024)
#define TiB(x) (GiB(x) * 1024)
