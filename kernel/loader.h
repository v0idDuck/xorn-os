// kernel/loader.h
#pragma once
#include <efi.h>
#include "utils.h"

// коды ошибок
#define LOADER_OK           0
#define LOADER_ERR_NOTFOUND 1
#define LOADER_ERR_BADMAGIC 2
#define LOADER_ERR_NOMEM    3

// заголовок .xe файла
typedef struct {
    unsigned char  magic[2];      // 'V', 'D'
    unsigned short version;       // версия формата
    unsigned int   code_size;     // размер кода
    unsigned int   entry_offset;  // смещение точки входа
} XeHeader;

int loader_run(const char* path, void* api);
void loader_init(EFI_SYSTEM_TABLE* sys);