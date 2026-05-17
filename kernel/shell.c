#include "shell.h"
#include "utils.h"
#include "loader.h"
#include "xorn.h"
// ─── Состояние шелла ─────────────────────────────────────
static EFI_SYSTEM_TABLE* gSys;
static char input[256];
static int  input_len;
static char cwd[128];

// ─── Вспомогательные функции ─────────────────────────────


// ─── Промпт ──────────────────────────────────────────────

static void draw_prompt() {
    display_print("xorn::", COLOR_WHITE);
    display_print(cwd,    COLOR_WHITE);
    display_print("> ",   COLOR_WHITE);
}

// ─── Команды ─────────────────────────────────────────────

static void cmd_help() {
    println("  help          - list commands",  COLOR_GRAY);
    println("  clear         - clear screen",   COLOR_GRAY);
    println("  ver           - system version", COLOR_GRAY);
    println("  reboot        - reboot system",  COLOR_GRAY);
    println("  fontsize <1-5> - change font size", COLOR_GRAY);
    println("  cd <path>     - change directory", COLOR_GRAY);
    println("  echo <text>   - print text", COLOR_GRAY);
    println("  mem           - show memory usage", COLOR_GRAY);
    println("  run <file>    - run .xe file", COLOR_GRAY);
    println("  ls            - list files in current directory", COLOR_GRAY);
}

static void cmd_clear() {
    display_clear(COLOR_BLACK);
    cursor_x = 0;
    cursor_y = 0;
    println("XORN OS v0.1.0 by VoidDuck", COLOR_WHITE);
    println("XORN SHELL / xsh v0.1.0",         COLOR_WHITE);
    println("type help for commands",      COLOR_GRAY);
}

static void cmd_ver() {
    println("  XORN OS v0.1.0", COLOR_WHITE);
    println("  XORN SHELL v0.1.0", COLOR_WHITE);
    println("  by VoidDuck",    COLOR_GRAY);
}

static void cmd_reboot() {
    gSys->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
}

static void cmd_cd(const char* arg) {
    if (arg[0] == 0 || str_eq(arg, "/") || str_eq(arg, "::/")) {
        str_copy(cwd, "/");
        return;
    }


    // cd .. — поднимаемся выше
    if (arg[0] == '.' && arg[1] == '.') {
        // ищем последний /
        int i = str_len(cwd) - 1;
        while (i > 0 && cwd[i] != '/') i--;
        cwd[i == 0 ? 1 : i] = 0;  // обрезаем
        return;
    }

    // cd /bin — абсолютный путь
    if (arg[0] == '/') {
        str_copy(cwd, arg);
        return;
    }

    // cd bin — относительный путь
    if (cwd[str_len(cwd) - 1] != '/')
        cwd[str_len(cwd)] = '/';
    str_copy(cwd + str_len(cwd), arg);
}

static void cmd_fontsize(const char* arg) {
    if (arg[0] == 0) {
        println("  usage: fontsize <1-5>", COLOR_GRAY);
        return;
    }
    int size = arg[0] - '0';  // конвертируем символ в число
    if (size < 1 || size > 5) {
        println("  error: size must be 1-5", COLOR_RED);
        return;
    }
    font_scale = size;
    cmd_clear();
    println("font size changed!", COLOR_GREEN);
}

static void cmd_echo(const char* arg) {
    if (arg[0] == 0) {
        println("  usage: echo <text>", COLOR_GRAY);
        return;
    }
    println(arg, COLOR_WHITE);
}

static void cmd_mem() {
    char buf[32];
    uint32 used_kb  = memory_used() / 1024;
    uint32 total_kb = 1024;

    display_print("  heap: ", COLOR_GRAY);
    int_to_str(used_kb, buf);
    display_print(buf, COLOR_WHITE);
    display_print(" / ", COLOR_GRAY);
    int_to_str(total_kb, buf);
    display_print(buf, COLOR_WHITE);
    println(" KB", COLOR_GRAY);
}


static void cmd_run(const char* arg) {
    char path[256];
    
    
    if (arg[0] == '/') {
        str_copy(path, arg);
    } else {
        // относительный — добавляем cwd
        str_copy(path, cwd);
        if (path[str_len(path) - 1] != '/')
            path[str_len(path)] = '/';
        str_copy(path + str_len(path), arg);
    }
    
    int result = loader_run(path, 0);

    if      (result == LOADER_ERR_NOTFOUND) println("  error: file not found", COLOR_RED);
    else if (result == LOADER_ERR_BADMAGIC) println("  error: not a .xe file", COLOR_RED);
    else if (result == LOADER_ERR_NOMEM)    println("  error: out of memory",  COLOR_RED);
}

