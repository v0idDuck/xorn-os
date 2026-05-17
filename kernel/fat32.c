#include "fat32.h"
#include "disk.h"
#include "memory.h"
#include "utils.h"

typedef struct __attribute__((packed)) {
    unsigned char  jump[3];
    unsigned char  oem[8];
    unsigned short bytes_per_sector;
    unsigned char  sectors_per_clus;
    unsigned short reserved_sectors;
    unsigned char  fat_count;
    unsigned short root_entries;
    unsigned short total_sectors_16;
    unsigned char  media_type;
    unsigned short fat_size_16;
    unsigned short sectors_per_track;
    unsigned short head_count;
    unsigned int   hidden_sectors;
    unsigned int   total_sectors_32;
    unsigned int   fat_size_32;
    unsigned short flags;
    unsigned short version;
    unsigned int   root_cluster;
    unsigned short fs_info_sector;
    unsigned short backup_boot;
    unsigned char  reserved[12];
    unsigned char  drive_number;
    unsigned char  reserved2;
    unsigned char  boot_sig;
    unsigned int   volume_id;
    unsigned char  volume_label[11];
    unsigned char  fs_type[8];
} Fat32BootSector;

typedef struct __attribute__((packed)) {
    unsigned char  name[8];
    unsigned char  ext[3];
    unsigned char  attributes;
    unsigned char  reserved;
    unsigned char  create_time_ms;
    unsigned short create_time;
    unsigned short create_date;
    unsigned short access_date;
    unsigned short cluster_high;
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned short cluster_low;
    unsigned int   file_size;
} Fat32DirEntry;

static Fat32BootSector boot;
static unsigned int fat_start;
static unsigned int data_start;
static unsigned int root_cluster;
static int          initialized = 0;

static unsigned int cluster_to_sector(unsigned int cluster) {
    return data_start + (cluster - 2) * boot.sectors_per_clus;
}

static unsigned int fat_next(unsigned int cluster) {
    unsigned char sector_buf[512];
    unsigned int fat_offset = cluster * 4;
    unsigned int fat_sector = fat_start + fat_offset / 512;
    unsigned int fat_index  = fat_offset % 512;
    disk_read_sector(fat_sector, sector_buf);
    unsigned int next = *(unsigned int*)(sector_buf + fat_index);
    return next & 0x0FFFFFFF;
}

static int fat_is_end(unsigned int cluster) {
    return cluster >= 0x0FFFFFF8;
}

static int fat_name_match(Fat32DirEntry* entry, const char* name) {
    char fat_name[13];
    int i = 0, j = 0;

    while (i < 8 && entry->name[i] != ' ')
        fat_name[j++] = entry->name[i++];

    if (entry->ext[0] != ' ') {
        fat_name[j++] = '.';
        i = 0;
        while (i < 3 && entry->ext[i] != ' ')
            fat_name[j++] = entry->ext[i++];
    }
    fat_name[j] = 0;

    i = 0;
    while (fat_name[i] && name[i]) {
        char a = fat_name[i];
        char b = name[i];
        if (b >= 'a' && b <= 'z') b -= 32;
        if (a != b) return 0;
        i++;
    }
    return fat_name[i] == 0 && name[i] == 0;
}

void fat32_init(void) {
    unsigned char raw[512];
    disk_read_sector(0, raw);

    unsigned short bps = raw[11] | ((unsigned short)raw[12] << 8);
    if (bps != 512) {
        initialized = 0;
        return;
    }

    fat_start  = raw[14] | ((unsigned int)raw[15] << 8);
    unsigned char fat_count = raw[16];
    unsigned int fat_size = raw[36] | ((unsigned int)raw[37] << 8) |
                            ((unsigned int)raw[38] << 16) | ((unsigned int)raw[39] << 24);
    unsigned char spc = raw[13];

    data_start   = fat_start + fat_count * fat_size;
    root_cluster = raw[44] | ((unsigned int)raw[45] << 8) |
                   ((unsigned int)raw[46] << 16) | ((unsigned int)raw[47] << 24);

    boot.sectors_per_clus = spc;
    initialized = 1;
}

static unsigned int read_chain(unsigned int start_cluster,
                                unsigned char* buf) {
    unsigned int cluster  = start_cluster;
    unsigned int written  = 0;
    unsigned int sec_size = 512;

    while (!fat_is_end(cluster)) {
        unsigned int sector = cluster_to_sector(cluster);
        for (int i = 0; i < boot.sectors_per_clus; i++) {
            disk_read_sector(sector + i, buf + written);
            written += sec_size;
        }
        cluster = fat_next(cluster);
    }
    return written;
}

