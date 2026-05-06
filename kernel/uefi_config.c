#include "uefi_config.h"
#include "utils.h"

static void trim_crlf(char* s) {
    int len = str_len(s);
    while (len > 0 && (s[len-1] == '\r' || s[len-1] == '\n' || s[len-1] == ' '))
        s[--len] = 0;
}

int uefi_config_get(EFI_HANDLE handle, EFI_SYSTEM_TABLE* sys,
                    const CHAR16* filepath, const char* key,
                    char* value, int max_len)
{
    EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfsp = NULL;
    sys->BootServices->LocateProtocol(&sfsp_guid, NULL, (void**)&sfsp);
    if (!sfsp) return 0;

    EFI_FILE_PROTOCOL* root = NULL;
    sfsp->OpenVolume(sfsp, &root);
    if (!root) return 0;

    EFI_FILE_PROTOCOL* file = NULL;
    EFI_STATUS s = root->Open(root, &file, (CHAR16*)filepath,
                              EFI_FILE_MODE_READ, 0);
    if (s != EFI_SUCCESS || !file) return 0;

    // читаем весь файл (конфиг маленький, 512 байт хватит)
    UINT8 buf[512];
    UINTN size = sizeof(buf) - 1;
    file->Read(file, &size, buf);
    file->Close(file);
    buf[size] = 0;

    // парсим построчно
    int i = 0;
    while (i < (int)size) {
        // пропускаем комментарии и пустые строки
        if (buf[i] == '#' || buf[i] == '\r' || buf[i] == '\n') {
            while (i < (int)size && buf[i] != '\n') i++;
            i++;
            continue;
        }

        // читаем ключ
        char k[64];
        int j = 0;
        while (i < (int)size && buf[i] != '=' && buf[i] != '\n' && buf[i] != '\r')
            k[j++] = buf[i++];
        k[j] = 0;
        trim_crlf(k);

        if (buf[i] != '=') {
            while (i < (int)size && buf[i] != '\n') i++;
            i++;
            continue;
        }
        i++; // пропускаем '='

        // читаем значение
        char v[128];
        j = 0;
        while (i < (int)size && buf[i] != '\n' && buf[i] != '\r' && j < max_len - 1)
            v[j++] = buf[i++];
        v[j] = 0;
        trim_crlf(v);

        if (str_eq(k, key)) {
            str_copy(value, v);
            return 1;
        }

        while (i < (int)size && buf[i] != '\n') i++;
        i++;
    }
    return 0;
}