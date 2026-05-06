// kernel/utils.h
#pragma once
#include <efi.h>

// ─── Строки (char) ───────────────────────────────────────
int  str_len(const char* s);
int  str_eq(const char* a, const char* b);
void str_copy(char* dst, const char* src);
void int_to_str(int n, char* buf);

// ─── Строки (CHAR16 / Unicode) ───────────────────────────
void wstr_to_str(CHAR16* src, char* dst);
int parse_args(char* line, char args[8][64]);
int str_to_int(const char* s);

// ─── Ввод UEFI ───────────────────────────────────────────
void   wait_for_key(EFI_SYSTEM_TABLE* sys);
CHAR16 get_key(EFI_SYSTEM_TABLE* sys);
void   read_line(EFI_SYSTEM_TABLE* sys, CHAR16* buf, int max_len);

// ─── Память ──────────────────────────────────────────────
void* memcpy(void* dst, const void* src, unsigned long size);
void* memset(void* ptr, int value, unsigned long size);