static unsigned int find_in_dir(unsigned int dir_cluster,
                                 const char* name) {
    unsigned int sec_size  = 512;
    unsigned int clus_size = boot.sectors_per_clus * sec_size;
    unsigned char* buf     = memory_alloc(clus_size);
    unsigned int cluster   = dir_cluster;

    while (!fat_is_end(cluster)) {
        unsigned int sector = cluster_to_sector(cluster);
        for (int i = 0; i < boot.sectors_per_clus; i++)
            disk_read_sector(sector + i, buf + i * sec_size);

        int entries = clus_size / 32;
        for (int i = 0; i < entries; i++) {
            Fat32DirEntry* entry = (Fat32DirEntry*)(buf + i * 32);
            if (entry->name[0] == 0x00) return 0;
            if (entry->name[0] == 0xE5) continue;
            if (entry->attributes == 0x0F) continue;

            if (fat_name_match(entry, name)) {
                unsigned int cluster_num =
                    ((unsigned int)entry->cluster_high << 16)
                    | entry->cluster_low;
                return cluster_num;
            }
        }
        cluster = fat_next(cluster);
    }
    return 0;
}

int fat32_read(const char* path, unsigned char** buf,
               unsigned int* size) {
    if (!initialized) return 0;

    unsigned int cluster = root_cluster;
    char part[64];
    int  i = 0, j = 0;

    if (path[0] == '/') i = 1;

    while (1) {
        j = 0;
        while (path[i] && path[i] != '/')
            part[j++] = path[i++];
        part[j] = 0;

        if (j == 0) break;

        cluster = find_in_dir(cluster, part);
        if (cluster == 0) return 0;

        if (path[i] == '/') i++;
        else break;
    }

    unsigned int clus_size = boot.sectors_per_clus * 512;
    *buf  = memory_alloc(clus_size * 16);
    *size = read_chain(cluster, *buf);

    return 1;
}

int fat32_ls(const char* path, FatEntry* entries, int max_count) {
    if (!initialized) return 0;
    if (!entries || max_count == 0) return 0;
    // находим кластер нужной директории
    unsigned int cluster = root_cluster;
    char part[64];
    int i = 0, j = 0;

    if (path[0] == '/') i = 1;

    while (path[i]) {
        j = 0;
        while (path[i] && path[i] != '/') part[j++] = path[i++];
        part[j] = 0;
        if (j == 0) { if (path[i] == '/') i++; continue; }

        cluster = find_in_dir(cluster, part);
        if (cluster == 0) return 0;
        if (path[i] == '/') i++;
    }

    // читаем содержимое директории
    int count = 0;
    unsigned char buf[512];

    while (!fat_is_end(cluster) && count < max_count) {
        unsigned int sector = cluster_to_sector(cluster);

        for (int s = 0; s < boot.sectors_per_clus; s++) {
            disk_read_sector(sector + s, buf);

            for (int n = 0; n < 512 / 32; n++) {
                Fat32DirEntry* e = (Fat32DirEntry*)(buf + n * 32);

                if (e->name[0] == 0x00) return count;
                if (e->name[0] == 0xE5) continue;
                if (e->attributes == 0x0F) continue;
                if (e->attributes & 0x08) continue;  
                if (e->name[0] == '.') continue;     

                entries[count].name[0] = 0;
                entries[count].name[13] = 0;
                entries[count].name[14] = 0;
                entries[count].name[15] = 0;
                entries[count].is_dir = 0;
                entries[count].size = 0;
                
                int jj = 0, k = 0;
                while (jj < 8 && e->name[jj] != ' ')
                    entries[count].name[k++] = e->name[jj++];
                if (e->ext[0] != ' ') {
                    entries[count].name[k++] = '.';
                    jj = 0;
                    while (jj < 3 && e->ext[jj] != ' ')
                        entries[count].name[k++] = e->ext[jj++];
                }
                entries[count].name[k] = 0;
                entries[count].is_dir  = (e->attributes & 0x10) ? 1 : 0;
                entries[count].size    = e->file_size;
                count++;
                if (count >= max_count) return count;
            }
        }
        cluster = fat_next(cluster);
    }
    return count;
}

static void fat_write(unsigned int cluster, unsigned int value) {
    unsigned char sector_buf[512];
    unsigned int fat_offset = cluster * 4;
    unsigned int fat_sector = fat_start + fat_offset / 512;
    unsigned int fat_index  = fat_offset % 512;
    disk_read_sector(fat_sector, sector_buf);
    *(unsigned int*)(sector_buf + fat_index) = value & 0x0FFFFFFF;
    disk_write_sector(fat_sector, sector_buf);
}

