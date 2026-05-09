#include <xeapi.h>


static void draw_app(char* name, int gx, int gy, int selected);

#define ICON_SIZE 64
#define ICON_PAD  35
#define GRID_X    20
#define GRID_Y    20
#define MAX_APPS 16

int app_count = 0;

typedef struct {
    const char* name;
    const char* path;
} AppDef;

RDATA n_terminal[] = "Terminal";
RDATA p_terminal[] = "/bin/shell.xe";
RDATA n_snake[]    = "Snake";
RDATA p_snake[]    = "/bin/snake.xe";

ENTRY entry(unsigned long long dummy, XornAPI* api) {
    xorn_init(api);
    api->clear_screen(0x000000);

    XFatEntry* entries = (XFatEntry*)xalloc(sizeof(XFatEntry) * MAX_APPS);
    RDATA bin[] = "bin";
    int app_count = _api->ls(bin, entries, MAX_APPS);

    
    
    int selected = 0;
    while (1) {
        api->clear_screen(0x000000);
    
        for (int i = 0; i < app_count; i++) {
            if (!entries[i].is_dir) {
                draw_app(entries[i].name, i, 0, selected == i);
            }
        }
    
        char c = _api->read_key();
        if (c == KEY_RIGHT && selected < app_count - 1) selected++;
        if (c == KEY_LEFT  && selected > 0) selected--;
        if (c == KEY_ENTER) {
        char path[32];
        int pi = 0;
        RDATA prefix[] = "/bin/";
        
        int pi2 = 0;
        while (prefix[pi2]) path[pi++] = prefix[pi2++];
        
        int ni = 0;
        while (entries[selected].name[ni]) path[pi++] = entries[selected].name[ni++];
        path[pi] = 0;
        _api->run(path);
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
}