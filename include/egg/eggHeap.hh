#pragma once

#include <egg/eggDisposer.hh>

#include <sdk/heapCommon.hh>

#include <new>

namespace EGG
{

class ExpHeap;

class Heap : Disposer
{
public:
    enum class Kind
    {
        None,
        Expanded,
        Frame,
        Unit,
        Assert,
    };

    Heap( RVL::MEMiHeapHead *handle );
    virtual ~Heap( );

    virtual void destroy( ) = 0;
    virtual Kind getHeapKind( ) const = 0;
    virtual void *alloc( size_t size, s32 align ) = 0;
    virtual void free( void *block ) = 0;
    virtual u32 getAllocatableSize( s32 align = 4 ) const = 0;

    void dispose( );

    void disableAllocation( );
    void enableAllocation( );
    bool tstDisableAllocation( ) const;

    void appendDisposer( Disposer *disposer );
    void removeDisposer( Disposer *disposer );

    Heap *becomeAllocatableHeap( );
    Heap *becomeCurrentHeap( );
    void registerHeapBuffer( void *buffer );

    void *getStartAddress( );
    void *getEndAddress( );

    const char *getName( ) const;
    Heap *getParentHeap( ) const;

    void setName( const char *name );
    void setParentHeap( Heap *heap );

    static void initialize( );
    static void *alloc( size_t size, int align, Heap *pHeap );
    static void free( void *block, Heap *pHeap );

    static Heap *findHeap( RVL::MEMiHeapHead *handle );
    static Heap *findContainHeap( const void *block );

    static ExpHeap *dynamicCastToExp( Heap *heap );
    static Heap *getCurrentHeap( );
    static void *getMemorySpace( );

    static constexpr uintptr_t getOffset( )
    {
        // offsetof doesn't work, so instead of hardcoding an offset, we derive it ourselves
        return reinterpret_cast<uintptr_t>( &reinterpret_cast<Heap *>( NULL )->mLink );
    }

protected:
    RVL::MEMiHeapHead *mHandle;
    void *mBlock;
    Heap *mParentHeap;
    u16 mFlags;
    RVL::MEMLink mLink;
    RVL::MEMList mChildren;
    const char *mName;

    static RVL::MEMList sHeapList;

    static Heap *sCurrentHeap;
    static bool sIsHeapInitialized;
    static Heap *sAllocatableHeap;
    static ExpHeap *sRootHeap;
    static void *sMemorySpace;
};

} // namespace EGG

void *operator new( size_t size ) noexcept;
void *operator new( size_t size, int align ) noexcept;
void *operator new( size_t size, EGG::Heap *heap, int align ) noexcept;
void *operator new[]( size_t size ) noexcept;
void *operator new[]( size_t size, int align ) noexcept;
void *operator new[]( size_t size, EGG::Heap *heap, int align ) noexcept;
void operator delete( void *block ) noexcept;
void operator delete[]( void *block ) noexcept;
