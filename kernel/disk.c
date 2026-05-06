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

    // ← дебаг — выводим все устройства
    for (UINTN i = 0; i < handle_count; i++) {
        EFI_BLOCK_IO* bio = 0;
        sys->BootServices->HandleProtocol(
            handles[i], &bio_guid, (void**)&bio
        );
        if (!bio || !bio->Media) continue;

        char buf[32];
        display_print("dev: LastBlock=", COLOR_GRAY);
        int_to_str((int)bio->Media->LastBlock, buf);
        display_print(buf, COLOR_WHITE);
        display_print(" Logical=", COLOR_GRAY);
        int_to_str(bio->Media->LogicalPartition, buf);
        display_println(buf, COLOR_WHITE);
    }

    for (UINTN i = 0; i < handle_count; i++) {
        EFI_BLOCK_IO* bio = 0;
        sys->BootServices->HandleProtocol(
            handles[i], &bio_guid, (void**)&bio
        );

        if (!bio->Media->MediaPresent) continue;
        if (bio->Media->LogicalPartition) continue;
        if (bio->Media->BlockSize != 512) continue;
        if (bio->Media->LastBlock < 65536) continue;   // меньше 32MB — пропуск
        if (bio->Media->LastBlock > 500000) continue;  // больше 256MB — пропуск (это esp)

        // ← добавь: пропускаем маленькие диски (меньше 32MB)
        if (bio->Media->LastBlock < 65536) continue;

        block_io = bio;
        media_id = bio->Media->MediaId;
        break;
    }


    sys->BootServices->FreePool(handles);

    
    if (block_io) {
        char buf[32];
        display_print("disk: found! MediaId=", COLOR_GREEN);
        int_to_str(media_id, buf);
        display_print(buf, COLOR_WHITE);
        display_print(" LastBlock=", COLOR_GRAY);
        int_to_str((int)block_io->Media->LastBlock, buf);
        display_println(buf, COLOR_WHITE);
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