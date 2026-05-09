// kernel/xorn.c
#include <efi.h>
#include <efiprot.h>
#include "display.h"
#include "keyboard.h"
#include "colors.h"
#include "shell.h"
#include "memory.h"
#include "utils.h"
#include "xorn-bare.h"
#include "disk.h"
#include "fat32.h"
#include "config.h"
#include "uefi_config.h"
#include "loader.h"

// цвета 
#define BL_WHITE        0x0F
#define BL_GRAY         0x08
#define BL_BLACK        0x00
#define BL_LIGHTGRAY    0x07
// макросы функций
#define printb(text) sys->ConOut->OutputString(sys->ConOut, text) 
#define clearb() sys->ConOut->ClearScreen(sys->ConOut);
#define colorb(attrib) sys->ConOut->SetAttribute(sys->ConOut, attrib)
// b специально для bootloader(безшрифта)
#define clear() sys->ConOut->ClearScreen(sys->ConOut);
#define color(attrib) sys->ConOut->SetAttribute(sys->ConOut, attrib)

int xorn_debug = 0;
int font_scale = 3;
int lsm = 0;
EFI_STATUS efi_main(EFI_HANDLE handle, EFI_SYSTEM_TABLE* sys) {

    
    sys->ConOut->ClearScreen(sys->ConOut);
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    sys->BootServices->LocateProtocol(&gop_guid, NULL, (void**)&gop);
    if (!gop) {
        printb(L"GOP not found!\r\n");
        while (1);
    }
    
    while(1) {
        colorb(BL_WHITE);
        printb(L"##########################   %               -=  \r\n");
        printb(L"#                        #    :@@@         .@@#\r\n");
        printb(L"#      XORN OS v0.1      #        %@@+   #@@.     \r\n");
        printb(L"#      BOOTLOADER        #           --:             \r\n");
        printb(L"#                        #           @@=  \r\n");
        printb(L"#  1.   Start XORN OS    #       +**-   =** \r\n");
        printb(L"#  2.   System Info      #       %@@+   #@@.\r\n");
        printb(L"#  3.   Reboot           #     ###        .##+\r\n");
        printb(L"#  E.   LSM              #    :@@@        .@@#\r\n");
        printb(L"#                        #   %               -= \r\n");
        printb(L"##########################\r\n");

        colorb(BL_GRAY);
        printb(L"LSM = Launch Setup Menu\r\n");
        colorb(BL_LIGHTGRAY);
        
        printb(L"Select an option: ");
        // sys->ConOut->SetCursorPosition(sys->ConOut, 0, 10);
        sys->ConOut->EnableCursor(sys->ConOut, TRUE);
        CHAR16 key = get_key(sys);
        sys->ConOut->EnableCursor(sys->ConOut, FALSE);
        colorb(BL_GRAY);
        clearb();
        if (key == L'2') {
            colorb(BL_WHITE);
            printb(L"\rXORN OS V0.0.2 2026");
            printb(L"\r\nAuthor: VoidDuck\r\nMode: UEFI x64\r\n");
            printb(L"Press any key to boot menu...\r\n");
            sys->ConOut->EnableCursor(sys->ConOut, TRUE);
            wait_for_key(sys);
            sys->ConOut->EnableCursor(sys->ConOut, FALSE);
            clearb();
        } else if (key == L'3') {
            sys->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
        } else if (key == L'E' || key == L'e') {
            clearb();
            colorb(BL_WHITE);
            lsm = 1;
            printb(L"Here is the XORN OS launch setup menu\r\n");
            printb(L"Default: xorn --shell\r\n");
            printb(L"Options:\r\n");
            printb(L"  --shell  - launch full shell(xsh)\r\n");
            printb(L"  --0      - launch minimal shell - Xorn Kernel Shell\r\n");
            printb(L"  --debug  - launch shell with debug info\r\n");
            printb(L"> xorn");
            CHAR16 line[256];
            
            read_line(sys, line, 256);
            char line_str[256];
            wstr_to_str(line, line_str);
            if (str_eq(line_str, " --shell")) {
                clearb();
                break;
            } else if (str_eq(line_str, " --0") || str_eq(line_str, "")) {
            nullmode:
                clearb();
                colorb(BL_WHITE);
                display_init(gop);
                keyboard_init(sys);
                memory_init();
                disk_init(sys);
                fat32_init();
                loader_init(sys);
                clearb();
                bare_start(sys);
            } else if (str_eq(line_str, " --debug")) {
                xorn_debug = 1;
                clearb();
                colorb(BL_WHITE);
                break;
            } else {
                printb(L"\r\nUnknown launch config\r\n");
                printb(L"Press any key to return to menu...\r\n");
                wait_for_key(sys);
                clearb();
            }
        } else {
            break;
        }

    }
    // char start_mode[64] = "--shell";
    // char debug_val[8]   = "0";
    // uefi_config_get(handle, sys, L"etc\\xorn\\boot.cfg", "start_setup", start_mode, sizeof(start_mode));
    // uefi_config_get(handle, sys, L"etc\\xorn\\boot.cfg", "debug",       debug_val,  sizeof(debug_val));
    // xorn_debug = str_eq(start_mode, "--debug");
    // xorn_debug = str_eq(debug_val, "1");
    // if (str_eq(start_mode, "--0")) {
    //     goto nullmode;
    // }
    colorb(BL_WHITE);
    display_init(gop);
    keyboard_init(sys);
    memory_init();
    disk_init(sys);
    fat32_init();
    loader_init(sys);
    XDEBUG(wait_for_key(sys););
    clearb();
    
    cursor_y = 5;

    shell_start(sys);
    
    return EFI_SUCCESS;
}
