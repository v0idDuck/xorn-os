#include <xeapi.h>


XeSetting xes;

RDATA titleo[] = "XORN OS V0.2.0 by VoidDuck";
RDATA title[] = "XORN SHELL V0.2.0";
RDATA thelp[] = "type help for list of commands";
RDATA slash[] = "/";
RDATA temprem[] = "temporarily removed due to bugs";
static void draw_prompt();
void parse_and_exec(char* cwd, const char* line);

RDATA s_clear[] = "clear";
RDATA s_echo[]  = "echo";
RDATA s_ver[] = "ver";
RDATA s_help[] = "help";
RDATA s_reboot[] = "reboot";
RDATA s_cd[] = "cd";
RDATA s_ls[] = "ls";
RDATA s_fontsize[] = "fontsize";
RDATA s_mem[] = "mem";
RDATA s_run[] = "run";

ENTRY entry(unsigned long long dummy, XornAPI* api) {
    xorn_init(api);
    char cwd[128];
    cwd[0] = '/'; cwd[1] = 0;
    xes.Fontsize = 3;
    xes.cx = 5;
    xes.cy = 5;
    api->clear_screen(0x000000);
    
    xprintln(xes, titleo, 0xFFFFFF);
    xprintln(xes, title, 0xFFFFFF);
    xprintln(xes, thelp, 0x888888);
    xprintln(xes, titleo, 0x000000);
    while (1) {
        draw_prompt(cwd);
        char buf[256];
        int len = 0;
        while (1) {
            char c = xblink_read(xes.cx, xes.cy);
            if (c == '\r') break;
            if (c == KEY_BACKSPACE && len > 0) {
                len--;
                xes.cx -= 6 * xes.Fontsize;
                rect(xes.cx, xes.cy, 6 * xes.Fontsize, 9 * xes.Fontsize, 0x000000);
                continue;
            }
            if (len < sizeof(buf) - 1 && c >= 32 && c <= 126) {
                buf[len++] = c;
                buf[len] = 0;
                char tmp[2];
                tmp[0] = c;
                tmp[1] = 0;
                xprint(xes, tmp, 0xFFFFFF);
            }
        }
        buf[len] = 0;
        buf[len] = 0;
        xes.cx = 40;
        xes.cy += 9 * xes.Fontsize;
        parse_and_exec(cwd, buf);
        xes.cx = 5;
    }
    
}

static void draw_prompt(char* cwd) {
    RDATA xorn[] = "xorn::";
    RDATA invite[] = "> ";
    xprint(xes, xorn, 0xFFFFFF);
    xprint(xes, cwd, 0xFFFFFF);
    xprint(xes, invite, 0xFFFFFF);
}

void cmd_clear() {
    _api->clear_screen(0x000000);
    xes.cx = 5;
    xes.cy = 5;
    xprintln(xes, titleo, 0xFFFFFF);
    xprintln(xes, title, 0xFFFFFF);
}

void cmd_ver() {
    RDATA v1[] = "  XORN OS v0.2.0";
    RDATA v2[] = "  XORN SHELL v0.2.0";
    RDATA v3[] = "  by VoidDuck";
    xprintln(xes, v1, 0xFFFFFF);
    xprintln(xes, v2, 0xFFFFFF);
    xprintln(xes, v3, 0x888888);
}

void cmd_help() {
    RDATA h1[] = "  help            - list commands";
    RDATA h2[] = "  clear           - clear screen";
    RDATA h3[] = "  ver             - system version";
    RDATA h4[] = "  reboot          - reboot system";
    RDATA h5[] = "  fontsize <1-5>  - change font size";
    RDATA h6[] = "  cd <path>       - change directory";
    RDATA h7[] = "  echo <text>     - print text";
    RDATA h8[] = "  mem             - memory usage";
    RDATA h9[] = "  run <file>      - run .xe file";
    RDATA h10[] = "  ls              - list files";
    xprintln(xes, h1, 0x888888);
    xprintln(xes, h2, 0x888888);
    xprintln(xes, h3, 0x888888);
    xprintln(xes, h4, 0x888888);
    xprintln(xes, h5, 0x888888);
    xprintln(xes, h6, 0x888888);
    xprintln(xes, h7, 0x888888);
    xprintln(xes, h8, 0x888888);
    xprintln(xes, h9, 0x888888);
    xprintln(xes, h10, 0x888888);
}

static void cmd_cd(const char* arg, char* cwd) {
    if (arg[0] == 0 || _str_eq(arg, "/") || _str_eq(arg, "::/")) {
        _str_copy(cwd, "/");
        return;
    }


    
    if (arg[0] == '.' && arg[1] == '.') {
        // ищем последний /
        int i = _str_len(cwd) - 1;
        while (i > 0 && cwd[i] != '/') i--;
        cwd[i == 0 ? 1 : i] = 0;
        return;
    }


    if (arg[0] == '/') {
    _str_copy(cwd, arg);
    return;
}

// относительный путь
int len = _str_len(cwd);
if (cwd[len - 1] != '/') {
    cwd[len] = '/';
    cwd[len + 1] = 0;
    len++;
}
_str_copy(cwd + len, arg);
}

