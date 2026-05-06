#pragma once

typedef unsigned int  uint32;
typedef unsigned char uint8;

void  memory_init(void);
void* memory_alloc(uint32 size);
void  memory_free(void* ptr);  // пока ничего не делает
uint32 memory_used(void);