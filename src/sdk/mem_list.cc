#include <sdk/list.hh>

namespace RVL
{

static inline MEMLink *GetLink( MEMList *list, void *object )
{
    return reinterpret_cast<MEMLink *>( GetAddrNum( object ) + list->offset );
}

static void SetFirstObject_( MEMList *list, void *object )
{
    ASSERT( list != NULL );
    ASSERT( object != NULL );

    MEMLink *link = GetLink( list, object );

    link->nextObject = NULL;
    link->prevObject = NULL;

    list->headObject = object;
    list->tailObject = object;
    ++list->numObjects;
}

void MEMInitList( MEMList *list, u16 offset )
{
    ASSERT( list != NULL );

    list->headObject = NULL;
    list->tailObject = NULL;
    list->numObjects = 0;
    list->offset = offset;
}

void MEMAppendListObject( MEMList *list, void *object )
{
    ASSERT( list != NULL );
    ASSERT( object != NULL );

    if( list->headObject == NULL )
    {
        SetFirstObject_( list, object );
    }
    else
    {
        MEMLink *link = GetLink( list, object );

        link->prevObject = list->tailObject;
        link->nextObject = NULL;

        GetLink( list, list->tailObject )->nextObject = object;
        list->tailObject = object;
        ++list->numObjects;
    }
}

void MEMRemoveListObject( MEMList *list, void *object )
{
    ASSERT( list != NULL );
    ASSERT( object != NULL );

    MEMLink *link = GetLink( list, object );

    if( link->prevObject == NULL )
    {
        list->headObject = link->nextObject;
    }
    else
    {
        GetLink( list, link->prevObject )->nextObject = link->nextObject;
    }

    if( link->nextObject == NULL )
    {
        list->tailObject = link->prevObject;
    }
    else
    {
        GetLink( list, link->nextObject )->prevObject = link->prevObject;
    }

    link->prevObject = NULL;
    link->nextObject = NULL;
    --list->numObjects;
}

void *MEMGetNextListObject( MEMList *list, void *object )
{
    ASSERT( list != NULL );

    if( object == NULL )
    {
        return list->headObject;
    }
    else
    {
        return GetLink( list, object )->nextObject;
    }
}

} // namespace RVL
