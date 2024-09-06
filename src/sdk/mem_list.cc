#include <sdk/list.hh>

namespace RVL
{

// ================================
//     PUBLIC FUNCTIONS
// ================================

MEMList::MEMList( u16 offset )
{
    mHeadObject = nullptr;
    mTailObject = nullptr;
    mNumObjects = 0;
    mOffset = offset;
}

void MEMList::append( void *object )
{
    ASSERT( object );

    if( !mHeadObject )
    {
        setFirstObject( object );
    }
    else
    {
        MEMLink *link = getLink( object );

        link->mPrevObject = mTailObject;
        link->mNextObject = nullptr;

        getLink( mTailObject )->mNextObject = object;
        mTailObject = object;
        ++mNumObjects;
    }
}

void MEMList::remove( void *object )
{
    ASSERT( object );

    MEMLink *link = getLink( object );

    if( !link->mPrevObject )
    {
        mHeadObject = link->mNextObject;
    }
    else
    {
        getLink( link->mPrevObject )->mNextObject = link->mNextObject;
    }

    if( !link->mNextObject )
    {
        mTailObject = link->mPrevObject;
    }
    else
    {
        getLink( link->mNextObject )->mPrevObject = link->mPrevObject;
    }

    link->mPrevObject = nullptr;
    link->mNextObject = nullptr;
    --mNumObjects;
}

void *MEMList::getFirst( )
{
    return getNext( nullptr );
}

void *MEMList::getNext( void *object )
{
    return object ? getLink( object )->mNextObject : mHeadObject;
}

// ================================
//     PRIVATE FUNCTIONS
// ================================

void MEMList::setFirstObject( void *object )
{
    ASSERT( object );

    MEMLink *link = getLink( object );

    link->mNextObject = nullptr;
    link->mPrevObject = nullptr;

    mHeadObject = object;
    mTailObject = object;
    ++mNumObjects;
}

MEMLink *MEMList::getLink( void *object )
{
    return reinterpret_cast<MEMLink *>( GetAddrNum( object ) + mOffset );
}

} // namespace RVL
