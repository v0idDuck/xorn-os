#include "config.h"
#include "fat32.h"
#include "utils.h"
#include "colors.h"
#include "display.h"

int config_get(const char* filepath, const char* key, char* value, int max_len) {
    unsigned char* buf = 0;
    unsigned int size = 0;
    
    if (!fat32_read(filepath, &buf, &size)) return 0;
    
    int i = 0;
    while (i < (int)size) {
        // пропускаем комментарии
        if (buf[i] == '#') {
            while (i < (int)size && buf[i] != '\n') i++;
            continue;
        }
        
        // читаем ключ
        char k[64];
        int j = 0;
        while (i < (int)size && buf[i] != '=' && buf[i] != '\n')
            k[j++] = buf[i++];
        k[j] = 0;
        display_print("key=[", COLOR_WHITE);
        display_print(k, COLOR_WHITE); 
        display_println("]", COLOR_WHITE);
        
        if (buf[i] == '=') {
            i++;
            // читаем значение
            char v[128];
            j = 0;
            while (i < (int)size && buf[i] != '\n' && buf[i] != '\r')
                v[j++] = buf[i++];
            v[j] = 0;
            
            display_print("val=[", COLOR_GRAY);
        display_print(v, COLOR_WHITE); 
        display_println("]", COLOR_GRAY);
            if (str_eq(k, key)) {
                str_copy(value, v);
                return 1;
            }
            
        }
        
        
        while (i < (int)size && buf[i] != '\n') i++;
        i++;
    }
    return 0;
}