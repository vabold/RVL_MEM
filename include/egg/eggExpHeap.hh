#pragma once

#include <egg/eggHeap.hh>

#include <sdk/expHeap.hh>

namespace EGG
{

class ExpHeap : public Heap
{
public:
    class GroupSizeRecord
    {
    public:
        GroupSizeRecord( );
        void reset( );
        size_t getGroupSize( u16 groupID ) const;
        void addSize( u16 groupID, size_t size );

    private:
        std::array<size_t, 256> mEntries;
    };

    ~ExpHeap( ) override;
    void destroy( ) override;
    Kind getHeapKind( ) const override;
    void *alloc( size_t size, s32 align ) override;
    void free( void *block ) override;
    u32 getAllocatableSize( s32 align = 4 ) const override;

    static void addGroupSize( void *block, RVL::MEMiHeapHead *heap, uintptr_t param );
    void calcGroupSize( GroupSizeRecord *record );

    void setGroupID( u16 groupID );

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
