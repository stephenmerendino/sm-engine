#include "SM/Memory.h"
#include "SM/Assert.h"

#include <cstdlib>

using namespace SM;

struct MemoryArena
{
    MemoryArenaType m_arenaType;    
    size_t m_sizeBytes;
};

MemoryArena g_memoryArenas[kNumArenaTypes] = 
{
    { .m_arenaType = kEngineGlobal, .m_sizeBytes = MiB(1) }   
};

class LinearAllocator
{
    public:
    void Init(void* storage, size_t size);
    void* Alloc(size_t sizeBytes, U32 alignment);
    template<typename T>
    T* Alloc(size_t numElements);
    template<typename T>
    T* Alloc();
    void Reset();

    void* m_pMemory = nullptr;
    size_t m_size = 0;
    U32 m_allocatedBytes = 0;
};

void LinearAllocator::Init(void* storage, size_t size)
{
    m_pMemory = storage;
    m_size = size;
    m_allocatedBytes = 0;
}

template<typename T>
T* LinearAllocator::Alloc(size_t numElements)
{
    return (T*)Alloc(sizeof(T) * numElements, alignof(T));
}

template<typename T>
T* LinearAllocator::Alloc()
{
    return Alloc<T>(1);
}

static uintptr_t AlignAddress(uintptr_t address, uintptr_t alignment)
{
    /*
     * 4 bit example for rounding up to nearest alignment
     * currentAllocAddr = 0b0110 = 6
     * alignment = "4 bit" alignment = 0b0100
     *
     *   0b0110 (currentAllocAddr)
     * + 0b0011 (alignment - 1)
     *   ------
     *   0b1001 (result of addition = 9)
     * & 0b1100 zero out the lower bits with ~(alignment - 1)
     *   ------
     *   0b1000 = 8 (rounded up from 6 to align to 4 bits)
     */
    uintptr_t nextAlignedAddr = (address + (alignment - 1)) & ~(alignment - 1);
    return  nextAlignedAddr;
}

void* LinearAllocator::Alloc(size_t sizeBytes, U32 alignment)
{
    uintptr_t currentAddr = (uintptr_t)m_pMemory + m_allocatedBytes;
    uintptr_t nextAlignedAddr = AlignAddress(currentAddr, alignment);

    uintptr_t alignmentDelta = nextAlignedAddr - currentAddr;
    size_t totalBytesNeeded = alignmentDelta + sizeBytes;

    SM_ASSERT(m_allocatedBytes + totalBytesNeeded <= m_size);

    m_allocatedBytes += totalBytesNeeded; 
    return (void*)nextAlignedAddr;
}

void LinearAllocator::Reset()
{
    m_allocatedBytes = 0;
}

LinearAllocator g_allocators[kNumArenaTypes];

void SM::InitAllocators()
{
    for(int i = 0; i < kNumArenaTypes; i++)
    {
        void* allocatorMemory = malloc(g_memoryArenas[i].m_sizeBytes);
        g_allocators[i].Init(allocatorMemory, g_memoryArenas[i].m_sizeBytes);
    }
}

void* SM::Alloc(MemoryArenaType arenaType, size_t numBytes, U32 alignment)
{
    return g_allocators[arenaType].Alloc(numBytes, alignment);
}
