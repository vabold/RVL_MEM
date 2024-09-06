#include <sdk/expHeap.hh>

#include <limits>
#include <new> // placement new

namespace RVL
{

// ================================
//     REGION FUNCTIONS
// ================================

Region::Region( void *start, void *end ) : start( start ), end( end ) {}

uintptr_t Region::getRange( ) const
{
    return GetAddrNum( end ) - GetAddrNum( start );
}

// ================================
//     LIST FUNCTIONS
// ================================

MEMiExpBlockHead *MEMiExpBlockList::insert( MEMiExpBlockHead *block, MEMiExpBlockHead *prev )
{
    MEMiExpBlockHead *next;

    block->mLink.prev = prev;
    if( prev )
    {
        next = prev->mLink.next;
        prev->mLink.next = block;
    }
    else
    {
        next = mHead;
        mHead = block;
    }

    block->mLink.next = next;
    if( next )
    {
        next->mLink.prev = block;
    }
    else
    {
        mTail = block;
    }

    return block;
}

MEMiExpBlockHead *MEMiExpBlockList::append( MEMiExpBlockHead *block )
{
    return insert( block, mTail );
}

MEMiExpBlockHead *MEMiExpBlockList::remove( MEMiExpBlockHead *block )
{
    MEMiExpBlockHead *prev = block->mLink.prev;
    MEMiExpBlockHead *next = block->mLink.next;

    if( prev )
    {
        prev->mLink.next = next;
    }
    else
    {
        mHead = next;
    }

    if( next )
    {
        next->mLink.prev = prev;
    }
    else
    {
        mTail = prev;
    }

    return prev;
}

// ================================
//     BLOCK FUNCTIONS
// ================================

MEMiExpBlockHead::MEMiExpBlockHead( const Region &region, u16 signature )
{
    mSignature = signature;
    mAttribute.val = 0;

    mSize = region.getRange( ) - sizeof( MEMiExpBlockHead );

    mLink.prev = nullptr;
    mLink.next = nullptr;
}

MEMiExpBlockHead *MEMiExpBlockHead::createFree( const Region &region )
{
    constexpr u16 FREE_BLOCK_SIGNATURE = 0x4652; // FR
    return new( region.start ) MEMiExpBlockHead( region, FREE_BLOCK_SIGNATURE );
}

MEMiExpBlockHead *MEMiExpBlockHead::createUsed( const Region &region )
{
    constexpr u16 USED_BLOCK_SIGNATURE = 0x5544; // UD
    return new( region.start ) MEMiExpBlockHead( region, USED_BLOCK_SIGNATURE );
}

Region MEMiExpBlockHead::getRegion( ) const
{
    return Region( SubOffset( this, mAttribute.fields.alignment ), getMemoryEnd( ) );
}

void *MEMiExpBlockHead::getMemoryStart( ) const
{
    return AddOffset( this, sizeof( MEMiExpBlockHead ) );
}

void *MEMiExpBlockHead::getMemoryEnd( ) const
{
    return AddOffset( getMemoryStart( ), mSize );
}

// ================================
//     HEAP FUNCTIONS
// ================================

MEMiExpHeapHead::MEMiExpHeapHead( void *end, u16 opt )
    : MEMiHeapHead( EXP_HEAP_SIGNATURE, AddOffset( this, sizeof( MEMiExpHeapHead ) ), end, opt )
{
    mGroupId = 0;
    mAttribute = 0;

    Region region = Region( getHeapStart( ), getHeapEnd( ) );
    MEMiExpBlockHead *block = MEMiExpBlockHead::createFree( region );

    mFreeBlocks.mHead = block;
    mFreeBlocks.mTail = block;
    mUsedBlocks.mHead = nullptr;
    mUsedBlocks.mTail = nullptr;
}

MEMiExpHeapHead::~MEMiExpHeapHead( ) = default;

MEMiExpHeapHead *MEMiExpHeapHead::create( void *startAddress, size_t size, u16 flag )
{
    void *endAddress = AddOffset( startAddress, size );

    startAddress = RoundUp( startAddress, 4 );
    endAddress = RoundDown( endAddress, 4 );

    uintptr_t startAddrNum = GetAddrNum( startAddress );
    uintptr_t endAddrNum = GetAddrNum( endAddress );

    if( startAddrNum > endAddrNum )
    {
        return nullptr;
    }

    if( endAddrNum - startAddrNum < sizeof( MEMiExpHeapHead ) + sizeof( MEMiExpBlockHead ) + 4 )
    {
        return nullptr;
    }

    return new( startAddress ) MEMiExpHeapHead( endAddress, flag );
}

void MEMiExpHeapHead::destroy( )
{
    this->~MEMiExpHeapHead( );
}

void *MEMiExpHeapHead::alloc( size_t size, s32 align )
{
    if( size == 0 )
    {
        size = 1;
    }
    size = RoundUp( size, 4 );

    void *block = nullptr;
    if( align >= 0 )
    {
        block = allocFromHead( size, align );
    }
    else
    {
        block = allocFromTail( size, -align );
    }

    return block;
}

void MEMiExpHeapHead::free( void *block )
{
    if( !block )
    {
        return;
    }

    MEMiExpBlockHead *head =
            reinterpret_cast<MEMiExpBlockHead *>( SubOffset( block, sizeof( MEMiExpBlockHead ) ) );

    Region region = head->getRegion( );
    mUsedBlocks.remove( head );
    recycleRegion( region );
}

u32 MEMiExpHeapHead::getAllocatableSize( s32 align ) const
{
    // Doesn't matter which direction it can be allocated from, take absolute value
    align = std::abs( align );

    u32 maxSize = 0;
    u32 x = std::numeric_limits<u32>::max( );

    for( MEMiExpBlockHead *block = mFreeBlocks.mHead; block; block = block->mLink.next )
    {
        void *memptr = block->getMemoryStart( );
        void *start = RoundUp( memptr, align );
        void *end = block->getMemoryEnd( );

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

// ================================
//     ALLOCATION FUNCTIONS
// ================================

void *MEMiExpHeapHead::allocFromHead( size_t size, s32 alignment )
{
    MEMiExpBlockHead *found = nullptr;
    u32 blockSize = -1;
    void *bestAddress = nullptr;

    for( MEMiExpBlockHead *block = mFreeBlocks.mHead; block; block = block->mLink.next )
    {
        void *const memptr = block->getMemoryStart( );
        void *const address = RoundUp( memptr, alignment );
        u32 offset = GetAddrNum( address ) - GetAddrNum( memptr );

        if( block->mSize < size + offset )
        {
            continue;
        }

        if( blockSize <= block->mSize )
        {
            continue;
        }

        found = block;
        blockSize = block->mSize;
        bestAddress = address;

        // This is a valid block to allocate in, but is it the best one?
        // heap->attribute & 1 decides whether or not we care
        if( !( mAttribute & 1 ) || blockSize == size )
        {
            break;
        }
    }

    if( !found )
    {
        return nullptr;
    }

    return allocUsedBlockFromFreeBlock( found, bestAddress, size, 0 );
}

void *MEMiExpHeapHead::allocFromTail( size_t size, s32 alignment )
{
    MEMiExpBlockHead *found = nullptr;
    u32 blockSize = -1;
    void *bestAddress = nullptr;

    for( MEMiExpBlockHead *block = mFreeBlocks.mTail; block; block = block->mLink.prev )
    {
        void *const start = block->getMemoryStart( );
        void *const endAddr = AddOffset( start, block->mSize );
        void *const end = RoundDown( SubOffset( endAddr, size ), alignment );

        if( static_cast<intptr_t>( GetAddrNum( end ) - GetAddrNum( start ) ) < 0 )
        {
            continue;
        }

        if( blockSize <= block->mSize )
        {
            continue;
        }

        found = block;
        blockSize = block->mSize;
        bestAddress = end;

        // This is a valid block to allocate in, but is it the best one?
        // heap->attribute & 1 decides whether or not we care
        if( !( mAttribute & 1 ) || blockSize == size )
        {
            break;
        }
    }

    if( !found )
    {
        return nullptr;
    }

    return allocUsedBlockFromFreeBlock( found, bestAddress, size, 1 );
}

void *MEMiExpHeapHead::allocUsedBlockFromFreeBlock( MEMiExpBlockHead *block, void *address,
        u32 size, s32 direction )
{
    // The left region represents the free block created to the left of the new memory block
    // The right region represents the free block created to the right of the new memory block
    // address -> address + size exists entirely between leftRegion.end and rightRegion.start
    Region leftRegion = block->getRegion( );
    Region rightRegion = Region( AddOffset( address, size ), leftRegion.end );
    leftRegion.end = SubOffset( address, sizeof( MEMiExpBlockHead ) );

    MEMiExpBlockHead *prev = mFreeBlocks.remove( block );

    if( leftRegion.getRange( ) < sizeof( MEMiExpBlockHead ) + 4 )
    {
        // Not enough room to insert a free memory block
        leftRegion.end = leftRegion.start;
    }
    else
    {
        prev = mFreeBlocks.insert( MEMiExpBlockHead::createFree( leftRegion ), prev );
    }

    if( rightRegion.getRange( ) < sizeof( MEMiExpBlockHead ) + 4 )
    {
        // Not enough room to insert a free memory block
        rightRegion.end = rightRegion.start;
    }
    else
    {
        mFreeBlocks.insert( MEMiExpBlockHead::createFree( rightRegion ), prev );
    }

    fillAllocMemory( leftRegion.end,
            GetAddrNum( rightRegion.start ) - GetAddrNum( leftRegion.end ) );

    // Now that the free blocks are cleared away, create a new used block
    Region region = Region( SubOffset( address, sizeof( MEMiExpBlockHead ) ), rightRegion.start );
    MEMiExpBlockHead *head = MEMiExpBlockHead::createUsed( region );

    head->mAttribute.fields.direction = direction;
    head->mAttribute.fields.alignment = GetAddrNum( head ) - GetAddrNum( leftRegion.end );
    head->mAttribute.fields.groupId = mGroupId;

    mUsedBlocks.append( head );

    return address;
}

bool MEMiExpHeapHead::recycleRegion( const Region &initialRegion )
{
    MEMiExpBlockHead *block = nullptr;
    Region region = initialRegion;

    for( MEMiExpBlockHead *search = mFreeBlocks.mHead; search; search = search->mLink.next )
    {
        if( search < initialRegion.start )
        {
            block = search;
            continue;
        }

        if( search == initialRegion.end )
        {
            region.end = search->getMemoryEnd( );
            mFreeBlocks.remove( search );
            fillNoUseMemory( search, sizeof( MEMiExpBlockHead ) );
        }

        break;
    }

    if( block && block->getMemoryEnd( ) == initialRegion.start )
    {
        region.start = block;
        block = mFreeBlocks.remove( block );
    }

    if( region.getRange( ) < sizeof( MEMiExpBlockHead ) )
    {
        return false;
    }

    fillFreeMemory( initialRegion.start, initialRegion.getRange( ) );
    mFreeBlocks.insert( MEMiExpBlockHead::createFree( region ), block );
    return true;
}

} // namespace RVL
