#include <sdk/heapCommon.hh>

#include <cstring>

namespace RVL
{

static MEMList sRootList;
static bool sRootListInitialized = false;

static u32 sFillVals[ 3 ] = {
        0xC3C3C3C3,
        0xF3F3F3F3,
        0xD3D3D3D3,
};

static MEMiHeapHead *FindContainHeap_( MEMList *list, const void *block )
{
    MEMiHeapHead *heap = NULL;
    MEMiHeapHead *search;

    while( ( heap = reinterpret_cast<MEMiHeapHead *>( MEMGetNextListObject( list, heap ) ) ) )
    {
        if( GetAddrNum( heap->heapStart ) > GetAddrNum( block ) ||
                GetAddrNum( block ) >= GetAddrNum( heap->heapEnd ) )
        {
            continue;
        }

        search = FindContainHeap_( &heap->childList, block );
        if( search )
        {
            return search;
        }

        return heap;
    }

    return NULL;
}

static MEMList *FindListContainHeap_( MEMiHeapHead *heap )
{
    MEMList *list = &sRootList;
    MEMiHeapHead *containHeap = FindContainHeap_( list, heap );
    if( containHeap )
    {
        list = &containHeap->childList;
    }

    return list;
}

void MEMiInitHeapHead( MEMiHeapHead *heap, u32 signature, void *heapStart, void *heapEnd, u16 opt )
{
    heap->signature = signature;
    heap->heapStart = heapStart;
    heap->heapEnd = heapEnd;
    heap->optFlag = opt;

    u32 size = GetAddrNum( heapEnd ) - GetAddrNum( heapStart );
    detail::FillNoUseMemory( heap, heapStart, size );

    MEMInitList( &heap->childList, offsetof( MEMiHeapHead, link ) );

    if( !sRootListInitialized )
    {
        MEMInitList( &sRootList, offsetof( MEMiHeapHead, link ) );
        sRootListInitialized = true;
    }

    MEMAppendListObject( FindListContainHeap_( heap ), heap );
}

void MEMiFinalizeHeap( MEMiHeapHead *heap )
{
    MEMRemoveListObject( FindListContainHeap_( heap ), heap );
    heap->signature = 0;
}

MEMiHeapHead *MEMFindContainHeap( const void *block )
{
    return FindContainHeap_( &sRootList, block );
}

void *MEMGetHeapEndAddress( MEMiHeapHead *heap )
{
    return heap->heapEnd;
}

u32 MEMGetFillValForHeap( u32 type )
{
    ASSERT( type < 3 );
    return sFillVals[ type ];
}

} // namespace RVL
