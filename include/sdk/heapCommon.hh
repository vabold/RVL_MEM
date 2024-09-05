#pragma once

#include <sdk/list.hh>

#include <array>
#include <cstring>

namespace RVL
{

class MEMiHeapHead
{
protected:
    MEMiHeapHead( u32 signature, void *heapStart, void *heapEnd, u16 opt );
    ~MEMiHeapHead( );

    void fillNoUseMemory( void *address, u32 size );
    void fillAllocMemory( void *address, u32 size );
    void fillFreeMemory( void *address, u32 size );

public:
    MEMList &getChildList( );
    void *getHeapStart( );
    void *getHeapEnd( );

    static MEMList &getRootList( );
    static u32 getFillVal( u32 type );
    static MEMiHeapHead *findContainHeap( const void *block );

    static constexpr u16 getLinkOffset( )
    {
        return offsetof( MEMiHeapHead, mLink );
    }

private:
    static MEMiHeapHead *findContainHeap( MEMList *list, const void *block );
    MEMList &findListContainHeap( ) const;

    u32 mSignature;
    u16 mOptFlag;
    MEMLink mLink;
    MEMList mChildList;
    void *mHeapStart;
    void *mHeapEnd;

    static MEMList sRootList;
    static constexpr std::array<u32, 3> sFillVals = { {
            0xC3C3C3C3,
            0xF3F3F3F3,
            0xD3D3D3D3,
    } };
};

} // namespace RVL
