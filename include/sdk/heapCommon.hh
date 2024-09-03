#pragma once

#include <sdk/list.hh>

#include <cstring>

namespace RVL
{

struct MEMiHeapHead
{
    u32 signature;
    MEMLink link;
    MEMList childList;
    void *heapStart;
    void *heapEnd;
    u16 optFlag;
};

void MEMiInitHeapHead( MEMiHeapHead *heap, u32 signature, void *heapStart, void *heapEnd, u16 opt );
void MEMiFinalizeHeap( MEMiHeapHead *heap );
MEMiHeapHead *MEMFindContainHeap( const void *block );

void *MEMGetHeapEndAddress( MEMiHeapHead *heap );
u32 MEMGetFillValForHeap( u32 type );

namespace detail
{

static inline void FillNoUseMemory( MEMiHeapHead *heap, void *address, u32 size )
{
    if( heap->optFlag & 2 )
    {
        memset( address, MEMGetFillValForHeap( 0 ), size );
    }
}

static inline void FillAllocMemory( MEMiHeapHead *heap, void *address, u32 size )
{
    if( heap->optFlag & 1 )
    {
        memset( address, 0, size );
    }
    else if( heap->optFlag & 2 )
    {
        memset( address, MEMGetFillValForHeap( 1 ), size );
    }
}

static inline void FillFreeMemory( MEMiHeapHead *heap, void *address, u32 size )
{
    if( heap->optFlag & 2 )
    {
        memset( address, MEMGetFillValForHeap( 2 ), size );
    }
}

} // namespace detail
} // namespace RVL
