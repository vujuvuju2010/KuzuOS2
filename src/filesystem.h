#ifndef FILESYSTEM_H
#define FILESYSTEM_H

// Kendi typedef'lerimiz
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// File system constants
#define MAX_FILES 32
#define MAX_FILENAME_LENGTH 64 // was 32 before
#define MAX_PATH_LENGTH 128 // was 64 before
#define MAX_FILE_SIZE 512 // was 256 before
#define FS_MAGIC 0x12345678

// File entry structure
struct fs_file_entry {
    char name[MAX_FILENAME_LENGTH];
    char path[MAX_PATH_LENGTH];  // Full path
    uint32_t size;
    uint32_t offset;
    uint8_t used;
    uint8_t is_directory;  // 1 if directory, 0 if file
};
struct fs_header* get_fs_header(void);
// File system header
struct fs_header {
    uint32_t magic;
    uint32_t num_files;
    struct fs_file_entry files[MAX_FILES];
};

// File system fonksiyonları
void fs_init();
void fs_late_init();  // Call after usb_init() for USB boot support
int fs_disk_test(); // Disk I/O test function
int fs_create_file(char* name, char* data, uint32_t size);
int fs_create_directory(char* path);
int fs_read_file(char* name, char* buffer, uint32_t max_size);
int fs_get_file_size(char* path);
int fs_write_file(char* name, char* data, uint32_t size);
int fs_delete_file(const char* path, int recursive);
void fs_list_files(char* current_path);
void fs_list_all();
int fs_file_exists(char* path);
int fs_is_directory(char* path);
int fs_any_exists(char* path);
int fs_any_is_directory(char* path);
int fs_chdir(char* path);
void fs_show_copy_progress();

// Low level sector I/O
int disk_read_sector(uint32_t lba, char* buffer);
int disk_write_sector(uint32_t lba, char* buffer);

// Forward declaration for USB device type
struct usb_device;

// RAM-backed virtual disk overlay
void ramdisk_init(uint32_t total_sectors);
void ramdisk_init_auto();
void ramdisk_preload_from_lba(uint32_t start_lba, uint32_t sector_count);
void ramdisk_preload_from_usb(struct usb_device* usb_dev, uint32_t start_lba, uint32_t sector_count);
// for VFS
int fs_readdir_index(char *dir_path, uint32_t idx,
                     char *name_out, int *is_dir_out);

void fs_create_directory_raw(const char *path);

// Virtual mount structure (needed by VFS)
typedef struct virtual_mount_s {
    char name[64];
    int (*is_virtual_dir)(const char *path);
    int (*list_virtual_dir)(const char *path, uint32_t index, char *name_out, int *is_dir_out);
    int (*stat_virtual)(const char *path, uint32_t *size_out, int *is_dir_out);
} virtual_mount_t;

// Virtual mount functions
virtual_mount_t* find_virtual_mount(const char *path);

// RAMFS functions
int ramfs_create_directory(const char *path);
int ramfs_create_file(const char *path);
int ramfs_exists(const char *path);

#endif