static void cmd_ls(const char* path) {
    XFatEntry* entries = (XFatEntry*)xalloc(sizeof(XFatEntry) * 4);
    int count = _api->ls(path, entries, 4);
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir) {
            xprint(xes, slash, 0x00FFFF);
            xprint(xes, entries[i].name, 0x00FFFF);
            xprintln(xes, slash, 0x00FFFF);
            xes.cx = 40;
        } else {
            int len = _str_len(entries[i].name);
            int is_xe = len > 3 &&
                entries[i].name[len-3] == '.' &&
                entries[i].name[len-2] == 'X' &&
                entries[i].name[len-1] == 'E';
            xprintln(xes, entries[i].name, is_xe ? 0xFFFF00 : 0xFFFFFF);
        }
}
    xes.cx = 5;
}

void cmd_fontsize(const char* arg) {
    if (arg[0] == 0) {
        RDATA usage[] = "  usage: fontsize <1-5>";
        xprintln(xes, usage, 0x888888);
        return;
    }
    int size = arg[0] - '0';
    if (size < 1 || size > 5) {
        RDATA err[] = "  error: size must be 1-5";
        xprintln(xes, err, 0xFF3333);
        return;
    }
    xes.Fontsize = size;
    cmd_clear();
    RDATA fsch[] = "fontsize changed!";
    xprintln(xes, fsch, COLOR_GREEN);
}
void cmd_mem() {
    uint32 used = _api->mem_used();
    uint32 total = _api->mem_total();
    uint32 used_kb = used / 1024;
    uint32 total_kb = total / 1024;
    
    char buf[16];
    RDATA heap[] = "  heap: ";
    RDATA sep[] = " / ";
    RDATA kb[] = " KB";
    
    xprint(xes, heap, 0x888888);
    _int_to_str(used_kb, buf);
    xprint(xes, buf, 0xFFFFFF);
    xprint(xes, sep, 0x888888);
    _int_to_str(total_kb, buf);
    xprint(xes, buf, 0xFFFFFF);
    xprintln(xes, kb, 0x888888);
}
static void cmd_run(char* cwd, const char* arg) {
    RDATA err_nf[] = "  error: file not found";
    RDATA err_bm[] = "  error: not a .xe file";
    RDATA err_nm[] = "  error: out of memory";
    
    char path[256];
    
    if (arg[0] == '/') {
        _str_copy(path, arg);
    } else {
        _str_copy(path, cwd);
        if (path[_str_len(path) - 1] != '/')
            path[_str_len(path)] = '/';
        _str_copy(path + _str_len(path), arg);
    }
    
    int result = _api->run(path);

    if      (result == 1) xprintln(xes, err_nf, 0xFF3333);
    else if (result == 2) xprintln(xes, err_bm, 0xFF3333);
    else if (result == 3) xprintln(xes, err_nm, 0xFF3333);
}

void parse_and_exec(char* cwd, const char* line) {
    char cmd[64];
    char arg[192];
    int i = 0, j = 0;

    
    while (line[i] && line[i] != ' ')
        cmd[j++] = line[i++];
    cmd[j] = 0;

    if (line[i] == ' ') i++;

    j = 0;
    while (line[i]) arg[j++] = line[i++];
    arg[j] = 0;
    if (_str_eq(cmd, s_clear)) cmd_clear();
    else if (_str_eq(cmd, s_echo)) xprintln(xes, arg, 0xFFFFFF);
    else if (_str_eq(cmd, s_ver)) cmd_ver();
    else if (_str_eq(cmd, s_help)) cmd_help();
    else if (_str_eq(cmd, s_reboot)) _api->reboot();
    else if (_str_eq(cmd, s_cd)) cmd_cd(arg, cwd);
    else if (_str_eq(cmd, s_ls)) {
        if (arg[0] != 0) {
            cmd_ls(arg);
        } else {
            cmd_ls(cwd);
        }
    }
    else if (_str_eq(cmd, s_fontsize)) cmd_fontsize(arg);
    else if (_str_eq(cmd, s_mem)) cmd_mem();
    else if (_str_eq(cmd, s_run)) xprintln(xes, temprem, 0xFFFFFF); // cmd_run(cwd, arg);
    else {
        // path(/bin/)
        char path[128];
        RDATA bin[] = "/bin/";
        RDATA xe[] = ".xe";
        _str_copy(path, bin);
        _str_copy(path + _str_len(path), cmd);
        _str_copy(path + _str_len(path), xe);
    
        int result = _api->run(path);
        if (result == 1) {
            RDATA unk[] = "  unknown command";
            xprintln(xes, unk, 0xFF3333);
        }
    }
}