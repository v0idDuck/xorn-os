#pragma once
#include <efi.h>

void fat32_init(void);
int  fat32_read(const char* path, unsigned char** buf, unsigned int* size);
typedef struct {
    char name[16];
    int  is_dir;
    unsigned int size;
} FatEntry;

int fat32_ls(const char* path, FatEntry* entries, int max_count);