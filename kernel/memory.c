#include "memory.h"

// 1MB heap
static uint8 heap[1024 * 1024];
static uint32 heap_pos = 0;

void memory_init(void) {
    heap_pos = 0;  // сбрасываем в начало
}

void* memory_alloc(uint32 size) {
    // проверяем что не вышли за пределы
    if (heap_pos + size > sizeof(heap)) {
        return 0;  // нет памяти
    }
    void* ptr = &heap[heap_pos];
    heap_pos += size;
    return ptr;
}

void memory_free(void* ptr) {
    // bump allocator не умеет освобождать
    // просто игнорируем
    (void)ptr;
}
uint32 memory_used(void) {
    return heap_pos;
}