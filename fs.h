#ifndef FS_H
#define FS_H

#include <stdint.h>
#include "fs/fat/ff.h"

typedef struct {
    FATFS fatfs;
} fs_t;

void fs_init  (fs_t *fs);
void fs_deinit(fs_t *fs);
int  fs_read  (const char *path, uint8_t *buf, uint32_t capacity);
int  fs_write (const char *path, const uint8_t *content, uint32_t size);
int  fs_delete(const char *path);
void fs_list  (void (*callback)(const char *path));
void run_program(const char *path);
int fs_append(const char *path, const uint8_t *content, uint32_t size);
int fs_cd(const char *path);
int fs_mkdir(const char *path);
int fs_getcwd(char *buf, uint32_t size);

#endif