#include "SM/Memory.h"
#include "SM/Assert.h"
#include "SM/Containers.h"

#include <cstdlib>

using namespace SM;

static size_t s_memoryArenaSizes[kNumBuiltInArenas] = 
{
    MiB(1) // kEngineGlobal   
};

LinearAllocator s_allocators[kNumBuiltInArenas];
Stack<LinearAllocator*, 256> s_allocatorStack;

void LinearAllocator::Init(void* storage, size_t size)
{
    m_pMemory = storage;
    m_size = size;
    m_allocatedBytes = 0;
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

void SM::InitBuiltInAllocators()
{
    for(int i = 0; i < kNumBuiltInArenas; i++)
    {
        void* allocatorMemory = malloc(s_memoryArenaSizes[i]);
        s_allocators[i].Init(allocatorMemory, s_memoryArenaSizes[i]);
    }
}

LinearAllocator* SM::GetBuiltInAllocator(BuiltInMemoryAllocator builtInArena)
{
    return &s_allocators[builtInArena];
}

void SM::PushAllocator(LinearAllocator* allocator)
{
    s_allocatorStack.Push(allocator);
}

void SM::PushAllocator(BuiltInMemoryAllocator builtInAllocator)
{
    s_allocatorStack.Push(GetBuiltInAllocator(builtInAllocator));
}

void SM::PopAllocator()
{
    // pop allocator from the stack
    s_allocatorStack.Pop();
}

LinearAllocator* SM::GetCurrentAllocator()
{
    LinearAllocator* allocator = nullptr;
    bool didGetAllocator = s_allocatorStack.Top(&allocator);
    SM_ASSERT(didGetAllocator);
    return allocator;
}
