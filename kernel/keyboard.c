#include "keyboard.h"
#include <eficon.h>
#include "colors.h"
#include "display.h"

static SIMPLE_INPUT_INTERFACE* con_in;

void keyboard_init(EFI_SYSTEM_TABLE* sys) {
    con_in = sys->ConIn;
}

char keyboard_read(void) {
    EFI_INPUT_KEY key;

    // сбрасываем буфер
    con_in->Reset(con_in, FALSE);

    // ждём нажатия
    while (con_in->ReadKeyStroke(con_in, &key) != EFI_SUCCESS);

    // специальные клавиши — scan code
    if (key.ScanCode != 0) return (char)key.ScanCode;

    // обычный символ
    return (char)key.UnicodeChar;
}

int keyboard_poll(void) {
    EFI_INPUT_KEY key;
    if (con_in->ReadKeyStroke(con_in, &key) == EFI_SUCCESS) {
        if (key.ScanCode != 0) return key.ScanCode;
        return key.UnicodeChar;
    }
    return 0;
}
char blink_read(EFI_SYSTEM_TABLE* sys, int x, int y) {
    int visible = 1;
    while (1) {
        // рисуем или стираем курсор
        if (visible) {
            display_rect(x, y, 2, 14, COLOR_WHITE);
        } else {
            display_rect(x, y, 2, 14, COLOR_BLACK);
        }
        visible = !visible;

        // ждём 500ms маленькими кусочками
        // и проверяем клавишу каждые 50ms
        for (int i = 0; i < 10; i++) {
            sys->BootServices->Stall(50000);  // 50ms
            int key = keyboard_poll();
            if (key != 0) {
                // стираем курсор перед выходом
                display_rect(x, y, 2, 14, COLOR_BLACK);
                return (char)key;
            }
        }
    }
}

int k_readline(char* buf, int max_len, EFI_SYSTEM_TABLE* sys, int x, int y) {
    int len = 0;
    buf[0] = 0;
    int cx = x;

    while (1) {
        char key = blink_read(sys, cx, y);

        if (key == '\r' || key == '\n') {
            buf[len] = 0;
            return len;

        } else if (key == '\b' || key == 0x08) {
            if (len > 0) {
                len--;
                buf[len] = 0;
                cx -= 6 * font_scale;
                display_rect(cx, y, 6 * font_scale, FONT_H, COLOR_BLACK);
            }

        } else if (key >= 0x20 && key < 0x7F) {
            if (len < max_len - 1) {
                buf[len++] = key;
                buf[len] = 0;
                char tmp[2] = {key, 0};
                display_print(tmp, COLOR_WHITE);
                cx += 6 * font_scale;
            }
        }
    }
}