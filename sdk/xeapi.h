// sdk/xeapi.h
// Xorn OS SDK v0.1.0
// XornAPI / XeAPI - средство для создания приложений для Xorn OS
// by v0idDuck
#pragma once

// ─── Типы ───────────────────────────────────────────────
typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;

// ─── Цвета ──────────────────────────────────────────────
#define COLOR_WHITE     0xFFFFFF
#define COLOR_BLACK     0x000000
#define COLOR_GRAY      0x888888
#define COLOR_DARKGRAY  0x444444
#define COLOR_RED       0xFF3333
#define COLOR_GREEN     0x33FF33
#define COLOR_BLUE      0x3333FF
#define COLOR_YELLOW    0xFFFF00
#define COLOR_CYAN      0x00FFFF

// ─── Клавиши ────────────────────────────────────────────
#define KEY_UP        0x01
#define KEY_DOWN      0x02
#define KEY_RIGHT     0x03
#define KEY_LEFT      0x04
#define KEY_ESC       0x17
#define KEY_ENTER     0x0D
#define KEY_BACKSPACE 0x08

// ─── Таблица функций ядра ────────────────────────────────
// Ядро предоставляет sysv_abi-обёртки, поэтому XE-программы вызывают
// функции через обычные указатели (System V по умолчанию для Linux-таргета).
typedef struct {
    // графика
    void (*clear_screen)(uint32 color);
    void (*draw_rect)(int x, int y, int w, int h, uint32 color);
    void (*draw_text)(int x, int y, const char* text, uint32 color, int scale);
    void (*draw_pixel)(int x, int y, uint32 color);

    // ввод
    char (*read_key)(void);
    int  (*key_pressed)(void);
    char (*blink_read)(int x, int y);

    // память
    void* (*alloc)(uint32 size);
    void  (*free)(void* ptr);
    uint32 (*mem_used)(void);
    uint32 (*mem_total)(void);

    // система
    void (*sleep)(uint32 ms);
    void (*exit)(void);
    void (*reboot)(void);
    void (*halt)(void);
    
    // fat32
    int (*ls)(const char* path, void* entries, int max);
    int (*write)(const char* path, void* buf, unsigned int size);

    // прилоджения
    int (*run)(const char* path);
} XornAPI;

// ─── Глобальное состояние SDK ────────────────────────────
XornAPI* _api  = 0;
int      _cx   = 40;
int      _cy   = 40;
int      _scale = 2;

#define ENTRY __attribute__((ms_abi)) void
#define sdk_ver "0.1.0"

// ─── Вспомогательные функции ────────────────────────────

static int _str_len(const char* s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

static void _int_to_str(int n, char* buf) {
    if (n == 0) { buf[0]='0'; buf[1]=0; return; }
    int i = 0;
    char tmp[32];
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = 0;
}

static int _str_eq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

static void _str_copy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = 0;
}

#define RDATA static const __attribute__((section(".text"))) char

typedef struct {
    int Fontsize;
    int cx;
    int cy;
} XeSetting;


#define XE_STR(buf, ...) \
    { char _tmp[] = {__VA_ARGS__, 0}; \
      for(int _i=0; _tmp[_i]; _i++) buf[_i]=_tmp[_i]; \
      buf[sizeof(_tmp)-1]=0; }


// ─── Инициализация ───────────────────────────────────────
#define xorn_init(a)    _api = (a); _cx = 40; _cy = 40

// ─── Графика ────────────────────────────────────────────
#define clear()         _api->clear_screen(COLOR_BLACK); _cx = 40; _cy = 40
#define clear_color(c)  _api->clear_screen(c); _cx = 40; _cy = 40
#define pixel(x,y,c)    _api->draw_pixel(x, y, c)
#define rect(x,y,w,h,c) _api->draw_rect(x, y, w, h, c)

// ─── Текст ───────────────────────────────────────────────

#define xprintln(xes, text, color) \
    do { \
        int old_cx = (xes).cx; \
        _api->draw_text((xes).cx, (xes).cy, text, color, (xes).Fontsize); \
        (xes).cx += _str_len(text) * 6 * (xes).Fontsize; \
        (xes).cy += 9 * (xes).Fontsize; \
        (xes).cx = old_cx; \
    } while (0)

#define xprint(xes, text, color) \
    do { \
        _api->draw_text((xes).cx, (xes).cy, text, color, (xes).Fontsize); \
        (xes).cx += _str_len(text) * 6 * (xes).Fontsize; \
    } while (0)


#define rprint(xes, text, color) \
    do { \
        RDATA _rt[] = text; \
        xprintln(xes, _rt, color); \
    } while(0)

#define rprintc(xes, text, color) \
    do { \
        RDATA _rt[] = text; \
        xprint(xes, _rt, color); \
    } while(0)

#define set_scale(s)    _scale = (s)
#define set_cursor(x,y) _cx = (x); _cy = (y)
#define newline()       _cx = 40; _cy += 9 * _scale

// ─── Ввод ────────────────────────────────────────────────
#define xread_key()      _api->read_key()
#define xkey_pressed()   _api->key_pressed()
#define xblink_read(x, y) _api->blink_read(x, y)

// ─── Память ──────────────────────────────────────────────
#define xalloc(size)    _api->alloc(size)
#define xfree(ptr)      _api->free(ptr)

// ─── Система ─────────────────────────────────────────────
#define xsleep(ms)      _api->sleep(ms)
#define xexit()         _api->exit()

// ─── C++ поддержка ───────────────────────────────────────
#ifdef __cplusplus
    #define XORN_ENTRY extern "C"
#else
    #define XORN_ENTRY
#endif

// ---- fat32 ------------------------------------
typedef struct {
    char name[16];  // 13 + 3 padding
    int  is_dir;
    unsigned int size;
} XFatEntry;
