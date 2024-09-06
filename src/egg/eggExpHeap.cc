#include <egg/eggExpHeap.hh>

#include <sdk/expHeap.hh>

#include <limits>

namespace EGG
{

ExpHeap::ExpHeap( RVL::MEMiHeapHead *handle ) : Heap( handle ) {}

ExpHeap::~ExpHeap( )
{
    dispose( );
    dynamicCastHandleToExp( )->destroy( );
}

ExpHeap *ExpHeap::create( void *startAddress, size_t size, u16 opt )
{
    ExpHeap *heap = nullptr;
    void *buffer = startAddress;

    void *endAddress = RoundDown( AddOffset( startAddress, size ), 4 );
    startAddress = RoundUp( startAddress, 4 );

    size_t addrRange = GetAddrNum( endAddress ) - GetAddrNum( startAddress );
    if( startAddress > endAddress || addrRange < sizeof( ExpHeap ) + 4 )
    {
        return nullptr;
    }

    void *handleStart = AddOffset( startAddress, sizeof( ExpHeap ) );
    RVL::MEMiExpHeapHead *handle =
            RVL::MEMiExpHeapHead::create( handleStart, addrRange - sizeof( ExpHeap ), opt );
    if( handle )
    {
        heap = new( startAddress ) ExpHeap( handle );
        heap->registerHeapBuffer( buffer );
    }

    return heap;
}

ExpHeap *ExpHeap::create( size_t size, Heap *pHeap, u16 opt )
{
    ExpHeap *heap = nullptr;

    if( !pHeap )
    {
        pHeap = Heap::getCurrentHeap( );
    }

    if( size == std::numeric_limits<size_t>::max( ) )
    {
        size = pHeap->getAllocatableSize( );
    }

    void *block = pHeap->alloc( size, 4 );
    if( block )
    {
        heap = create( block, size, opt );
        if( heap )
        {
            heap->setParentHeap( pHeap );
        }
        else
        {
            pHeap->free( block );
        }
    }

    return heap;
}

void ExpHeap::destroy( )
{
    Heap *pParent = getParentHeap( );
    this->~ExpHeap( );
    if( pParent )
    {
        pParent->free( this );
    }
}

Heap::Kind ExpHeap::getHeapKind( ) const
{
    return Heap::Kind::Expanded;
}

void *ExpHeap::alloc( size_t size, s32 align )
{
    if( tstDisableAllocation( ) )
    {
        PANIC( "HEAP ALLOC FAIL (%p, %s): Heap is locked", this, mName );
    }

    return dynamicCastHandleToExp( )->alloc( size, align );
}

void ExpHeap::free( void *block )
{
    dynamicCastHandleToExp( )->free( block );
}

u32 ExpHeap::getAllocatableSize( s32 align ) const
{
    return dynamicCastHandleToExp( )->getAllocatableSize( align );
}

RVL::MEMiExpHeapHead *ExpHeap::dynamicCastHandleToExp( )
{
    return reinterpret_cast<RVL::MEMiExpHeapHead *>( mHandle );
}

const RVL::MEMiExpHeapHead *ExpHeap::dynamicCastHandleToExp( ) const
{
    return reinterpret_cast<RVL::MEMiExpHeapHead *>( mHandle );
}

void ExpHeap::initRootHeap( void *startAddress, size_t size )
{
    sRootHeap = create( startAddress, size, 2 );
    sRootHeap->setName( "EGGRoot" );
    sRootHeap->becomeCurrentHeap( );
}

ExpHeap *ExpHeap::getRootHeap( )
{
    return sRootHeap;
}

ExpHeap *ExpHeap::sRootHeap = nullptr;

} // namespace EGG
