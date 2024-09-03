#include <sdk/expHeap.hh>

#include <limits>

namespace RVL
{

struct Region
{
    void *start;
    void *end;
};

// ================================
//     MISC FUNCTIONS
// ================================

static inline void GetRegionOfMBlock_( Region *region, MEMiExpBlockHead *block )
{
    region->start = SubOffset( block, block->attribute.fields.alignment );
    region->end = AddOffset( AddOffset( block, sizeof( MEMiExpBlockHead ) ), block->size );
}

static inline MEMiExpBlockHead *InitMBlock_( Region *region, u16 signature )
{
    MEMiExpBlockHead *block = reinterpret_cast<MEMiExpBlockHead *>( region->start );

    block->signature = signature;
    block->attribute.val = 0;

    u32 size = GetAddrNum( region->end ) - ( GetAddrNum( block ) + sizeof( MEMiExpBlockHead ) );
    block->size = size;

    block->link.prev = NULL;
    block->link.next = NULL;

    return block;
}

static inline MEMiExpBlockHead *InitFreeMBlock_( Region *region )
{
    constexpr u16 FREE_BLOCK_SIGNATURE = 0x4652; // FR
    return InitMBlock_( region, FREE_BLOCK_SIGNATURE );
}

static inline MEMiExpBlockHead *InitUsedMBlock_( Region *region )
{
    constexpr u16 USED_BLOCK_SIGNATURE = 0x5544; // UD
    return InitMBlock_( region, USED_BLOCK_SIGNATURE );
}

static inline MEMiExpHeapHead *InitExpHeap_( MEMiExpHeapHead *heap, void *end, u16 opt )
{
    constexpr u32 EXP_HEAP_SIGNATURE = 0x45585048; // EXPH
    MEMiInitHeapHead( heap, EXP_HEAP_SIGNATURE, AddOffset( heap, sizeof( MEMiExpHeapHead ) ), end,
            opt );

    heap->groupId = 0;
    heap->attribute = 0;

    Region region;
    region.start = heap->heapStart;
    region.end = heap->heapEnd;
    MEMiExpBlockHead *block = InitFreeMBlock_( &region );

    heap->freeBlocks.head = block;
    heap->freeBlocks.tail = block;
    heap->usedBlocks.head = NULL;
    heap->usedBlocks.tail = NULL;

    return heap;
}

// ================================
//     LIST FUNCTIONS
// ================================

static inline MEMiExpBlockHead *InsertMBlock_( MEMiExpBlockList *list, MEMiExpBlockHead *block,
        MEMiExpBlockHead *prev )
{
    MEMiExpBlockHead *next;

    block->link.prev = prev;
    if( prev )
    {
        next = prev->link.next;
        prev->link.next = block;
    }
    else
    {
        next = list->head;
        list->head = block;
    }

    block->link.next = next;
    if( next )
    {
        next->link.prev = block;
    }
    else
    {
        list->tail = block;
    }

    return block;
}

static inline MEMiExpBlockHead *AppendMBlock_( MEMiExpBlockList *list, MEMiExpBlockHead *block )
{
    return InsertMBlock_( list, block, list->tail );
}

static inline MEMiExpBlockHead *RemoveMBlock_( MEMiExpBlockList *list, MEMiExpBlockHead *block )
{
    MEMiExpBlockHead *prev = block->link.prev;
    MEMiExpBlockHead *next = block->link.next;

    if( prev )
    {
        prev->link.next = next;
    }
    else
    {
        list->head = next;
    }

    if( next )
    {
        next->link.prev = prev;
    }
    else
    {
        list->tail = prev;
    }

    return prev;
}

// ================================
//     ALLOCATION FUNCTIONS
// ================================

static void *AllocUsedBlockFromFreeBlock_( MEMiExpHeapHead *heap, MEMiExpBlockHead *block,
        void *address, u32 size, s32 direction )
{
    Region region0;
    Region region1;

    GetRegionOfMBlock_( &region0, block );
    region1.end = region0.end;
    region1.start = AddOffset( address, size );
    region0.end = SubOffset( address, sizeof( MEMiExpBlockHead ) );

    MEMiExpBlockHead *prev = RemoveMBlock_( &heap->freeBlocks, block );

    if( GetAddrNum( region0.end ) - GetAddrNum( region0.start ) < sizeof( MEMiExpBlockHead ) + 4 )
    {
        region0.end = region0.start;
    }
    else
    {
        prev = InsertMBlock_( &heap->freeBlocks, InitFreeMBlock_( &region0 ), prev );
    }

    if( GetAddrNum( region1.end ) - GetAddrNum( region1.start ) < sizeof( MEMiExpBlockHead ) + 4 )
    {
        region1.end = region1.start;
    }
    else
    {
        InsertMBlock_( &heap->freeBlocks, InitFreeMBlock_( &region1 ), prev );
    }

    detail::FillAllocMemory( heap, region0.end,
            GetAddrNum( region1.start ) - GetAddrNum( region0.end ) );

    Region region2;
    region2.start = SubOffset( address, sizeof( MEMiExpBlockHead ) );
    region2.end = region1.start;
    MEMiExpBlockHead *head = InitUsedMBlock_( &region2 );

    head->attribute.fields.direction = direction;
    head->attribute.fields.alignment = GetAddrNum( head ) - GetAddrNum( region0.end );
    head->attribute.fields.groupId = heap->groupId;

    AppendMBlock_( &heap->usedBlocks, head );

    return address;
}

static void *AllocFromHead_( MEMiExpHeapHead *heap, size_t size, s32 alignment )
{
    MEMiExpBlockHead *found = NULL;
    u32 blockSize = -1;
    void *bestAddress = NULL;

    for( MEMiExpBlockHead *block = heap->freeBlocks.head; block; block = block->link.next )
    {
        void *const memptr = AddOffset( block, sizeof( MEMiExpBlockHead ) );
        void *const address = RoundUp( memptr, alignment );
        u32 offset = GetAddrNum( address ) - GetAddrNum( memptr );

        if( block->size < size + offset )
        {
            continue;
        }

        if( blockSize <= block->size )
        {
            continue;
        }

        found = block;
        blockSize = block->size;
        bestAddress = address;

        // This is a valid block to allocate in, but is it the best one?
        // heap->attribute & 1 decides whether or not we care
        if( !( heap->attribute & 1 ) || blockSize == size )
        {
            break;
        }
    }

    if( !found )
    {
        return NULL;
    }

    return AllocUsedBlockFromFreeBlock_( heap, found, bestAddress, size, 0 );
}

static void *AllocFromTail_( MEMiExpHeapHead *heap, size_t size, s32 alignment )
{
    MEMiExpBlockHead *found = NULL;
    u32 blockSize = -1;
    void *bestAddress = NULL;

    for( MEMiExpBlockHead *block = heap->freeBlocks.tail; block; block = block->link.prev )
    {
        void *const start = AddOffset( block, sizeof( MEMiExpBlockHead ) );
        void *const endAddr = AddOffset( start, block->size );
        void *const end = RoundDown( SubOffset( endAddr, size ), alignment );

        if( static_cast<intptr_t>( GetAddrNum( end ) - GetAddrNum( start ) ) < 0 )
        {
            continue;
        }

        if( blockSize <= block->size )
        {
            continue;
        }

        found = block;
        blockSize = block->size;
        bestAddress = end;

        // This is a valid block to allocate in, but is it the best one?
        // heap->attribute & 1 decides whether or not we care
        if( !( heap->attribute & 1 ) || blockSize == size )
        {
            break;
        }
    }

    if( !found )
    {
        return NULL;
    }

    return AllocUsedBlockFromFreeBlock_( heap, found, bestAddress, size, 1 );
}

static bool RecycleRegion_( MEMiExpHeapHead *heap, Region *initialRegion )
{
    MEMiExpBlockHead *block = NULL;
    Region region = *initialRegion;

    for( MEMiExpBlockHead *search = heap->freeBlocks.head; search; search = search->link.next )
    {
        if( search < initialRegion->start )
        {
            block = search;
            continue;
        }

        if( search == initialRegion->end )
        {
            region.end = AddOffset( AddOffset( search, sizeof( MEMiExpBlockHead ) ), search->size );
            RemoveMBlock_( &heap->freeBlocks, search );
            detail::FillNoUseMemory( heap, search, sizeof( MEMiExpBlockHead ) );
        }

        break;
    }

    if( block &&
            AddOffset( AddOffset( block, sizeof( MEMiExpBlockHead ) ), block->size ) ==
                    initialRegion->start )
    {
        region.start = block;
        block = RemoveMBlock_( &heap->freeBlocks, block );
    }

    if( GetAddrNum( region.end ) - GetAddrNum( region.start ) < sizeof( MEMiExpBlockHead ) )
    {
        return false;
    }

    detail::FillFreeMemory( heap, initialRegion->start,
            GetAddrNum( initialRegion->end ) - GetAddrNum( initialRegion->start ) );
    InsertMBlock_( &heap->freeBlocks, InitFreeMBlock_( &region ), block );
    return true;
}

// ================================
//     PUBLIC FUNCTIONS
// ================================

MEMiExpHeapHead *MEMCreateExpHeapEx( void *startAddress, size_t size, u16 flag )
{
    void *endAddress = AddOffset( startAddress, size );

    startAddress = RoundUp( startAddress, 4 );
    endAddress = RoundDown( endAddress, 4 );

    uintptr_t startAddrNum = GetAddrNum( startAddress );
    uintptr_t endAddrNum = GetAddrNum( endAddress );

    if( startAddrNum > endAddrNum ||
            endAddrNum - startAddrNum < sizeof( MEMiExpHeapHead ) + sizeof( MEMiExpBlockHead ) + 4 )
    {
        return NULL;
    }

    return InitExpHeap_( reinterpret_cast<MEMiExpHeapHead *>( startAddress ), endAddress, flag );
}

void *MEMDestroyExpHeap( MEMiExpHeapHead *heap )
{
    MEMiFinalizeHeap( heap );
    return heap;
}

void *MEMAllocFromExpHeapEx( MEMiExpHeapHead *heap, size_t size, s32 align )
{
    if( size == 0 )
    {
        size = 1;
    }
    size = RoundUp( size, 4 );

    void *block = NULL;
    if( align >= 0 )
    {
        block = AllocFromHead_( heap, size, align );
    }
    else
    {
        block = AllocFromTail_( heap, size, -align );
    }

    return block;
}

void MEMFreeToExpHeap( MEMiExpHeapHead *heap, void *block )
{
    if( !block )
    {
        return;
    }

    MEMiExpBlockHead *head =
            reinterpret_cast<MEMiExpBlockHead *>( SubOffset( block, sizeof( MEMiExpBlockHead ) ) );

    Region region;
    GetRegionOfMBlock_( &region, head );
    RemoveMBlock_( &heap->usedBlocks, head );
    RecycleRegion_( heap, &region );
}

u32 MEMGetAllocatableSizeForExpHeapEx( MEMiExpHeapHead *heap, s32 align )
{
    // Doesn't matter which direction it can be allocated from, take absolute value
    align = align >= 0 ? align : -align;

    u32 maxSize = 0;
    u32 x = std::numeric_limits<u32>::max( );

    for( MEMiExpBlockHead *block = heap->freeBlocks.head; block; block = block->link.next )
    {
        void *memptr = AddOffset( block, sizeof( MEMiExpBlockHead ) );
        void *start = RoundUp( memptr, align );
        void *end = AddOffset( memptr, block->size );

        if( GetAddrNum( start ) < GetAddrNum( end ) )
        {
            u32 size = GetAddrNum( end ) - GetAddrNum( start );
            u32 offset = GetAddrNum( start ) - GetAddrNum( memptr );

            if( maxSize < size || ( maxSize == size && x > offset ) )
            {
                maxSize = size;
                x = offset;
            }
        }
    }

    return maxSize;
}

} // namespace RVL
