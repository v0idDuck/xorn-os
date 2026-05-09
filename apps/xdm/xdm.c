#include <xeapi.h>


static void draw_app(char* name, int gx, int gy, int selected);
static void draw_icon(int type, int x, int y);
static int get_icon_type(char* short_name);
static void draw_topbar(int selected, int active);

#define ICON_SIZE 64
#define ICON_PAD  35
#define GRID_X    20
#define GRID_Y    35
#define MAX_APPS 16

#define ICON_DEFAULT  0
#define ICON_TERMINAL 1
#define ICON_SNAKE    2
#define ICON_CALC     3

int app_count = 0;

typedef struct {
    const char* name;
    const char* path;
} AppDef;



ENTRY entry(unsigned long long dummy, XornAPI* api) {
    xorn_init(api);
    api->clear_screen(0x000000);

    XFatEntry* entries = (XFatEntry*)xalloc(sizeof(XFatEntry) * MAX_APPS);
    RDATA bin[] = "bin";
    int app_count = _api->ls(bin, entries, MAX_APPS);

    
    int mode = 0; // 0 = иконки, 1 = hud
    int bar_selected = 0; // 0=XDM, 1=EXIT, 2=REBOOT, 3=HALT

    int selected = 0;
    while (1) {
        api->clear_screen(0x000000);
        draw_topbar(bar_selected, mode == 1);
    
        for (int i = 0; i < app_count; i++) {
            if (!entries[i].is_dir) {
                draw_app(entries[i].name, i, 0, mode == 0 && selected == i);
            }
        }
    
        char c = _api->read_key();
    
        if (c == '\t') {
            mode = !mode;
        } else if (mode == 0) {
            if (c == KEY_RIGHT && selected < app_count - 1) selected++;
            if (c == KEY_LEFT  && selected > 0) selected--;
            if (c == KEY_ENTER) { /* запуск */ }
        } else {
            if (c == KEY_RIGHT && bar_selected < 3) bar_selected++;
            if (c == KEY_LEFT  && bar_selected > 0) bar_selected--;
            if (c == KEY_ENTER) {
                if (bar_selected == 1) xexit();
                if (bar_selected == 2) _api->reboot();
                // HALT потом
            }
        }
    }
}

static void draw_app(char* name, int gx, int gy, int selected) {
    char short_name[16];
    int k = 0;
    while (name[k] && name[k] != '.') {
        short_name[k] = name[k];
        k++;
    }
    short_name[k] = 0;
    
    int x = GRID_X + gx * (ICON_SIZE + ICON_PAD);
    int y = GRID_Y + gy * (ICON_SIZE + ICON_PAD);
    uint32 border = selected ? 0x3333FF : 0x444444;
    
    _api->draw_rect(x, y, ICON_SIZE, ICON_SIZE, 0x222222);
    _api->draw_rect(x, y, ICON_SIZE, 2, border);
    _api->draw_rect(x, y+ICON_SIZE-2, ICON_SIZE, 2, border);
    _api->draw_rect(x, y, 2, ICON_SIZE, border);
    _api->draw_rect(x+ICON_SIZE-2, y, 2, ICON_SIZE, border);
    _api->draw_text(x, y + ICON_SIZE + 4, short_name, 0xFFFFFF, 2);
    int type = get_icon_type(short_name);
    draw_icon(type, x, y);
}

static int get_icon_type(char* short_name) {
    RDATA shell[] = "SHELL";
    RDATA snake[] = "SNAKE";
    RDATA calc[] = "CALC";
    if (_str_eq(short_name, shell)) return ICON_TERMINAL;
    if (_str_eq(short_name, snake)) return ICON_SNAKE;
    if (_str_eq(short_name, calc)) return ICON_CALC;
    return ICON_DEFAULT;
}

static void draw_icon(int type, int x, int y) {
    int cx = x + ICON_SIZE/2;
    int cy = y + ICON_SIZE/2;
    
    if (type == ICON_TERMINAL) {
        // рисуем >_
        RDATA t[] = ">_";
        _api->draw_text(cx - 10, cy - 5, t, 0x33FF33, 2);
    } else if (type == ICON_SNAKE) {
        // рисуем змейку — зелёный квадрат
        _api->draw_rect(cx - 10, cy - 10, 20, 20, 0x33FF33);
        _api->draw_rect(cx - 10, cy - 10, 6, 6, 0x00AA00);
    } else if (type == ICON_CALC) {
        RDATA c[] = "2+2";
        _api->draw_text(cx - 10, cy - 5, c, 0x33FF33, 2);
    
    } else {
        // дефолт — просто XE
        RDATA t[] = "XE";
        _api->draw_text(cx - 8, cy - 5, t, 0x888888, 2);
    }
}

static void draw_topbar(int selected, int active) {
    _api->draw_rect(0, 0, 1280, 24, active ? 0x333333 : 0x222222);
    
    RDATA b0[] = " XDM ";
    RDATA b1[] = " EXIT ";
    RDATA b2[] = " REBOOT ";
    RDATA b3[] = " HALT ";
    RDATA b9[] = " tab to switch mode";

    _api->draw_text(10,  4, b0, active && selected==0 ? 0x3333FF : 0xFFFFFF, 2);
    _api->draw_text(80,  4, b1, active && selected==1 ? 0x3333FF : 0xFFFFFF, 2);
    _api->draw_text(150, 4, b2, active && selected==2 ? 0x3333FF : 0xFFFFFF, 2);
    _api->draw_text(260, 4, b3, active && selected==3 ? 0x3333FF : 0xFFFFFF, 2);
    _api->draw_text(1040, 4, b9, 0x888888, 2);
}