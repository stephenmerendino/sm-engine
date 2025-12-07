#pragma once

#include "SM/StandardTypes.h"

namespace SM
{
    constexpr inline bool IsOnlyBitSet(U8 bits, U8 flag) { return (bits & ~flag) == 0; }
    constexpr inline bool IsBitSet(U8 bits, U8 flag) { return (bits & flag); }
    constexpr inline void SetBit(U8& bits, U8 flag) { bits |= flag; }
    constexpr inline void UnSetBit(U8& bits, U8 flag) { bits = bits & ~flag; }

    constexpr inline bool IsOnlyBitSet(U16 bits, U16 flag) { return (bits & ~flag) == 0; }
    constexpr inline bool IsBitSet(U16 bits, U16 flag) { return (bits & flag); }
    constexpr inline void SetBit(U16& bits, U16 flag) { bits |= flag; }
    constexpr inline void UnSetBit(U16& bits, U16 flag) { bits = bits & ~flag; }

    constexpr inline bool IsOnlyBitSet(U32 bits, U32 flag) { return (bits & ~flag) == 0; }
    constexpr inline bool IsBitSet(U32 bits, U32 flag) { return (bits & flag); }
    constexpr inline void SetBit(U32& bits, U32 flag) { bits |= flag; }
    constexpr inline void UnSetBit(U32& bits, U32 flag) { bits = bits & ~flag; }

    constexpr inline bool IsOnlyBitSet(U64 bits, U64 flag) { return (bits & ~flag) == 0; }
    constexpr inline bool IsBitSet(U64 bits, U64 flag) { return (bits & flag); }
    constexpr inline void SetBit(U64& bits, U64 flag) { bits |= flag; }
    constexpr inline void UnSetBit(U64& bits, U64 flag) { bits = bits & ~flag; }
}
