#pragma once

#include <Common.hh>

namespace RVL
{

struct MEMLink
{
    void *prevObject;
    void *nextObject;
};

struct MEMList
{
    void *headObject;
    void *tailObject;
    u16 numObjects;
    u16 offset;
};

void MEMInitList( MEMList *list, u16 offset );
void MEMAppendListObject( MEMList *list, void *object );
void MEMRemoveListObject( MEMList *list, void *object );
void *MEMGetNextListObject( MEMList *list, void *object );

} // namespace RVL
