#include "memory.h"

typedef struct Block {
    uint32 size;
    int free;
    struct Block* next;
} Block;

static uint8 heap[1024 * 1024];
static Block* head = 0;

void memory_init(void) {
    head = (Block*)heap;
    head->size = sizeof(heap) - sizeof(Block);
    head->free = 1;
    head->next = 0;
}

void* memory_alloc(uint32 size) {
    Block* cur = head;
    while (cur) {
        if (cur->free && cur->size >= size) {
            // делим блок если остаток достаточно большой
            if (cur->size >= size + sizeof(Block) + 16) {
                Block* newb = (Block*)((uint8*)cur + sizeof(Block) + size);
                newb->size = cur->size - size - sizeof(Block);
                newb->free = 1;
                newb->next = cur->next;
                cur->next = newb;
                cur->size = size;
            }
            cur->free = 0;
            return (void*)((uint8*)cur + sizeof(Block));
        }
        cur = cur->next;
    }
    return 0;
}

void memory_free(void* ptr) {
    if (!ptr) return;
    Block* b = (Block*)((uint8*)ptr - sizeof(Block));
    b->free = 1;

    // merge соседних свободных блоков
    Block* cur = head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += sizeof(Block) + cur->next->size;
            cur->next = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

uint32 memory_used(void) {
    uint32 used = 0;
    Block* cur = head;
    while (cur) {
        if (!cur->free) used += cur->size;
        cur = cur->next;
    }
    return used;
}