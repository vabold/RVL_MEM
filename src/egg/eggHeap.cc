#include <egg/eggExpHeap.hh>

#include <cstdio>
#include <cstdlib>

namespace EGG
{

Heap::Heap( RVL::MEMiHeapHead *handle ) : mHandle( handle )
{
    if( !sIsHeapListInitialized )
    {
        PANIC( "Cannot create a heap before calling Heap::initalize" );
    }

    mBlock = NULL;
    mParentHeap = NULL;
    mName = "NoName";
    mFlags = 0;

    nw4r::ut::List_Append( &sHeapList, this );
}

Heap::~Heap( )
{
    nw4r::ut::List_Remove( &sHeapList, this );
}

void Heap::disableAllocation( )
{
    mFlags |= 1;
}

void Heap::enableAllocation( )
{
    mFlags &= ~1;
}

bool Heap::tstDisableAllocation( ) const
{
    return mFlags & 1;
}

Heap *Heap::becomeAllocatableHeap( )
{
    Heap *oldHeap = sAllocatableHeap;
    sAllocatableHeap = this;
    return oldHeap;
}

Heap *Heap::becomeCurrentHeap( )
{
    Heap *oldHeap = sCurrentHeap;
    sCurrentHeap = this;
    return oldHeap;
}

void Heap::registerHeapBuffer( void *buffer )
{
    mBlock = buffer;
}

void *Heap::getStartAddress( )
{
    return this;
}

void *Heap::getEndAddress( )
{
    return RVL::MEMGetHeapEndAddress( mHandle );
}

const char *Heap::getName( ) const
{
    return mName;
}

Heap *Heap::getParentHeap( ) const
{
    return mParentHeap;
}

void Heap::setName( const char *name )
{
    mName = name;
}

void Heap::setParentHeap( Heap *heap )
{
    mParentHeap = heap;
}

void Heap::initialize( )
{
    constexpr size_t MEMORY_SPACE_SIZE = 0x1000000;

    // offsetof doesn't work, so instead of hardcoding an offset, we derive it ourselves
    uintptr_t offset = reinterpret_cast<uintptr_t>( &reinterpret_cast<Heap *>( NULL )->mLink );

    nw4r::ut::List_Init( &sHeapList, offset );
    sIsHeapListInitialized = true;

    sMemorySpace = malloc( MEMORY_SPACE_SIZE );
    ExpHeap::initRootHeap( sMemorySpace, MEMORY_SPACE_SIZE );
}

void *Heap::alloc( size_t size, int align, Heap *pHeap )
{
    Heap *currentHeap = sCurrentHeap;

    if( sAllocatableHeap )
    {
        if( currentHeap && !pHeap )
        {
            pHeap = currentHeap;
        }

        if( pHeap != sAllocatableHeap )
        {
            WARN( "HEAP ALLOC FAIL (%p, %s): Allocatable heap is %p (%s)", pHeap, pHeap->getName( ),
                    sAllocatableHeap, sAllocatableHeap->getName( ) );

            return NULL;
        }
    }

    if( pHeap )
    {
        return pHeap->alloc( size, align );
    }

    if( currentHeap )
    {
        void *block = currentHeap->alloc( size, align );

        if( !block )
        {
            u32 heapFreeSize = currentHeap->getAllocatableSize( 0x4 );
            s32 heapSize = GetAddrNum( currentHeap->getEndAddress( ) ) -
                    GetAddrNum( currentHeap->getStartAddress( ) );

            constexpr f32 BYTES_TO_MBYTES = 1024.0f * 1024.0f;
            f32 heapSizeMB = static_cast<f32>( heapSize ) / BYTES_TO_MBYTES;
            f32 sizeMB = static_cast<f32>( size ) / BYTES_TO_MBYTES;

            WARN( "HEAP ALLOC FAIL (%p, %s):\nFree bytes: %d (%.1fMBytes)\nAlloc bytes: %d "
                  "(%.1fMBytes)\nAlign: %d",
                    currentHeap, currentHeap->getName( ), heapFreeSize, heapSizeMB, size, sizeMB,
                    align );
        }

        return block;
    }

    WARN( "HEAP ALLOC FAIL: Cannot allocate %d from heap %p", size, pHeap );
    return NULL;
}

void Heap::free( void *block, Heap *pHeap )
{
    if( !pHeap )
    {
        RVL::MEMiHeapHead *handle = RVL::MEMFindContainHeap( block );
        if( !handle )
        {
            return;
        }

        pHeap = findHeap( handle );
        if( !pHeap )
        {
            return;
        }
    }

    pHeap->free( block );
}

Heap *Heap::findHeap( RVL::MEMiHeapHead *handle )
{
    Heap *heap = NULL;

    if( sIsHeapListInitialized )
    {
        Heap *node = NULL;
        while( ( node = reinterpret_cast<Heap *>( nw4r::ut::List_GetNext( &sHeapList, node ) ) ) )
        {
            if( node->mHandle == handle )
            {
                heap = node;
                break;
            }
        }
    }

    return heap;
}

Heap *Heap::findContainHeap( const void *block )
{
    Heap *heap = NULL;

    if( RVL::MEMiHeapHead *handle = RVL::MEMFindContainHeap( block ) )
    {
        heap = findHeap( handle );
    }

    return heap;
}

ExpHeap *Heap::dynamicCastToExp( Heap *heap )
{
    if( heap->getHeapKind( ) == Kind::Expanded )
    {
        return reinterpret_cast<ExpHeap *>( heap );
    }

    return NULL;
}

Heap *Heap::getCurrentHeap( )
{
    return sCurrentHeap;
}

void *Heap::getMemorySpace( )
{
    return sMemorySpace;
}

} // namespace EGG

void *operator new( size_t size ) noexcept
{
    return EGG::Heap::alloc( size, 4, NULL );
}

void *operator new( size_t size, int align ) noexcept
{
    return EGG::Heap::alloc( size, align, NULL );
}

void *operator new( size_t size, EGG::Heap *heap, int align ) noexcept
{
    return EGG::Heap::alloc( size, align, heap );
}

void *operator new( size_t /* size */, void *block ) noexcept
{
    return block;
}

void *operator new[]( size_t size ) noexcept
{
    return EGG::Heap::alloc( size, 4, NULL );
}

void *operator new[]( size_t size, int align ) noexcept
{
    return EGG::Heap::alloc( size, align, NULL );
}

void *operator new[]( size_t size, EGG::Heap *heap, int align ) noexcept
{
    return EGG::Heap::alloc( size, align, heap );
}

void operator delete( void *block ) noexcept
{
    EGG::Heap::free( block, NULL );
}

void operator delete( void *block, size_t size ) noexcept
{
    (void)size;
    EGG::Heap::free( block, NULL );
}

void operator delete[]( void *block ) noexcept
{
    EGG::Heap::free( block, NULL );
}

void operator delete[]( void *block, size_t size ) noexcept
{
    (void)size;
    EGG::Heap::free( block, NULL );
}

nw4r::ut::List EGG::Heap::sHeapList;

EGG::Heap *EGG::Heap::sCurrentHeap = NULL;
bool EGG::Heap::sIsHeapListInitialized = false;
EGG::Heap *EGG::Heap::sAllocatableHeap = NULL;
void *EGG::Heap::sMemorySpace = NULL;
