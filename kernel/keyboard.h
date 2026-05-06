#pragma once
#include <efi.h>
#include <eficon.h>

// специальные клавиши (scan codes)
#define KEY_UP      0x01
#define KEY_DOWN    0x02
#define KEY_RIGHT   0x03
#define KEY_LEFT    0x04
#define KEY_ESC     0x17
#define KEY_BACKSPACE 0x08

char blink_read(EFI_SYSTEM_TABLE* sys, int x, int y);
void keyboard_init(EFI_SYSTEM_TABLE* sys);
char keyboard_read(void);      // ждёт нажатия, возвращает символ
int  keyboard_poll(void);      // 0 если ничего не нажато
int k_readline(char* buf, int max_len, EFI_SYSTEM_TABLE* sys, int x, int y);