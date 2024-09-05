#include <sdk/heapCommon.hh>

#include <cstring>

namespace RVL
{

MEMiHeapHead::MEMiHeapHead( u32 signature, void *heapStart, void *heapEnd, u16 opt )
    : mChildList( MEMList( getLinkOffset( ) ) )
{
    mSignature = signature;
    mHeapStart = heapStart;
    mHeapEnd = heapEnd;
    mOptFlag = opt;

    fillNoUseMemory( heapStart, GetAddrNum( heapEnd ) - GetAddrNum( heapStart ) );

    findListContainHeap( ).append( this );
}

MEMiHeapHead::~MEMiHeapHead( )
{
    findListContainHeap( ).remove( this );
    mSignature = 0;
}

void MEMiHeapHead::fillNoUseMemory( void *address, u32 size )
{
    if( mOptFlag & 2 )
    {
        memset( address, getFillVal( 0 ), size );
    }
}

void MEMiHeapHead::fillAllocMemory( void *address, u32 size )
{
    if( mOptFlag & 1 )
    {
        memset( address, 0, size );
    }
    else if( mOptFlag & 2 )
    {
        memset( address, getFillVal( 1 ), size );
    }
}

void MEMiHeapHead::fillFreeMemory( void *address, u32 size )
{
    if( mOptFlag & 2 )
    {
        memset( address, getFillVal( 2 ), size );
    }
}

MEMiHeapHead *MEMiHeapHead::findContainHeap( const void *block )
{
    return findContainHeap( &sRootList, block );
}

MEMiHeapHead *MEMiHeapHead::findContainHeap( MEMList *list, const void *block )
{
    MEMiHeapHead *heap = nullptr;
    while( ( heap = reinterpret_cast<MEMiHeapHead *>( list->getNext( heap ) ) ) )
    {
        if( GetAddrNum( heap->mHeapStart ) > GetAddrNum( block ) ||
                GetAddrNum( block ) >= GetAddrNum( heap->mHeapEnd ) )
        {
            continue;
        }

        MEMiHeapHead *search = findContainHeap( &heap->mChildList, block );
        if( search )
        {
            return search;
        }

        return heap;
    }

    return nullptr;
}

MEMList &MEMiHeapHead::findListContainHeap( ) const
{
    MEMiHeapHead *containHeap = findContainHeap( this );
    return containHeap ? containHeap->getChildList( ) : getRootList( );
}

MEMList &MEMiHeapHead::getChildList( )
{
    return mChildList;
}

MEMList &MEMiHeapHead::getRootList( )
{
    return sRootList;
}

void *MEMiHeapHead::getHeapStart( )
{
    return mHeapStart;
}

void *MEMiHeapHead::getHeapEnd( )
{
    return mHeapEnd;
}

u32 MEMiHeapHead::getFillVal( u32 type )
{
    ASSERT( type < 3 );
    return sFillVals[ type ];
}

MEMList MEMiHeapHead::sRootList = MEMList( MEMiHeapHead::getLinkOffset( ) );

} // namespace RVL
