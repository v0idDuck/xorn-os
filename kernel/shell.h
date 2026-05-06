#pragma once
#include <efi.h>
#include "display.h"
#include "keyboard.h"
#include "colors.h"
#include "memory.h"
#include "utils.h"
#include "fat32.h"

void shell_start(EFI_SYSTEM_TABLE* sys);
void int_to_str(int n, char* buf);
int str_eq(const char* a, const char* b);