static unsigned int fat_alloc_cluster(void) {
    unsigned char sector_buf[512];
    // ищем свободный кластер (значение 0 в FAT)
    for (unsigned int cluster = 2; cluster < 0xFFFFFF0; cluster++) {
        unsigned int fat_offset = cluster * 4;
        unsigned int fat_sector = fat_start + fat_offset / 512;
        unsigned int fat_index  = fat_offset % 512;
        disk_read_sector(fat_sector, sector_buf);
        unsigned int val = *(unsigned int*)(sector_buf + fat_index) & 0x0FFFFFFF;
        if (val == 0) {
            fat_write(cluster, 0x0FFFFFFF); // отмечаем как конец цепочки
            return cluster;
        }
    }
    return 0; // нет свободных кластеров
}

int fat32_write(const char* path, unsigned char* buf, unsigned int size) {
    if (!initialized) return 0;

    // находим родительскую директорию и имя файла
    char dir_path[128];
    char filename[13];
    int i = str_len(path) - 1;
    while (i > 0 && path[i] != '/') i--;
    
    // копируем путь до папки
    int j = 0;
    while (j < i) { dir_path[j] = path[j]; j++; }
    dir_path[j] = 0;
    if (j == 0) { dir_path[0] = '/'; dir_path[1] = 0; }
    
    // копируем имя файла
    j = 0;
    i++;
    while (path[i]) filename[j++] = path[i++];
    filename[j] = 0;

    // находим кластер директории
    unsigned int dir_cluster = root_cluster;
    if (dir_path[0] != '/' || dir_path[1] != 0) {
        char part[64];
        int pi = (dir_path[0] == '/') ? 1 : 0;
        while (dir_path[pi]) {
            j = 0;
            while (dir_path[pi] && dir_path[pi] != '/') part[j++] = dir_path[pi++];
            part[j] = 0;
            if (j == 0) { if (dir_path[pi] == '/') pi++; continue; }
            dir_cluster = find_in_dir(dir_cluster, part);
            if (dir_cluster == 0) return 0;
            if (dir_path[pi] == '/') pi++;
        }
    }

    // выделяем кластеры для данных
    unsigned int bytes_per_clus = boot.sectors_per_clus * 512;
    unsigned int clusters_needed = (size + bytes_per_clus - 1) / bytes_per_clus;
    
    unsigned int first_cluster = fat_alloc_cluster();
    if (first_cluster == 0) return 0;
    
    unsigned int cur_cluster = first_cluster;
    unsigned int written = 0;
    
    for (unsigned int c = 0; c < clusters_needed; c++) {
        unsigned int sector = cluster_to_sector(cur_cluster);
        for (int s = 0; s < boot.sectors_per_clus; s++) {
            unsigned char tmp[512];
            for (int b = 0; b < 512; b++) {
                tmp[b] = (written < size) ? buf[written++] : 0;
            }
            disk_write_sector(sector + s, tmp);
        }
        
        if (c < clusters_needed - 1) {
            unsigned int next = fat_alloc_cluster();
            if (next == 0) return 0;
            fat_write(cur_cluster, next);
            cur_cluster = next;
        }
    }

    // ищем свободную запись в директории
    unsigned int cluster = dir_cluster;
    while (!fat_is_end(cluster)) {
        unsigned int sector = cluster_to_sector(cluster);
        for (int s = 0; s < boot.sectors_per_clus; s++) {
            unsigned char sec_buf[512];
            disk_read_sector(sector + s, sec_buf);
            for (int n = 0; n < 512 / 32; n++) {
                Fat32DirEntry* e = (Fat32DirEntry*)(sec_buf + n * 32);
                if (e->name[0] == 0x00 || e->name[0] == 0xE5) {
                    // свободная запись — заполняем
                    for (int b = 0; b < 8; b++) e->name[b] = ' ';
                    for (int b = 0; b < 3; b++) e->ext[b]  = ' ';
                    
                    // копируем имя в 8.3 формат
                    int ni = 0, ei = 0;
                    int dot = 0;
                    for (int fi = 0; filename[fi]; fi++) {
                        char c = filename[fi];
                        if (c >= 'a' && c <= 'z') c -= 32;
                        if (c == '.') { dot = 1; continue; }
                        if (!dot && ni < 8) e->name[ni++] = c;
                        else if (dot && ei < 3) e->ext[ei++]  = c;
                    }
                    
                    e->attributes  = 0x20; // архивный
                    e->cluster_high = (first_cluster >> 16) & 0xFFFF;
                    e->cluster_low  = first_cluster & 0xFFFF;
                    e->file_size    = size;
                    e->reserved     = 0;
                    e->create_time_ms = 0;
                    e->create_time  = 0;
                    e->create_date  = 0;
                    e->access_date  = 0;
                    e->modify_time  = 0;
                    e->modify_date  = 0;
                    
                    disk_write_sector(sector + s, sec_buf);
                    return 1;
                }
            }
        }
        cluster = fat_next(cluster);
    }
    return 0;
}