static void cmd_ls(const char* arg) {
    char path[256];
    
    if (arg[0] == 0) {
        // без аргумента — текущая папка
        str_copy(path, cwd);
    } else if (arg[0] == '/') {
        // абсолютный путь
        str_copy(path, arg);
    } else {
        // относительный
        str_copy(path, cwd);
        if (path[str_len(path) - 1] != '/')
            path[str_len(path)] = '/';
        str_copy(path + str_len(path), arg);
    }
    
    FatEntry entries[64];
    int count = fat32_ls(path, entries, 64);
    
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir) {
            display_print("  ", COLOR_GRAY);
            display_print("/", COLOR_CYAN);
            display_print(entries[i].name, COLOR_CYAN);
            println("/", COLOR_CYAN);
        } else {
            int len = str_len(entries[i].name);
            int is_xe = len > 3 && 
                        entries[i].name[len-3] == '.' &&
                        entries[i].name[len-2] == 'X' &&
                        entries[i].name[len-1] == 'E';
        
            display_print("  ", COLOR_GRAY);
            println(entries[i].name, is_xe ? COLOR_GREEN : COLOR_WHITE);
        }
    }
}

static void cmd_test_write() {
    unsigned char data[] = "Hello from FAT32 write!";
    int result = fat32_write("/test.txt", data, 23);
    if (result) println("write ok!", COLOR_GREEN);
    else println("write failed!", COLOR_RED);
}

// ─── Парсер ──────────────────────────────────────────────

static void parse_and_exec(char* line) {
    char cmd[64];
    char arg[192];
    int i = 0, j = 0;

    // читаем команду до пробела
    while (line[i] && line[i] != ' ')
        cmd[j++] = line[i++];
    cmd[j] = 0;

    // пропускаем пробел
    if (line[i] == ' ') i++;

    // остаток — аргумент
    j = 0;
    while (line[i]) arg[j++] = line[i++];
    arg[j] = 0;

    // выполняем
    if      (str_eq(cmd, "help"))   cmd_help();
    else if (str_eq(cmd, "clear"))  cmd_clear();
    else if (str_eq(cmd, "ver"))    cmd_ver();
    else if (str_eq(cmd, "reboot")) cmd_reboot();
    else if (str_eq(cmd, "fontsize")) cmd_fontsize(arg);
    else if (str_eq(cmd, "cd"))      cmd_cd(arg);
    else if (str_eq(cmd, "echo"))    cmd_echo(arg);
    else if (str_eq(cmd, "mem"))     cmd_mem();
    else if (str_eq(cmd, "run")) cmd_run(arg);
    else if (str_eq(cmd, "ls")) cmd_ls(arg);
    else if (str_eq(cmd, "test_write")) cmd_test_write();
    else {

        char path[128];
        path[0] = '/'; path[1] = 'b'; path[2] = 'i'; 
        path[3] = 'n'; path[4] = '/'; path[5] = 0;
        str_copy(path + 5, cmd);

        int len = str_len(path);
        path[len] = '.'; path[len+1] = 'x'; 
        path[len+2] = 'e'; path[len+3] = 0;
    
        int result = loader_run(path, 0);
        if (result == LOADER_ERR_NOTFOUND) {
            display_print("  unknown: ", COLOR_RED);
            println(cmd, COLOR_GRAY);
        }
    }
}

// ─── Главный цикл ────────────────────────────────────────

void shell_start(EFI_SYSTEM_TABLE* sys) {
    gSys = sys;
    str_copy(cwd, "/");
    input_len = 0;

    // шапка
    display_clear(COLOR_BLACK);
    println("XORN OS v0.1.0 by VoidDuck", COLOR_WHITE);
    println("XORN SHELL v0.1.0",         COLOR_WHITE);
    println("type help for commands",      COLOR_GRAY);
  

    println("", COLOR_BLACK);

    draw_prompt();

    while (1) {
        // мигающий курсор и ввод
        char key = blink_read(sys, cursor_x, cursor_y);

        if (key == '\r' || key == '\n') {
            // Enter — выполняем команду
            input[input_len] = 0;
            println("", COLOR_BLACK);  // перенос строки

            if (input_len > 0)
                parse_and_exec(input);

            // сброс буфера
            input_len = 0;
            input[0]  = 0;
            draw_prompt();

        } else if (key == CHAR_BACKSPACE) {
            // Backspace — удаляем символ
            if (input_len > 0) {
                input_len--;
                input[input_len] = 0;
                cursor_x -= 6 * font_scale;
                display_rect(cursor_x, cursor_y,
                             6 * font_scale, FONT_H, COLOR_BLACK);
            }

        } else if (key >= 0x20 && key < 0x7F) {
            // обычный символ
            if (input_len < 255) {
                input[input_len++] = key;
                input[input_len]   = 0;
                char buf[2] = {key, 0};
                display_print(buf, COLOR_WHITE);
            }
        }
    }
}