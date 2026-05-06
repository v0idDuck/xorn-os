// kernel/display.h
#pragma once
#include <efi.h>
#include <efiprot.h>
#include "xorn.h"

#define FONT_H      (9 * font_scale)

extern int cursor_x;
extern int cursor_y;

void display_init(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop);
void display_clear(UINT32 color);
void display_pixel(int x, int y, UINT32 color);
void display_rect(int x, int y, int w, int h, UINT32 color);
void display_char(int x, int y, char c, UINT32 color, int scale);
void display_text(int x, int y, const char* text, UINT32 color, int scale);
void display_print(const char* text, UINT32 color);    // ← добавь
void display_println(const char* text, UINT32 color);  // ← добавь

#define println(text, color) display_println(text, color)
#define print(text, color)   display_print(text, color)