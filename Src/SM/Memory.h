#pragma once

#include "SM/StandardTypes.h"

namespace SM
{
    enum BuiltInMemoryAllocator
    {
        kEngineGlobal,
        kNumBuiltInArenas
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
        kAlign1KiB = KiB(1),
        kAlign1MiB = MiB(1),
        kAlign2MiB = MiB(2)
    };

    class LinearAllocator
    {
        public:
        void Init(void* storage, size_t size);
        void* Alloc(size_t sizeBytes, U32 alignment = kAlign1);
        template<typename T>
        T* Alloc(size_t numElements);
        template<typename T>
        T* Alloc();
        void Reset();

        void* m_pMemory = nullptr;
        size_t m_size = 0;
        U32 m_allocatedBytes = 0;
    };

    void InitBuiltInAllocators();
    LinearAllocator* GetBuiltInAllocator(BuiltInMemoryAllocator builtInArena);

    void PushAllocator(LinearAllocator* allocator);
    void PushAllocator(BuiltInMemoryAllocator builtInAllocator);
    void PopAllocator();
    LinearAllocator* GetCurrentAllocator();

    class ScopedAllocator
    {
    public:
        ScopedAllocator(LinearAllocator* allocator)
        {
            PushAllocator(allocator);
        }

        ScopedAllocator(BuiltInMemoryAllocator builtInAllocator)
        {
            PushAllocator(GetBuiltInAllocator(builtInAllocator));
        }

        ~ScopedAllocator()
        {
            PopAllocator();
        }
    };

    #define PushScopedStackAllocator(stackMemorySize) \
        LinearAllocator stackLinearAllocator; \
        void* stackMemory = _alloca(stackMemorySize); \
        stackLinearAllocator.Init(stackMemory, stackMemorySize); \
        ScopedAllocator scopedAllocator##__LINE__(&stackLinearAllocator);

    inline void* Alloc(size_t sizeBytes, U32 alignment = kAlign1)
    {
        return GetCurrentAllocator()->Alloc(sizeBytes, alignment);
    }

    template<typename T>
    T* Alloc(size_t numElements)
    {
        return GetCurrentAllocator()->Alloc<T>(numElements);
    }

    template<typename T>
    T* Alloc()
    {
        return GetCurrentAllocator()->Alloc<T>();
    }

    inline void* Alloc(BuiltInMemoryAllocator builtInAllocator, size_t sizeBytes, U32 alignment = kAlign1)
    {
        return GetBuiltInAllocator(builtInAllocator)->Alloc(sizeBytes, alignment);
    }

    template<typename T>
    T* Alloc(BuiltInMemoryAllocator builtInAllocator, size_t numElements)
    {
        return GetBuiltInAllocator(builtInAllocator)->Alloc<T>(numElements);
    }

    template<typename T>
    T* Alloc(BuiltInMemoryAllocator builtInAllocator)
    {
        return GetBuiltInAllocator(builtInAllocator)->Alloc<T>();
    }
}
