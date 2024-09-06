#pragma once

#include <sdk/heapCommon.hh>

#include <functional>

namespace RVL
{

struct Region
{
    Region( void *start, void *end );
    uintptr_t getRange( ) const;

    void *start;
    void *end;
};

struct MEMiExpBlockHead;

struct MEMiExpBlockLink
{
    MEMiExpBlockHead *prev;
    MEMiExpBlockHead *next;
};

struct MEMiExpBlockList
{
    MEMiExpBlockHead *insert( MEMiExpBlockHead *block, MEMiExpBlockHead *prev );
    MEMiExpBlockHead *append( MEMiExpBlockHead *block );
    MEMiExpBlockHead *remove( MEMiExpBlockHead *block );

    MEMiExpBlockHead *mHead;
    MEMiExpBlockHead *mTail;
};

struct MEMiExpBlockHead
{
private:
    MEMiExpBlockHead( const Region &region, u16 signature );

public:
    static MEMiExpBlockHead *createFree( const Region &region );
    static MEMiExpBlockHead *createUsed( const Region &region );

    Region getRegion( ) const;
    void *getMemoryStart( ) const;
    void *getMemoryEnd( ) const;

    u16 mSignature;
    union
    {
        u16 val;
        struct
        {
            u16 direction : 1;
            u16 alignment : 7;
            u16 groupId : 8;
        } fields;
    } mAttribute;
    u32 mSize;
    MEMiExpBlockLink mLink;
};

class MEMiExpHeapHead : public MEMiHeapHead
{
private:
    MEMiExpHeapHead( void *end, u16 opt );
    ~MEMiExpHeapHead( );

public:
    typedef std::function<void( void *, MEMiHeapHead *, uintptr_t )> Visitor;

    static MEMiExpHeapHead *create( void *startAddress, size_t size, u16 flag );
    void destroy( );

    void *alloc( size_t size, s32 align );
    void free( void *block );
    u32 getAllocatableSize( s32 align ) const;
    void visitAllocated( Visitor visitor, uintptr_t param );

    u16 getGroupID( ) const;
    void setGroupID( u16 groupID );

private:
    void *allocFromHead( size_t size, s32 alignment );
    void *allocFromTail( size_t size, s32 alignment );
    void *allocUsedBlockFromFreeBlock( MEMiExpBlockHead *block, void *address, u32 size,
            s32 direction );
    bool recycleRegion( const Region &initialRegion );

    MEMiExpBlockList mFreeBlocks;
    MEMiExpBlockList mUsedBlocks;
    u16 mGroupId;
    u16 mAttribute;

    static constexpr u32 EXP_HEAP_SIGNATURE = 0x45585048; // EXPH
};

} // namespace RVL
