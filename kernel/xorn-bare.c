// kernel/xorn-bare.c

#include "xorn-bare.h"

static void draw_prompt() {
    display_print("xorn:/>", COLOR_WHITE);
}

void parse_and_exec(char* line);
static EFI_SYSTEM_TABLE* gSys;
static char input[256];
static int input_len;
void bare_start(EFI_SYSTEM_TABLE* sys) {
    gSys = sys;
    display_clear(COLOR_BLACK);
    println("XORN OS v0.1.0", COLOR_WHITE);
    println("", COLOR_BLACK);
    println("commands:", COLOR_GRAY);
    println("  shell   - start xsh", COLOR_GRAY);
    println("  reboot  - reboot", COLOR_GRAY);
    println("  info    - system info", COLOR_GRAY);
    println("  help    - list of all commands", COLOR_GRAY);
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

void parse_and_exec(char* line) {
    char args[8][64];
    int argc = parse_args(line, args);
    if (argc == 0) return;

    // встроенные команды
    if (str_eq(args[0], "shell")) {
        cursor_x = 0;
        cursor_y = 0;
        display_clear(COLOR_BLACK);
        shell_start(gSys);

    } else if (str_eq(args[0], "reboot")) {
        gSys->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

    } else if (str_eq(args[0], "info")) {
        println("XORN OS v0.1.0", COLOR_WHITE);
        println("XORN KERNEL SHELL", COLOR_GRAY);

    // ─── прямые вызовы функций ───────────────────────────

    } else if (str_eq(args[0], "display_text")) {
        // display_text x y text color
        if (argc < 5) { println("  usage: display_text x y text color", COLOR_GRAY); return; }
        int x     = str_to_int(args[1]);
        int y     = str_to_int(args[2]);
        int color = str_to_int(args[4]);
        display_text(x, y, args[3], color, font_scale);

    } else if (str_eq(args[0], "display_rect")) {
        // display_rect x y w h color
        if (argc < 6) { println("  usage: display_rect x y w h color", COLOR_GRAY); return; }
        int x     = str_to_int(args[1]);
        int y     = str_to_int(args[2]);
        int w     = str_to_int(args[3]);
        int h     = str_to_int(args[4]);
        int color = str_to_int(args[5]);
        display_rect(x, y, w, h, color);

    } else if (str_eq(args[0], "display_clear")) {
        // display_clear color
        int color = argc > 1 ? str_to_int(args[1]) : 0x000000;
        display_clear(color);
        cursor_x = 0;
        cursor_y = 0;

    } else if (str_eq(args[0], "memory_used")) {
        char buf[32];
        int_to_str(memory_used() / 1024, buf);
        display_print("  ", COLOR_GRAY);
        display_print(buf, COLOR_WHITE);
        println(" KB used", COLOR_GRAY);
    } else if (str_eq(args[0], "memory_alloc")) {
        // memory_alloc size
        if (argc < 2) { println("  usage: memory_alloc size", COLOR_GRAY); return; }
        int size = str_to_int(args[1]);
        void* ptr = memory_alloc(size);
        char buf[32];
        int_to_str((int)(uintptr_t)ptr, buf);
        display_print("  allocated at ", COLOR_GRAY);
        display_print(buf, COLOR_WHITE);
        println("", COLOR_BLACK);
    } else if (str_eq(args[0], "memory_free")) {
        // memory_free addr
        if (argc < 2) { println("  usage: memory_free addr", COLOR_GRAY); return; }
        int addr = str_to_int(args[1]);
        memory_free((void*)(uintptr_t)addr);
        display_print("  freed ", COLOR_GRAY);
        char buf[32];
        int_to_str(addr, buf);
        display_print(buf, COLOR_WHITE);
        println("", COLOR_BLACK);
    } else if (str_eq(args[0], "help")) {
        println("commands:", COLOR_GRAY);
        println("  shell   - start xsh", COLOR_GRAY);
        println("  reboot  - reboot", COLOR_GRAY);
        println("  info    - system info", COLOR_GRAY);
        println("  help    - list of all commands", COLOR_GRAY);
        println("  display_text x y text color - display text at (x,y) with color", COLOR_GRAY);
        println("  display_rect x y w h color - display rectangle at (x,y) with size (w,h) and color", COLOR_GRAY);
        println("  display_clear color - clear screen with color", COLOR_GRAY);
        println("  memory_used - show used memory in KB", COLOR_GRAY);
        println("  memory_alloc size - allocate memory and show address", COLOR_GRAY);
        println("  memory_free addr - free memory at address", COLOR_GRAY);
        println("", COLOR_BLACK);
    } else {
        display_print("  unknown: ", COLOR_RED);
        println(args[0], COLOR_GRAY);
    }
}