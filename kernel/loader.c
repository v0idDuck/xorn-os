#include "loader.h"
#include "memory.h"
#include "display.h"
#include "colors.h"
#include "fat32.h"
#include "keyboard.h"
#include "../sdk/xeapi.h"
#include "colors.h"

#define XE_LOAD_ADDR 0x1000000

static EFI_SYSTEM_TABLE* gSys;

void loader_init(EFI_SYSTEM_TABLE* sys) {
    gSys = sys;
}

static char __attribute__((sysv_abi)) _xe_read_key(void) { return keyboard_read(); }
static int  __attribute__((sysv_abi)) _xe_key_pressed(void) { return keyboard_poll(); }
static void __attribute__((sysv_abi)) _xe_clear_screen(uint32_t c) { display_clear(c); }
static void __attribute__((sysv_abi)) _xe_draw_rect(int x, int y, int w, int h, uint32_t c) { display_rect(x,y,w,h,c); }
static void __attribute__((sysv_abi)) _xe_draw_text(int x, int y, const char* t, uint32_t c, int s) { display_text(x,y,t,c,s); }
static void __attribute__((sysv_abi)) _xe_draw_pixel(int x, int y, uint32_t c) { display_pixel(x,y,c); }
static void __attribute__((sysv_abi)) _xe_reboot(void) {
    gSys->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
}
static int __attribute__((sysv_abi)) _xe_ls(const char* path, void* entries, int max) {
    if (!path) return 0;
    return fat32_ls(path, (FatEntry*)entries, max);
}
static uint32_t __attribute__((sysv_abi)) _xe_mem_used(void) { return memory_used(); }
static uint32_t __attribute__((sysv_abi)) _xe_mem_total(void) { return 1024 * 1024; }
static int __attribute__((sysv_abi)) _xe_run(const char* path) {
    return loader_run(path, 0);
}
static char __attribute__((sysv_abi)) _xe_blink_read(int x, int y) {
    return blink_read(gSys, x, y);
}

int loader_run(const char* path, void* api) {
    
    XornAPI api_struct;
    api_struct.clear_screen = _xe_clear_screen;
    api_struct.draw_rect    = _xe_draw_rect;
    api_struct.draw_text    = _xe_draw_text;
    api_struct.draw_pixel   = _xe_draw_pixel;
    api_struct.read_key     = _xe_read_key;
    api_struct.key_pressed  = _xe_key_pressed;
    api_struct.alloc        = memory_alloc;
    api_struct.free         = memory_free;
    api_struct.sleep        = NULL;
    api_struct.exit         = NULL;
    api_struct.reboot = _xe_reboot;
    api_struct.ls = _xe_ls;
    api_struct.mem_used  = _xe_mem_used;
    api_struct.mem_total = _xe_mem_total;
    api_struct.run = _xe_run;
    api_struct.blink_read = _xe_blink_read;
    
    unsigned char* file_buf  = 0;
    unsigned int   file_size = 0;

    if (!fat32_read(path, &file_buf, &file_size)) {
        return LOADER_ERR_NOTFOUND;
    }

    if (file_buf[0] != 'V' || file_buf[1] != 'D') {
        return LOADER_ERR_BADMAGIC;
    }

    unsigned int code_size = file_buf[4] |
                            ((unsigned int)file_buf[5] << 8) |
                            ((unsigned int)file_buf[6] << 16) |
                            ((unsigned int)file_buf[7] << 24);

    unsigned int entry_offset = file_buf[8] |
                               ((unsigned int)file_buf[9] << 8) |
                               ((unsigned int)file_buf[10] << 16) |
                               ((unsigned int)file_buf[11] << 24);

    if (code_size == 0) return LOADER_ERR_BADMAGIC;
    
    
    // загружаем по фиксированному адресу
    unsigned char* code = (unsigned char*)XE_LOAD_ADDR;
    for (unsigned int i = 0; i < code_size; i++)
        code[i] = file_buf[12 + i];

    // передаём массив функций
    char dbuf[32];
    XDEBUG(display_print("display_clear addr=", COLOR_GRAY));
    XDEBUG(int_to_str((int)&display_clear, dbuf));
    XDEBUG(display_println(dbuf, COLOR_WHITE));
    XDEBUG(display_print("code addr=", COLOR_GRAY));
    XDEBUG(int_to_str((int)code, dbuf));
    XDEBUG(display_println(dbuf, COLOR_WHITE));
    char dbufd[32];
    XDEBUG(display_print("clear_addr=", COLOR_GRAY));
    XDEBUG(int_to_str((int)&display_clear, dbufd));
    XDEBUG(display_println(dbufd, COLOR_WHITE));
    typedef void (*XeEntry)(unsigned long long, XornAPI*);
    XeEntry entry_fn = (XeEntry)(code + entry_offset);
    XDEBUG(keyboard_read();); 



entry_fn(0, &api_struct);

    display_println("returned!", COLOR_GREEN);
    return LOADER_OK;
}