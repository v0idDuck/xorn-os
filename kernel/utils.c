#include "efi.h"

int str_len(const char* s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

int str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

void str_copy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = 0;
}

void int_to_str(int n, char* buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }

    int i = 0;
    char tmp[32];

    while (n > 0) {
        tmp[i++] = '0' + (n % 10);
        n /= 10;
    }

    // разворачиваем строку
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = 0;
}

// Функция ожидания нажатия любой клавиши
void wait_for_key(EFI_SYSTEM_TABLE *SystemTable) {
    EFI_INPUT_KEY Key;
    // 1. Сначала очищаем буфер (на случай, если ты уже что-то нажал)
    SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);

    // 2. Ждем нажатия
    while (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key) == EFI_NOT_READY) {
        // Просто крутимся в цикле, пока не вернут "SUCCESS"
    }
}
CHAR16 get_key(EFI_SYSTEM_TABLE *SystemTable) {
    EFI_INPUT_KEY Key;
    SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);
    while (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key) == EFI_NOT_READY);
    return Key.UnicodeChar;
}
void read_line(EFI_SYSTEM_TABLE* sys, CHAR16* buf, int max_len) {
    int i = 0;
    EFI_INPUT_KEY key;

    while (1) {
        // ждём нажатия
        while (sys->ConIn->ReadKeyStroke(sys->ConIn, &key) != EFI_SUCCESS);

        if (key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            // Enter — заканчиваем
            buf[i] = 0;
            break;

        } else if (key.UnicodeChar == CHAR_BACKSPACE) {
            // Backspace — удаляем символ
            if (i > 0) {
                i--;
                buf[i] = 0;
                // стираем символ на экране
                sys->ConOut->OutputString(sys->ConOut, L"\b \b");
            }

        } else if (key.UnicodeChar >= 0x20 && i < max_len - 1) {
            // обычный символ
            buf[i++] = key.UnicodeChar;
            // выводим на экран
            CHAR16 tmp[2] = {key.UnicodeChar, 0};
            sys->ConOut->OutputString(sys->ConOut, tmp);
        }
    }
}

void wstr_to_str(CHAR16* src, char* dst) {
    while (*src) *dst++ = (char)*src++;
    *dst = 0;
}
// разбивает строку на слова
// "display_text 100 100 HELLO" → args[0]="display_text" args[1]="100" ...
// возвращает количество аргументов
int parse_args(char* line, char args[8][64]) {
    int argc = 0;
    int i = 0;

    while (*line && argc < 8) {
        // пропускаем пробелы
        while (*line == ' ') line++;
        if (!*line) break;

        // читаем слово
        i = 0;
        while (*line && *line != ' ') {
            args[argc][i++] = *line++;
        }
        args[argc][i] = 0;
        argc++;
    }
    return argc;
}

// конвертирует строку в int
// поддерживает hex (0xFF) и decimal
int str_to_int(const char* s) {
    // hex
    if (s[0] == '0' && s[1] == 'x') {
        int result = 0;
        s += 2;
        while (*s) {
            result *= 16;
            if (*s >= '0' && *s <= '9') result += *s - '0';
            else if (*s >= 'a' && *s <= 'f') result += *s - 'a' + 10;
            else if (*s >= 'A' && *s <= 'F') result += *s - 'A' + 10;
            s++;
        }
        return result;
    }
    // decimal
    int result = 0;
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s++ - '0');
    }
    return result;
}

// ─── Функции для работы с памятью ────────────────────────
// Копирует size байт из src в dst
void* memcpy(void* dst, const void* src, unsigned long size) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (size--) *d++ = *s++;
    return dst;
}

// Заполняет память байтом value
void* memset(void* ptr, int value, unsigned long size) {
    unsigned char* p = (unsigned char*)ptr;
    unsigned char v = (unsigned char)value;
    while (size--) *p++ = v;
    return ptr;
}