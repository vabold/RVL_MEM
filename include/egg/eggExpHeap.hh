#pragma once

#include <egg/eggHeap.hh>

#include <sdk/expHeap.hh>

namespace EGG
{

class ExpHeap : public Heap
{
public:
    ~ExpHeap( ) override;
    void destroy( ) override;
    Kind getHeapKind( ) const override;
    void *alloc( size_t size, s32 align ) override;
    void free( void *block ) override;
    u32 getAllocatableSize( s32 align = 4 ) const override;

    RVL::MEMiExpHeapHead *dynamicCastHandleToExp( );
    const RVL::MEMiExpHeapHead *dynamicCastHandleToExp( ) const;

    static ExpHeap *create( void *startAddress, size_t size, u16 opt );
    static ExpHeap *create( size_t size, Heap *heap, u16 opt );

    static void initRootHeap( void *startAddress, size_t size );

    static ExpHeap *getRootHeap( );

private:
    ExpHeap( RVL::MEMiHeapHead *handle );

    static ExpHeap *sRootHeap;
};

} // namespace EGG
