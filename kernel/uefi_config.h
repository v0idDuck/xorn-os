#pragma once
#include <efi.h>

// Читает key=value из файла на ESP через UEFI Simple File System
// Возвращает 1 если ключ найден, 0 если нет
int uefi_config_get(EFI_HANDLE handle, EFI_SYSTEM_TABLE* sys,
                    const CHAR16* filepath, const char* key,
                    char* value, int max_len);