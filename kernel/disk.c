#include "disk.h"
#include "colors.h"
static EFI_BLOCK_IO* block_io = 0;
static unsigned int  media_id = 0;

void disk_init(EFI_SYSTEM_TABLE* sys) {
    EFI_GUID bio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    UINTN handle_count = 0;
    EFI_HANDLE* handles = 0;

    sys->BootServices->LocateHandleBuffer(
        ByProtocol, &bio_guid, 0, &handle_count, &handles
    );

    for (UINTN i = 0; i < handle_count; i++) {
        EFI_BLOCK_IO* bio = 0;
        sys->BootServices->HandleProtocol(
            handles[i], &bio_guid, (void**)&bio
        );

        if (!bio || !bio->Media) continue;
        if (!bio->Media->MediaPresent) continue;
        if (bio->Media->LogicalPartition) continue;
        if (bio->Media->BlockSize != 512) continue;
        if (bio->Media->LastBlock < 65536) continue;

        // читаем первый сектор
        unsigned char buf[512] __attribute__((aligned(512)));
        EFI_STATUS s = bio->ReadBlocks(bio, bio->Media->MediaId, 0, 512, buf);
        if (s != EFI_SUCCESS) continue;

        // проверяем FAT32 сигнатуру
        // bytes 82-89 = "FAT32   "
        if (buf[82] != 'F' || buf[83] != 'A' || buf[84] != 'T') continue;
        // проверяем boot сигнатуру
        if (buf[510] != 0x55 || buf[511] != 0xAA) continue;
        // метка
        if (buf[71] != 'X' || buf[72] != 'O' || buf[73] != 'R' ||
    buf[74] != 'N' || buf[75] != 'O' || buf[76] != 'S') continue;
        block_io = bio;
        media_id = bio->Media->MediaId;
        break;
    }

    sys->BootServices->FreePool(handles);

    if (block_io) {
        display_println("disk: found!", COLOR_GREEN);
    } else {
        display_println("disk: NOT found!", COLOR_RED);
    }
}
// статический выровненный буфер
static unsigned char sector_buf[512] __attribute__((aligned(512)));

void disk_read_sector(unsigned int lba, void* buf) {
    if (!block_io) return;
    
    EFI_STATUS status = block_io->ReadBlocks(
        block_io, media_id, lba, 512, sector_buf
    );
    
    if (status != EFI_SUCCESS) {
        display_println("disk_read: FAILED!", COLOR_RED);
        return;
    }
    
    // копируем в нужный буфер
    unsigned char* dst = (unsigned char*)buf;
    for (int i = 0; i < 512; i++)
        dst[i] = sector_buf[i];
}