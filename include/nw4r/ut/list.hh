#pragma once

#include <Common.hh>

namespace nw4r::ut
{

struct List
{
    void *headObject;
    void *tailObject;
    u16 numObjects;
    u16 offset;
};

struct Link
{
    void *prevObject;
    void *nextObject;
};

void List_Init( List *list, u16 offset );
void List_Append( List *list, void *object );
void List_Remove( List *list, void *object );
void *List_GetNext( const List *list, const void *object );

} // namespace nw4r::ut
