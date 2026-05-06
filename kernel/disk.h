#pragma once
#include <efi.h>
#include "display.h"
#include "utils.h"

void disk_init(EFI_SYSTEM_TABLE* sys);
void disk_read_sector(unsigned int lba, void* buf);