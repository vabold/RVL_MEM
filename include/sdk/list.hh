#pragma once

#include <Common.hh>

namespace RVL
{

struct MEMLink
{
    void *mPrevObject;
    void *mNextObject;
};

struct MEMList
{
    MEMList( u16 offset );
    void append( void *object );
    void remove( void *object );
    void *getNext( void *object );

    void *mHeadObject;
    void *mTailObject;
    u16 mNumObjects;
    u16 mOffset;

private:
    void setFirstObject( void *object );
    MEMLink *getLink( void *object );
};

} // namespace RVL
