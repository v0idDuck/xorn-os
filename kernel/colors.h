// kernel/colors.h
#pragma once
#include "xorn.h"

#define COLOR_WHITE     0xFFFFFF
#define cwh     0xFFFFFF
#define COLOR_BLACK     0x000000
#define COLOR_GRAY      0x888888
#define COLOR_DARKGRAY  0x444444
#define COLOR_RED       0xFF3333
#define COLOR_GREEN     0x33FF33
#define COLOR_BLUE      0x3333FF
#define COLOR_YELLOW    0xFFFF00
#define COLOR_CYAN      0x00FFFF

#define XDEBUG(expr) if (xorn_debug) { expr; }