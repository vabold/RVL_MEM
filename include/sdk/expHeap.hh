#pragma once

#include <sdk/heapCommon.hh>

namespace RVL
{

struct MEMiExpBlockHead;

struct MEMiExpBlockLink
{
    MEMiExpBlockHead *prev;
    MEMiExpBlockHead *next;
};

struct MEMiExpBlockList
{
    MEMiExpBlockHead *head;
    MEMiExpBlockHead *tail;
};

struct MEMiExpBlockHead
{
    u16 signature;
    union
    {
        u16 val;
        struct
        {
            u16 direction : 1;
            u16 alignment : 7;
            u16 groupId : 8;
        } fields;
    } attribute;
    u32 size;
    MEMiExpBlockLink link;
};

struct MEMiExpHeapHead : MEMiHeapHead
{
    MEMiExpBlockList freeBlocks;
    MEMiExpBlockList usedBlocks;
    u16 groupId;
    u16 attribute;
};

MEMiExpHeapHead *MEMCreateExpHeapEx( void *startAddress, size_t size, u16 flag );
void *MEMDestroyExpHeap( MEMiExpHeapHead *heap );
void *MEMAllocFromExpHeapEx( MEMiExpHeapHead *heap, size_t size, s32 align );
void MEMFreeToExpHeap( MEMiExpHeapHead *heap, void *block );
u32 MEMGetAllocatableSizeForExpHeapEx( MEMiExpHeapHead *heap, s32 align );

} // namespace RVL
