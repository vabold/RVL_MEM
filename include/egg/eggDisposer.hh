#pragma once

#include <sdk/list.hh>

namespace EGG
{

class Heap;

class Disposer
{
    friend class Heap;

public:
    static constexpr u16 getLinkOffset( )
    {
        return offsetof( Disposer, mLink );
    }

protected:
    Disposer( );
    virtual ~Disposer( );

private:
    Heap *mHeap;
    RVL::MEMLink mLink;
};

} // namespace EGG
