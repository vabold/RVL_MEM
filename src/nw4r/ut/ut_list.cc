#include <nw4r/ut/list.hh>

namespace nw4r::ut
{

static inline Link *GetLink( const List *list, const void *object )
{
    return reinterpret_cast<Link *>( GetAddrNum( object ) + list->offset );
}

void List_Init( List *list, u16 offset )
{
    list->headObject = NULL;
    list->tailObject = NULL;
    list->numObjects = 0;
    list->offset = offset;
}

static void SetFirstObject( List *list, void *object )
{
    Link *link = GetLink( list, object );

    link->nextObject = NULL;
    link->prevObject = NULL;

    list->headObject = object;
    list->tailObject = object;
    ++list->numObjects;
}

void List_Append( List *list, void *object )
{
    if( !list->headObject )
    {
        SetFirstObject( list, object );
        return;
    }

    Link *link = GetLink( list, object );
    link->prevObject = list->tailObject;
    link->nextObject = NULL;

    GetLink( list, list->tailObject )->nextObject = object;
    list->tailObject = object;

    ++list->numObjects;
}

void List_Remove( List *list, void *object )
{
    Link *link = GetLink( list, object );

    if( !link->prevObject )
    {
        list->headObject = link->nextObject;
    }
    else
    {
        GetLink( list, link->prevObject )->nextObject = link->nextObject;
    }

    if( !link->nextObject )
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

void *List_GetNext( const List *list, const void *object )
{
    return !object ? list->headObject : GetLink( list, object )->nextObject;
}

} // namespace nw4r::ut
