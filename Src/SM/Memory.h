#pragma once

#include "SM/StandardTypes.h"

namespace SM
{
    enum MemoryArenaType
    {
        kEngineGlobal,
        kNumArenaTypes
    };

    enum
    {
        kAlign1 = 1,
        kAlign2 = 2,
        kAlign4 = 4,
        kAlign8 = 8,
        kAlign16 = 16,
        kAlign32 = 32,
        kAlign64 = 64,
        kAlign128 = 128,
        kAlign256 = 256,
        kAlign512 = 512,
        kAlign1Kib = KiB(1),
        kAlign1MiB = MiB(1),
        kAlign2MiB = MiB(2)
    };

    void InitAllocators();

    void* Alloc(MemoryArenaType arenaType, size_t numBytes, U32 alignment = kAlign1);

    template<typename T>
    T* Alloc(MemoryArenaType arenaType)
    {
        return (T*)Alloc(arenaType, sizeof(T), alignof(T));
    }

    template<typename T>
    T* Alloc(MemoryArenaType arenaType, size_t numElements)
    {
        return (T*)Alloc(arenaType, sizeof(T) * numElements, alignof(T));
    }
}
