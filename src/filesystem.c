/*FILESYSTEM FOR KUZUOS2 WRITTEN BY VUJUVUJU AT GOD KNOWS WHEN AND NEVER TOUCHED OR FINGERED N ANY WAYS AGAIN*/
#include "filesystem.h"
#include "memory.h"
#include "process.h"
#include "vga.h"
#include "io.h"
#include "usb.h"
#include "kuzulib/fs/vfs.h"
#include "usb.h"
#include "z_utils.h" // NEEDED FOR GODDAMN PRINTF 
// In-memory ramfs entries (for mkdir/touch)
#define MAX_RAMFS_ENTRIES 256

typedef struct {
    char path[256];
    char name[64];
    int is_directory;
    char *data;
    uint32_t size;
    int used;
} ramfs_entry_t;

static ramfs_entry_t ramfs_entries[MAX_RAMFS_ENTRIES];
static int ramfs_initialized = 0;

static void ramfs_init(void) {
    if (ramfs_initialized) return;
    for (int i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        ramfs_entries[i].used = 0;
        ramfs_entries[i].data = 0;
        ramfs_entries[i].size = 0;
    }
    ramfs_initialized = 1;
}

int ramfs_create_directory(const char *path) {
    extern void print(const char *);
    print("[ramfs_create_directory] START path='");
    print(path);
    print("'\n");
    
    ramfs_init();
    
    // Normalize path - if relative, prepend current directory
    char full_path[256];
    if (path[0] != '/') {
        extern char kernel_cwd[256];
        int i = 0;
        while (kernel_cwd[i] && i < 255) {
            full_path[i] = kernel_cwd[i];
            i++;
        }
        if (i > 1 && full_path[i-1] != '/') full_path[i++] = '/';
        int j = 0;
        while (path[j] && i < 255) {
            full_path[i++] = path[j++];
        }
        full_path[i] = '\0';
    } else {
        int i = 0;
        while (path[i] && i < 256) {
            full_path[i] = path[i];
            i++;
        }
        full_path[i] = '\0';
    }
    
    print("[ramfs_create_directory] full_path='");
    print(full_path);
    print("'\n");
    
    // Check if already exists
    for (int i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        if (ramfs_entries[i].used && strcmp(ramfs_entries[i].path, full_path) == 0) {
            print("[ramfs_create_directory] already exists\n");
            return -1; // Already exists
        }
    }
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        if (!ramfs_entries[i].used) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        print("[ramfs_create_directory] no space\n");
        return -1; // No space
    }
    
    // Extract name from path
    const char *name = full_path;
    for (int i = 0; full_path[i]; i++) {
        if (full_path[i] == '/') name = &full_path[i + 1];
    }
    
    // Create entry
    strcpy(ramfs_entries[slot].path, full_path);
    strcpy(ramfs_entries[slot].name, name[0] ? name : "root");
    ramfs_entries[slot].is_directory = 1;
    ramfs_entries[slot].data = 0;
    ramfs_entries[slot].size = 0;
    ramfs_entries[slot].used = 1;
    
    print("[ramfs_create_directory] SUCCESS\n");
    return 0;
}

int ramfs_create_file(const char *path) {
    ramfs_init();
    
    // Check if already exists
    for (int i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        if (ramfs_entries[i].used && strcmp(ramfs_entries[i].path, path) == 0) {
            return -1; // Already exists
        }
    }
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        if (!ramfs_entries[i].used) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return -1; // No space
    
    // Extract name from path
    const char *name = path;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/') name = &path[i + 1];
    }
    
    // Create entry
    strcpy(ramfs_entries[slot].path, path);
    strcpy(ramfs_entries[slot].name, name[0] ? name : "file");
    ramfs_entries[slot].is_directory = 0;
    ramfs_entries[slot].data = 0;
    ramfs_entries[slot].size = 0;
    ramfs_entries[slot].used = 1;
    
    return 0;
}

int ramfs_exists(const char *path) {
    ramfs_init();
    for (int i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        if (ramfs_entries[i].used && strcmp(ramfs_entries[i].path, path) == 0) {
            return 1;
        }
    }
    return 0;
}

ramfs_entry_t* ramfs_get_entry(const char *path) {
    ramfs_init();
    for (int i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        if (ramfs_entries[i].used && strcmp(ramfs_entries[i].path, path) == 0) {
            return &ramfs_entries[i];
        }
    }
    return 0;
}

int ramfs_get_entry_by_index(uint32_t index, char **name_out, int *is_dir_out) {
    ramfs_init();
    uint32_t count = 0;
    for (int i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        if (ramfs_entries[i].used) {
            if (count == index) {
                *name_out = ramfs_entries[i].name;
                *is_dir_out = ramfs_entries[i].is_directory;
                return 0;
            }
            count++;
        }
    }
    return -1;
}

// Get ramfs entry by index, but only for entries that are direct children of dir_path
int ramfs_get_entry_by_index_in_dir(const char *dir_path, uint32_t index, char **name_out, int *is_dir_out) {
    ramfs_init();
    
    // Normalize dir_path
    int dir_len = 0;
    while (dir_path[dir_len]) dir_len++;
    
    // Remove trailing slash unless it's root
    char normalized_dir[256];
    int i = 0;
    while (i < dir_len && i < 255) {
        normalized_dir[i] = dir_path[i];
        i++;
    }
    if (i > 1 && normalized_dir[i-1] == '/') {
        i--;
    }
    normalized_dir[i] = '\0';
    dir_len = i;
    
    uint32_t count = 0;
    for (i = 0; i < MAX_RAMFS_ENTRIES; i++) {
        if (!ramfs_entries[i].used) continue;
        
        // Check if this entry is a direct child of dir_path
        // Entry path should start with dir_path
        int match = 1;
        int j;
        for (j = 0; j < dir_len; j++) {
            if (ramfs_entries[i].path[j] != normalized_dir[j]) {
                match = 0;
                break;
            }
        }
        
        if (!match) continue;
        
        // After dir_path, there should be a '/' (or nothing for root)
        if (dir_len > 1) {
            if (ramfs_entries[i].path[dir_len] != '/') continue;
            j = dir_len + 1;
        } else {
            // Root directory
            if (ramfs_entries[i].path[0] != '/') continue;
            if (ramfs_entries[i].path[1] == '\0') continue; // Skip root itself
            j = 1;
        }
        
        // Make sure there are no more slashes (direct child, not grandchild)
        int has_slash = 0;
        while (ramfs_entries[i].path[j]) {
            if (ramfs_entries[i].path[j] == '/') {
                has_slash = 1;
                break;
            }
            j++;
        }
        if (has_slash) continue;
        
        // This is a direct child!
        if (count == index) {
            *name_out = ramfs_entries[i].name;
            *is_dir_out = ramfs_entries[i].is_directory;
            return 0;
        }
        count++;
    }
    return -1;
}

// Active ATA I/O ports (default: primary bus). Updated during detection.
static uint16_t ata_io_base = 0x1F0;
static uint16_t ata_ctrl_port = 0x3F6; // currently unused, kept for completeness

// Disk I/O portları (computed from active bus)
#define DISK_DATA_PORT (ata_io_base + 0)
#define DISK_ERROR_PORT (ata_io_base + 1)
#define DISK_SECTOR_COUNT_PORT (ata_io_base + 2)
#define DISK_LBA_LOW_PORT (ata_io_base + 3)
#define DISK_LBA_MID_PORT (ata_io_base + 4)
#define DISK_LBA_HIGH_PORT (ata_io_base + 5)
#define DISK_DRIVE_PORT (ata_io_base + 6)
#define DISK_COMMAND_PORT (ata_io_base + 7)
#define DISK_STATUS_PORT (ata_io_base + 7)

// Disk komutları
#define DISK_CMD_READ 0x20
#define DISK_CMD_WRITE 0x30
#define ATAPI_CMD_PACKET 0xA0
#define ATAPI_CMD_IDENTIFY 0xA1
#define ATAPI_SECTOR_SIZE 2048

// --- Global state ---
static int is_atapi_device = 0;
static uint8_t* ramdisk_buffer = 0;
static uint32_t ramdisk_total_sectors = 0;
static uint8_t ramdisk_enabled = 0;
static int boot_from_usb = 0;  // Flag to track USB boot

// Device type detection
typedef enum {
    DEVICE_TYPE_NONE = 0,
    DEVICE_TYPE_ATA_HDD,
    DEVICE_TYPE_ATAPI_CDROM,
    DEVICE_TYPE_ATAPI_DVD
} DeviceType;

static DeviceType device_type = DEVICE_TYPE_NONE;

static void ata_set_bus(uint16_t io_base, uint16_t ctrl_port) {
    ata_io_base = io_base;
    ata_ctrl_port = ctrl_port;
}

// --- Helper Functions (defined before use) ---
static int strlen_local(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int strncmp_local(const char* a, const char* b, int n) { for (int i=0;i<n;i++){ unsigned char x=a[i], y=b[i]; if (x!=y) return x-y; if (x==0||y==0) return 0;} return 0; }
static int strncmp_case_insensitive(const char* a, const char* b, int n) { 
    for (int i=0;i<n;i++){
        unsigned char x=a[i], y=b[i];
        if (x >= 'a' && x <= 'z') x = x - 'a' + 'A';
        if (y >= 'a' && y <= 'z') y = y - 'a' + 'A';
        if (x!=y) return x-y; 
        if (x==0||y==0) return 0;
    } 
    return 0; 
}
// Case-insensitive strcmp
static int strcmp_case_insensitive(const char* a, const char* b) {
    while (*a || *b) {
        unsigned char x = *a, y = *b;
        if (x >= 'a' && x <= 'z') x = x - 'a' + 'A';
        if (y >= 'a' && y <= 'z') y = y - 'a' + 'A';
        if (x != y) return x - y;
        a++; b++;
    }
    return 0;
}

static char* strchr_local(const char* str, char c) { while (*str != '\0') { if (*str == c) return (char*)str; str++; } return 0; }

// Local strcmp wrapper for const strings
static int strcmp_local(const char* a, const char* b) {
    while (*a || *b) {
        unsigned char x = *a, y = *b;
        if (x != y) return x - y;
        a++; b++;
    }
    return 0;
}
static void* memset(void* dest, int c, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (uint32_t i = 0; i < n; i++) d[i] = (uint8_t)c;
    return dest;
}

void* memcpy(void* dest, const void* src, uint32_t n) { // which dumb fuk did this static for gods sake ASSUME THE FUTUREE
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

// --- Disk Detection System ---

// Status register bits
#define ATA_SR_BSY   0x80    // Busy
#define ATA_SR_DRDY  0x40    // Drive ready
#define ATA_SR_DRQ   0x08    // Data request ready
#define ATA_SR_ERR   0x01    // Error

// ATA commands
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_IDENTIFY_PACKET 0xA1

// Wait for BSY to clear
static int ata_wait_bsy() {
    for (int i = 0; i < 4; i++) inb(DISK_STATUS_PORT);
    
    for (int i = 0; i < 10000; i++) {
        uint8_t status = inb(DISK_STATUS_PORT);
        if (!(status & ATA_SR_BSY))
            return 0;
    }
    return -1;
}

// Wait for DRQ to be set
static int ata_wait_drq_set() {
    for (int i = 0; i < 10000; i++) {
        uint8_t status = inb(DISK_STATUS_PORT);
        if (status & ATA_SR_DRQ)
            return 0;
        if (status & ATA_SR_ERR)
            return -1;
    }
    return -1;
}

// Read 256 words from disk data port
static void read_identify_buffer(uint16_t* buffer) {
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(DISK_DATA_PORT);
    }
}

// Main device detection for a single bus - returns device type or NONE
static DeviceType detect_device_on_bus(uint16_t io_base, uint16_t ctrl_port) {
    ata_set_bus(io_base, ctrl_port);

    // Select primary/secondary master on this bus
    outb(DISK_DRIVE_PORT, 0xA0);
    for (volatile int i = 0; i < 100000; i++);
    
    // Check for ATAPI signature FIRST (before any commands)
    uint8_t lba_mid = inb(DISK_LBA_MID_PORT);
    uint8_t lba_high = inb(DISK_LBA_HIGH_PORT);
    
    print("Initial signature: LBA_MID=0x");
    char hex[3];
    hex[0] = "0123456789ABCDEF"[lba_mid >> 4];
    hex[1] = "0123456789ABCDEF"[lba_mid & 0xF];
    hex[2] = 0;
    print(hex);
    print(" LBA_HIGH=0x");
    hex[0] = "0123456789ABCDEF"[lba_high >> 4];
    hex[1] = "0123456789ABCDEF"[lba_high & 0xF];
    print(hex); print("\n");
    
    // If ATAPI signature is present, don't even try ATA commands
    if (lba_mid == 0x14 && lba_high == 0xEB) {
        print_color("ATAPI signature detected - skipping ATA detection!\n", VGA_COLOR_LIGHT_GREEN);
        is_atapi_device = 1;
        return DEVICE_TYPE_ATAPI_CDROM;
    }
    
    // Check if device exists
    uint8_t status = inb(DISK_STATUS_PORT);
    if (status == 0 || status == 0xFF) {
        print_color("No status response on this bus\n", VGA_COLOR_YELLOW);
        return DEVICE_TYPE_NONE;
    }
    
    // Wait for not busy
    int timeout = 0;
    while ((inb(DISK_STATUS_PORT) & 0x80) && timeout++ < 100000);
    
    if (timeout >= 100000) {
        print_color("Device busy timeout on this bus\n", VGA_COLOR_YELLOW);
        return DEVICE_TYPE_NONE;
    }
    
    // Try IDENTIFY command
    outb(DISK_COMMAND_PORT, ATA_CMD_IDENTIFY);
    
    for (volatile int i = 0; i < 400; i++) inb(DISK_STATUS_PORT);
    
    status = inb(DISK_STATUS_PORT);
    
    // Check signature AGAIN after IDENTIFY
    lba_mid = inb(DISK_LBA_MID_PORT);
    lba_high = inb(DISK_LBA_HIGH_PORT);
    
    print("After IDENTIFY: LBA_MID=0x");
    hex[0] = "0123456789ABCDEF"[lba_mid >> 4];
    hex[1] = "0123456789ABCDEF"[lba_mid & 0xF];
    print(hex);
    print(" LBA_HIGH=0x");
    hex[0] = "0123456789ABCDEF"[lba_high >> 4];
    hex[1] = "0123456789ABCDEF"[lba_high & 0xF];
    print(hex); print("\n");
    
    // ATAPI signature appeared?
    if (lba_mid == 0x14 && lba_high == 0xEB) {
        print_color("ATAPI signature detected after IDENTIFY!\n", VGA_COLOR_LIGHT_GREEN);
        
        // Send DEVICE RESET
        outb(DISK_COMMAND_PORT, 0x08);
        for (volatile int i = 0; i < 50000; i++);
        
        outb(DISK_DRIVE_PORT, 0xA0);
        for (volatile int i = 0; i < 1000; i++);
        outb(DISK_COMMAND_PORT, ATA_CMD_IDENTIFY_PACKET);
        
        timeout = 0;
        while ((inb(DISK_STATUS_PORT) & 0x80) && timeout++ < 100000);
        
        timeout = 0;
        while (!(inb(DISK_STATUS_PORT) & 0x08) && timeout++ < 100000);
        
        if (timeout < 100000) {
            uint16_t identify_buf[256];
            read_identify_buffer(identify_buf);
        }
        
        print_color("Device: ATAPI CD-ROM/DVD\n", VGA_COLOR_LIGHT_GREEN);
        is_atapi_device = 1;
        return DEVICE_TYPE_ATAPI_CDROM;
    }
    
    // If IDENTIFY had error, treat as no device
    if (status & ATA_SR_ERR) {
        print_color("IDENTIFY error on this bus\n", VGA_COLOR_YELLOW);
        return DEVICE_TYPE_NONE;
    }
    
    // Wait for DRQ
    timeout = 0;
    while (!(inb(DISK_STATUS_PORT) & 0x08) && timeout++ < 100000);
    
    if (timeout >= 100000) {
        print_color("No DRQ on this bus\n", VGA_COLOR_YELLOW);
        return DEVICE_TYPE_NONE;
    }
    
    uint16_t identify_buf[256];
    read_identify_buffer(identify_buf);
    
    print_color("Device: ATA HDD/SSD\n", VGA_COLOR_LIGHT_GREEN);
    is_atapi_device = 0;
    return DEVICE_TYPE_ATA_HDD;
}

// Wrapper that scans primary then secondary
static DeviceType detect_device() {
    print("\n=== Disk Detection ===\n");
    DeviceType type = detect_device_on_bus(0x1F0, 0x3F6);
    if (type != DEVICE_TYPE_NONE) return type;
    print("Trying secondary IDE bus (expected for QEMU -cdrom)...\n");
    return detect_device_on_bus(0x170, 0x376);
}

// Forward declarations
static int disk_wait();

// Read one 2048-byte ATAPI block via READ(10)
static int atapi_read_block_2048(uint32_t lba, char* buffer) {
    // Select master drive
    outb(DISK_DRIVE_PORT, 0xA0);
    
    // Small delay after select
    inb(DISK_STATUS_PORT); inb(DISK_STATUS_PORT); inb(DISK_STATUS_PORT); inb(DISK_STATUS_PORT);
    
    // Wait for BSY=0, DRDY=1
    int timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(DISK_STATUS_PORT);
        if (!(status & 0x80) && (status & 0x40)) break;
    }
    
    // Set Features register (use DMA=0, overlap=0)
    outb(DISK_ERROR_PORT, 0x00);
    
    // Set transfer size (2048 bytes max)
    outb(DISK_LBA_MID_PORT, 0x00);  // LBA mid = low byte of 0x0800
    outb(DISK_LBA_HIGH_PORT, 0x08);  // LBA high = high byte
    
    // Send PACKET command
    outb(DISK_COMMAND_PORT, 0xA0);
    
    // Wait 400ns
    inb(DISK_STATUS_PORT); inb(DISK_STATUS_PORT); inb(DISK_STATUS_PORT); inb(DISK_STATUS_PORT);
    
    // Wait for BSY=0
    timeout = 100000;
    while (timeout--) {
        if (!(inb(DISK_STATUS_PORT) & 0x80)) break;
    }
    
    // Check for error
    uint8_t status = inb(DISK_STATUS_PORT);
    if (status & 0x01) return -1;
    
    // Wait for DRQ=1 (ready for packet)
    timeout = 100000;
    while (timeout--) {
        status = inb(DISK_STATUS_PORT);
        if (status & 0x08) break;
        if (status & 0x01) return -1;
    }
    if (!(status & 0x08)) return -1;
    
    // Prepare READ(10) SCSI command packet (12 bytes)
    // SCSI commands are BIG-ENDIAN, but we send as little-endian WORDS
    uint8_t cmd[12];
    cmd[0] = 0x28;  // READ(10) opcode
    cmd[1] = 0x00;  // Reserved/LUN
    cmd[2] = (lba >> 24) & 0xFF;  // LBA byte 3 (MSB)
    cmd[3] = (lba >> 16) & 0xFF;  // LBA byte 2
    cmd[4] = (lba >> 8) & 0xFF;   // LBA byte 1
    cmd[5] = lba & 0xFF;          // LBA byte 0 (LSB)
    cmd[6] = 0x00;  // Reserved
    cmd[7] = 0x00;  // Transfer length high byte
    cmd[8] = 0x01;  // Transfer length low byte (1 sector)
    cmd[9] = 0x00;  // Control
    cmd[10] = 0x00; // Padding
    cmd[11] = 0x00; // Padding
    
    // Convert to little-endian words for ATA data port
    uint16_t packet[6];
    for (int i = 0; i < 6; i++) {
        packet[i] = cmd[i*2] | (cmd[i*2+1] << 8);
    }
    
    // Send packet (6 words = 12 bytes)
    for (int i = 0; i < 6; i++) {
        outw(DISK_DATA_PORT, packet[i]);
    }
    
    // Wait for BSY=0 (command processing)
    timeout = 1000000;  // Longer timeout for seeking
    while (timeout--) {
        if (!(inb(DISK_STATUS_PORT) & 0x80)) break;
    }
    
    // Check for error after command
    status = inb(DISK_STATUS_PORT);
    if (status & 0x01) {
        uint8_t err = inb(DISK_ERROR_PORT);
        return -1;
    }
    
    // Wait for DRQ=1 (data ready)
    timeout = 1000000;
    while (timeout--) {
        status = inb(DISK_STATUS_PORT);
        if (status & 0x08) break;
        if (status & 0x01) return -1;
    }
    if (!(status & 0x08)) return -1;
    
    // Read actual byte count device is sending
    uint16_t byte_count = inb(DISK_LBA_MID_PORT) | (inb(DISK_LBA_HIGH_PORT) << 8);
    
    // Typically should be 2048, but read what device says
    uint16_t words = byte_count / 2;
    if (words > 1024) words = 1024;  // Safety cap
    
    // Read the data
    for (int i = 0; i < words; i++) {
        uint16_t data = inw(DISK_DATA_PORT);
        buffer[i*2] = data & 0xFF;
        buffer[i*2+1] = (data >> 8) & 0xFF;
    }
    
    // If device sent less than 2048 bytes, zero the rest
    for (int i = words*2; i < 2048; i++) {
        buffer[i] = 0;
    }
    
    // Wait for command complete (BSY=0, DRQ=0)
    timeout = 100000;
    while (timeout--) {
        status = inb(DISK_STATUS_PORT);
        if (!(status & 0x88)) break;  // !BSY && !DRQ
    }
    
    return 0;
}

int disk_wait() {
    uint32_t attempts = 1000000;
    while (attempts--) {
        uint8_t status = inb(DISK_STATUS_PORT);
        if ((status & 0x80) == 0) return 0;
    }
    return -1;
}

int disk_read_sector(uint32_t lba, char* buffer) {
    if (ramdisk_enabled) {
        if (lba >= ramdisk_total_sectors) return -1;
        memcpy(buffer, ramdisk_buffer + (lba * 512), 512);
        return 0;
    }
    
    // USB boot: read directly from USB device
    if (boot_from_usb) {
        extern usb_device_t usb_devices[16];
        extern int msc_read_sector(usb_device_t*, unsigned int, void*);
        
        // Find USB mass storage device
        for (int i = 0; i < 16; i++) {
            if (usb_devices[i].used && usb_devices[i].connected && 
                usb_devices[i].driver == 2) {  // USB_DRIVER_MSC = 2
                return msc_read_sector(&usb_devices[i], lba, buffer);
            }
        }
        // No USB device found, fall through to ATA/ATAPI
    }

    // Use ATAPI for CD-ROM/DVD devices
    if (device_type == DEVICE_TYPE_ATAPI_CDROM || device_type == DEVICE_TYPE_ATAPI_DVD) {
        char blk2048[2048];
        uint32_t lba2048 = lba / 4;
        uint32_t off = (lba % 4) * 512;

        if (atapi_read_block_2048(lba2048, blk2048) == 0) {
            memcpy(buffer, blk2048 + off, 512);
            return 0;
        } else {
            return -1;
        }
    }

    // Standard ATA/HDD mode
    if (disk_wait() != 0) return -1;
    
    outb(DISK_SECTOR_COUNT_PORT, 1);
    outb(DISK_LBA_LOW_PORT, (uint8_t)(lba & 0xFF));
    outb(DISK_LBA_MID_PORT, (uint8_t)((lba >> 8) & 0xFF));
    outb(DISK_LBA_HIGH_PORT, (uint8_t)((lba >> 16) & 0xFF));
    outb(DISK_DRIVE_PORT, (uint8_t)(((lba >> 24) & 0x0F) | 0xE0));
    outb(DISK_COMMAND_PORT, 0x20);
    
    uint32_t attempts = 1000000;
    while (attempts--) {
        uint8_t status = inb(DISK_STATUS_PORT);
        if (status & 0x01) return -1;
        if (status & 0x08) break;
    }
    if ((inb(DISK_STATUS_PORT) & 0x08) == 0) return -1;
    
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(DISK_DATA_PORT);
        buffer[i * 2] = data & 0xFF;
        buffer[i * 2 + 1] = (data >> 8) & 0xFF;
    }
    return 0;
}

int disk_write_sector(uint32_t lba, char* buffer) {
    if (ramdisk_enabled) {
        if (lba >= ramdisk_total_sectors) return -1;
        if (!ramdisk_buffer) return -1;
        memcpy(ramdisk_buffer + (lba * 512), buffer, 512);
        return 0;
    }

    if (disk_wait() != 0) return -1;
    
    outb(DISK_SECTOR_COUNT_PORT, 1);
    outb(DISK_LBA_LOW_PORT, (uint8_t)(lba & 0xFF));
    outb(DISK_LBA_MID_PORT, (uint8_t)((lba >> 8) & 0xFF));
    outb(DISK_LBA_HIGH_PORT, (uint8_t)((lba >> 16) & 0xFF));
    outb(DISK_DRIVE_PORT, (uint8_t)(((lba >> 24) & 0x0F) | 0xE0));
    outb(DISK_COMMAND_PORT, 0x30);
    
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        outw(DISK_DATA_PORT, data);
    }
    
    uint32_t attempts2 = 1000000;
    while (attempts2--) {
        uint8_t status = inb(DISK_STATUS_PORT);
        if (status & 0x01) return -1;
        if ((status & 0x80) == 0) break;
    }
    if ((inb(DISK_STATUS_PORT) & 0x80) != 0) return -1;
    
    return 0;
}

// --- ISO9660 minimal reader from RAM overlay ---
static int iso_read_block2048(uint32_t lba2048, char* out2048) {
    uint8_t prev = ramdisk_enabled;
    ramdisk_enabled = 0;
    
    // Try ATAPI/DVD read if device is CD-ROM/DVD
    if ((device_type == DEVICE_TYPE_ATAPI_CDROM || device_type == DEVICE_TYPE_ATAPI_DVD) && 
        atapi_read_block_2048(lba2048, out2048) == 0) {
        ramdisk_enabled = prev;
        return 0;
    }
    
    // Fall back to reading 4 ATA sectors
    for (int i = 0; i < 4; i++) {
        if (disk_read_sector(lba2048 * 4 + i, out2048 + i * 512) != 0) {
            ramdisk_enabled = prev;
            return -1;
        }
    }
    ramdisk_enabled = prev;
    return 0;
}

static int iso_get_volume_size_blocks(uint32_t* out_blocks2048) {
    char pvd[2048];
    
    print("Attempting to read ISO9660 PVD at block 16...\n");
    
    // Disable ramdisk temporarily to read from actual hardware
    uint8_t prev_ramdisk = ramdisk_enabled;
    ramdisk_enabled = 0;
    
    int read_success = 0;
    
    // Try ATAPI method first if detected as ATAPI
    if (is_atapi_device) {
        print("Using ATAPI read method...\n");
        if (atapi_read_block_2048(16, pvd) == 0) {
            print("ATAPI read successful, checking PVD...\n");
            read_success = 1;
        } else {
            print("ATAPI read failed, trying ATA method...\n");
        }
    }
    
    // Try ATA method if ATAPI failed or not detected
    if (!read_success) {
        print("Using ATA read method (4x512 sectors)...\n");
        int read_failed = 0;
        for (int i = 0; i < 4; i++) {
            if (disk_read_sector(16 * 4 + i, pvd + i * 512) != 0) {
                read_failed = 1;
                print("ATA read failed at sector ");
                char sbuf[8]; int s = 16*4+i, pos=0;
                if (s==0) sbuf[pos++]='0'; 
                else { 
                    char rev[8]; int rp=0; 
                    while(s>0){rev[rp++]='0'+(s%10);s/=10;} 
                    while(rp--)sbuf[pos++]=rev[rp];
                }
                sbuf[pos]=0; print(sbuf); print("\n");
                break;
            }
        }
        
        if (read_failed) {
            ramdisk_enabled = prev_ramdisk;
            print("Cannot read from disk - no ISO detected\n");
            return -1;
        }
        
        print("ATA read successful, checking PVD...\n");
        read_success = 1;
    }
    
    // Restore ramdisk state
    ramdisk_enabled = prev_ramdisk;
    
    // Check PVD signature
    if ((unsigned char)pvd[0] != 0x01) {
        print("Invalid PVD type: 0x");
        char hex[3];
        hex[0] = "0123456789ABCDEF"[(unsigned char)pvd[0] >> 4];
        hex[1] = "0123456789ABCDEF"[(unsigned char)pvd[0] & 0xF];
        hex[2] = 0;
        print(hex); print(" (expected 0x01)\n");
        return -1;
    }
    
    if (pvd[1] != 'C' || pvd[2] != 'D' || pvd[3] != '0' || 
        pvd[4] != '0' || pvd[5] != '1') {
        print("Invalid CD001 signature: ");
        for (int i = 1; i <= 5; i++) {
            if (pvd[i] >= 32 && pvd[i] <= 126) {
                char c[2] = {pvd[i], 0};
                print(c);
            } else {
                print("?");
            }
        }
        print(" (expected CD001)\n");
        return -1;
    }
    
    print("Valid ISO9660 PVD found!\n");
    
    uint32_t vss = (uint32_t)(uint8_t)pvd[80] | 
                   ((uint32_t)(uint8_t)pvd[81] << 8) | 
                   ((uint32_t)(uint8_t)pvd[82] << 16) | 
                   ((uint32_t)(uint8_t)pvd[83] << 24);
    
    if (vss == 0) {
        print("Volume size is zero!\n");
        return -1;
    }
    
    print("ISO volume size: ");
    char sbuf[16]; int pos=0; uint32_t v = vss;
    if (v==0) sbuf[pos++]='0'; 
    else { 
        char rev[16]; int rp=0; 
        while(v>0){rev[rp++]='0'+(v%10);v/=10;} 
        while(rp--)sbuf[pos++]=rev[rp];
    }
    sbuf[pos]=0; print(sbuf); print(" blocks (2048 bytes each)\n");
    
    *out_blocks2048 = vss;
    return 0;
}

typedef struct { uint32_t lba; uint32_t size; } iso_extent;

static int iso_get_root(iso_extent* out) {
    char pvd[2048];
    int read_result = iso_read_block2048(16, pvd);
    if (read_result != 0) {
        return -1;
    }
    
    if ((unsigned char)pvd[0] != 0x01 || pvd[1] != 'C' || pvd[2] != 'D' || pvd[3] != '0' || pvd[4] != '0' || pvd[5] != '1') {
        return -1;
    }
    
    unsigned char* rr = (unsigned char*)(pvd + 156);
    if (rr[0] < 34) {
        return -1;
    }
    
    out->lba = (uint32_t)rr[2] | ((uint32_t)rr[3] << 8) | ((uint32_t)rr[4] << 16) | ((uint32_t)rr[5] << 24);
    out->size = (uint32_t)rr[10] | ((uint32_t)rr[11] << 8) | ((uint32_t)rr[12] << 16) | ((uint32_t)rr[13] << 24);
    
    return 0;
}

// Helper: convert string to lowercase
static void to_lowercase(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = str[i] - 'A' + 'a';
        }
    }
}

static void iso_norm_token(const char* in, char* out, int out_len) {
    int pos = 0;
    for (int i = 0; in[i] && pos < out_len - 1; i++) {
        char c = in[i];
        if (c == ';') break;
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        if (c == '_') c = '-';
        if (c == '.') continue;
        out[pos++] = c;
    }
    out[pos] = '\0';
}

static int iso_names_match(const char* search, const char* iso_name) {
    char a[64];
    char b[64];
    iso_norm_token(search, a, sizeof(a));
    iso_norm_token(iso_name, b, sizeof(b));
    if (strcmp_local(a, b) == 0) return 1;

    int alen = strlen_local(a);
    int blen = strlen_local(b);
    if (blen == 8 && alen > 8 && strncmp_local(a, b, 8) == 0) return 1;
    if (alen == 8 && blen > 8 && strncmp_local(b, a, 8) == 0) return 1;
    return 0;
}

static int iso_find_in_dir(iso_extent dir, const char* name, iso_extent* out, int* is_dir) {
    uint32_t blocks = (dir.size + 2047) / 2048;
    for (uint32_t b=0;b<blocks;b++) {
        char blk[2048]; if (iso_read_block2048(dir.lba + b, blk) != 0) return -1;
        uint32_t off = 0; while (off < 2048) {
            uint8_t len = (uint8_t)blk[off + 0]; if (len == 0) break;
            unsigned char* dr = (unsigned char*)(blk + off);
            uint8_t flags = dr[25];
            uint8_t fi_len = dr[32]; const char* fi = (const char*)(dr + 33);
            if (!(fi_len == 1 && (fi[0] == 0 || fi[0] == 1))) {
                char nm[64]; int nlen=(fi_len<63)?fi_len:63; for (int k=0;k<nlen;k++) nm[k]=fi[k]; nm[nlen]=0; for (int k=0;k<nlen;k++) if (nm[k]==';'){ nm[k]=0; break; }
                to_lowercase(nm);
                int nm_len = strlen_local(nm);
                if (nm_len > 0 && nm[nm_len-1] == '.') nm[nm_len-1] = 0; nm_len = strlen_local(nm);
                
                // Convert search name to lowercase for comparison
                char name_lower[64]; 
                int name_len = strlen_local(name);
                for (int k=0; k<name_len && k<63; k++) name_lower[k] = name[k];
                name_lower[name_len] = 0;
                to_lowercase(name_lower);
                
                // Remove trailing dot from search name too
                if (name_len > 0 && name_lower[name_len-1] == '.') name_lower[name_len-1] = 0;
                name_len = strlen_local(name_lower);
                
                // Match exact or ISO9660 8.3 truncated names (qwerty-layout -> qwerty_l)
                if (iso_names_match(name, nm)) {
                    out->lba = (uint32_t)dr[2] | ((uint32_t)dr[3] << 8) | ((uint32_t)dr[4] << 16) | ((uint32_t)dr[5] << 24);
                    out->size= (uint32_t)dr[10]| ((uint32_t)dr[11]<<8)| ((uint32_t)dr[12]<<16)| ((uint32_t)dr[13]<<24);
                    *is_dir = (flags & 0x02) ? 1 : 0;
                    return 0;
                }
            }
            off += len;
        }
    }
    return -1;
}

static int iso_lookup_path(const char* path, iso_extent* out, int* is_dir) {
    iso_extent cur; if (iso_get_root(&cur)!=0) return -1; int dirflag=1; if (path[0]!='/') return -1; const char* p=path+1; if (*p==0){ *out=cur; *is_dir=1; return 0; }
    char token[64]; while (*p) {
        int t=0; while (*p && *p!='/') { if (t<63) token[t++]=*p; p++; } token[t]=0; if (*p=='/') p++;
        iso_extent next; int next_is_dir=0; if (iso_find_in_dir(cur, token, &next, &next_is_dir)!=0) return -1; cur=next; dirflag=next_is_dir;
    }
    *out=cur; *is_dir=dirflag; return 0;
}

static char* strchr(const char* str, char c) {
    while (*str != '\0') {
        if (*str == c) return (char*)str;
        str++;
    }
    return 0;
}

int fs_disk_test() {
    char test_buffer[512];
    char test_data[512];
    for (int i = 0; i < 512; i++) test_data[i] = i & 0xFF;
    if (disk_write_sector(10, test_data) != 0) {
        print("Disk write test failed\n");
        return -1;
    }
    if (disk_read_sector(10, test_buffer) != 0) {
        print("Disk read test failed\n");
        return -1;
    }
    for (int i = 0; i < 512; i++) {
        if (test_buffer[i] != test_data[i]) {
            print("Disk data verification failed\n");
            return -1;
        }
    }
    print("Disk I/O test passed\n");
    return 0;
}

#define FS_SECTOR_START 1000
#define FS_SECTOR_COUNT 8

static int fs_read_header(struct fs_header* header);
static int fs_write_header(const struct fs_header* header);

// Static global to avoid stack overflow - reused across calls
static struct fs_header g_fs_header_temp;

static void fs_ensure_header_initialized() {
    if (fs_read_header(&g_fs_header_temp) != 0 || g_fs_header_temp.magic != FS_MAGIC) {
        for (int i = 0; i < sizeof(struct fs_header); i++) ((char*)&g_fs_header_temp)[i] = 0;
        g_fs_header_temp.magic = FS_MAGIC;
        g_fs_header_temp.num_files = 1;
        strcpy(g_fs_header_temp.files[0].name, "root");
        strcpy(g_fs_header_temp.files[0].path, "/");
        g_fs_header_temp.files[0].size = 0;
        g_fs_header_temp.files[0].offset = 0;
        g_fs_header_temp.files[0].used = 1;
        g_fs_header_temp.files[0].is_directory = 1;
        fs_write_header(&g_fs_header_temp);
    }
}

// RAMDISK API
void ramdisk_init(uint32_t total_sectors) {
    if (ramdisk_enabled) return;
    uint32_t bytes = total_sectors * 512;
    ramdisk_buffer = (uint8_t*)kmalloc(bytes);
    if (ramdisk_buffer) {
        ramdisk_total_sectors = total_sectors;
        memset(ramdisk_buffer, 0, bytes);
        ramdisk_enabled = 1;
        print("RAM disk enabled (");
        char mbuf[16]; int pos = 0; uint32_t v = bytes / (1024*1024); 
        if (v == 0) { mbuf[pos++] = '0'; } else { 
            char rev[16]; int rp = 0; 
            while (v > 0) { rev[rp++] = '0' + (v % 10); v /= 10; } 
            while (rp--) mbuf[pos++] = rev[rp]; 
        } 
        mbuf[pos] = 0; 
        print(mbuf); print("MB)\n");
    } else {
        ramdisk_enabled = 0;
        print("RAM disk allocation failed\n");
    }
}

void ramdisk_init_auto() {
    if (ramdisk_enabled) return;
    const uint32_t try_sizes_mb[] = {64, 32, 16, 8, 4, 2};
    for (unsigned i = 0; i < sizeof(try_sizes_mb)/sizeof(try_sizes_mb[0]); i++) {
        uint32_t sectors = (try_sizes_mb[i] * 1024 * 1024) / 512;
        ramdisk_init(sectors);
        if (ramdisk_enabled) {
            print("RAM disk size: ");
            char mbuf[8]; int pos = 0; uint32_t v = try_sizes_mb[i]; if (v == 0) mbuf[pos++] = '0'; else { char rev[8]; int rp = 0; while (v > 0) { rev[rp++] = '0' + (v % 10); v /= 10; } while (rp--) mbuf[pos++] = rev[rp]; } mbuf[pos] = 0; print(mbuf); print("MB\n");
            return;
        }
    }
    print("ERROR: Could not allocate any ramdisk size\n");
}

// USB boot detection helper
static usb_device_t* find_usb_boot_device(void) {
    extern usb_device_t usb_devices[MAX_USB_DEVICES];
    for (int i = 0; i < MAX_USB_DEVICES; i++) {
        if (usb_devices[i].used && usb_devices[i].connected && 
            usb_devices[i].driver == USB_DRIVER_MSC) {
            return &usb_devices[i];
        }
    }
    return 0;
}

// Preload from USB mass storage device
void ramdisk_preload_from_usb(usb_device_t* dev, uint32_t start_lba, uint32_t sector_count) {
    if (!ramdisk_enabled) {
        print("FATAL: RAM disk not enabled! Skipping preload.\n");
        return;
    }
    if (sector_count == 0) {
        print("FATAL: sector_count is zero! Nothing to copy.\n");
        return;
    }
    if (sector_count > ramdisk_total_sectors)
        sector_count = ramdisk_total_sectors;

    extern int msc_read_sector(usb_device_t*, unsigned int, void*);
    
    char tmp[512];
    uint32_t total = sector_count;
    uint32_t last_shown_percent = 101;
    const uint32_t bar_width = 30;
    uint32_t read_errors = 0;
    uint32_t consecutive_errors = 0;

    print("Copying ISO from USB to RAM (read-only USB -> RAM, RW enabled)\n");
    print("Starting USB read... (this may take a while)\n");

    // Read sector by sector with delays to avoid USB timeouts
    for (uint32_t i = 0; i < sector_count; i++) {
        // Longer delay every sector to prevent USB issues
        if ((i & 0x7) == 0x7) {
            for (volatile int d = 0; d < 100000; d++);
        }
        
        int r = msc_read_sector(dev, start_lba + i, tmp);
        
        if (r != 0) {
            read_errors++;
            consecutive_errors++;
            // Zero fill on error
            for (int b = 0; b < 512; b++) tmp[b] = 0;
            
            // If too many consecutive errors, abort
            if (consecutive_errors > 10) {
                print_color("\n\nToo many USB read errors, aborting\n", VGA_COLOR_RED);
                z_printf("Failed at sector %u\n", start_lba + i);
                break;
            }
            
            // Longer delay after error
            for (volatile int d = 0; d < 500000; d++);
        } else {
            consecutive_errors = 0;
        }
        
        memcpy(ramdisk_buffer + ((start_lba + i) * 512), tmp, 512);

        uint32_t done = i + 1;
        if (total == 0) total = 1;
        uint32_t percent = (done * 100) / total;
        
        // Show progress every 1% instead of 5% to show it's working
        if (percent > last_shown_percent || done == total) {
            last_shown_percent = percent;
            char bar[31];
            uint32_t filled = (percent * bar_width) / 100;
            for (uint32_t j = 0; j < bar_width; j++) bar[j] = (j < filled) ? '#' : '-';
            bar[bar_width] = '\0';

            print("  ["); print(bar); print("] ");
            char pbuf[4]; int p = percent; int ppos = 0;
            if (p == 0) pbuf[ppos++] = '0';
            else {
                char rev[4]; int rp = 0;
                while (p > 0 && rp < 3) { rev[rp++] = '0' + (p % 10); p /= 10; }
                while (rp--) pbuf[ppos++] = rev[rp];
            }
            pbuf[ppos] = 0; print(pbuf); print("%  ");
            
            uint32_t mb_done = (done * 512) / (1024*1024);
            uint32_t mb_total = (total * 512) / (1024*1024);
            char nbuf[16]; int npos = 0;
            if (mb_done == 0) nbuf[npos++] = '0';
            else {
                char revn[16]; int rn = 0;
                while (mb_done > 0) { revn[rn++] = '0' + (mb_done % 10); mb_done /= 10; }
                while (rn--) nbuf[npos++] = revn[rn];
            }
            nbuf[npos] = 0; print(nbuf);
            print("MB/");
            npos = 0;
            if (mb_total == 0) nbuf[npos++] = '0';
            else {
                char revt[16]; int rt = 0;
                while (mb_total > 0) { revt[rt++] = '0' + (mb_total % 10); mb_total /= 10; }
                while (rt--) nbuf[npos++] = revt[rt];
            }
            nbuf[npos] = 0; print(nbuf); print("MB");
            
            if (read_errors > 0) {
                print(" (");
                char ebuf[8]; int epos = 0; uint32_t e = read_errors;
                if (e == 0) ebuf[epos++] = '0';
                else {
                    char reve[8]; int re = 0;
                    while (e > 0) { reve[re++] = '0' + (e % 10); e /= 10; }
                    while (re--) ebuf[epos++] = reve[re];
                }
                ebuf[epos] = 0; print(ebuf); print(" errors)");
            }
            print("\n");
        }
    }

    print_color("\nUSB->RAM preload complete. Operating on RAM (no writes to USB)\n", VGA_COLOR_LIGHT_GREEN);
    boot_from_usb = 1;
}

void ramdisk_preload_from_lba(uint32_t start_lba, uint32_t sector_count) {
    if (!ramdisk_enabled) {
        print("FATAL: RAM disk not enabled! Skipping preload.\n");
        return;
    }
    if (sector_count == 0) {
        print("FATAL: sector_count is zero! Nothing to copy.\n");
        return;
    }
    if (sector_count > ramdisk_total_sectors)
        sector_count = ramdisk_total_sectors;

    struct fs_header header_snapshot;
    {
        char hdrbuf[4096];
        uint8_t prev = ramdisk_enabled;
        ramdisk_enabled = 0;
        int hdr_ok = 1;
        for (int i = 0; i < FS_SECTOR_COUNT; i++) {
            if (disk_read_sector(FS_SECTOR_START + i, hdrbuf + i * 512) != 0) { hdr_ok = 0; break; }
        }
        ramdisk_enabled = prev;
        if (hdr_ok) {
            for (int i = 0; i < sizeof(struct fs_header); i++) ((char*)&header_snapshot)[i] = hdrbuf[i];
        } else {
            for (int i = 0; i < sizeof(struct fs_header); i++) ((char*)&header_snapshot)[i] = 0;
        }
    }

    char tmp[512];
    uint32_t total = sector_count;
    uint32_t last_shown_percent = 101;
    const uint32_t bar_width = 30;
    uint32_t read_errors = 0;

    print("Copying system image to RAM (read-only ISO -> RAM, RW enabled)\n");

    #define BATCH_SIZE 16
    for (uint32_t i = 0; i < sector_count && (start_lba + i) < ramdisk_total_sectors; ) {
        uint32_t batch_count = (BATCH_SIZE < (sector_count - i)) ? BATCH_SIZE : (sector_count - i);
        if ((start_lba + i + batch_count) > ramdisk_total_sectors) {
            batch_count = ramdisk_total_sectors - (start_lba + i);
        }

        for (uint32_t j = 0; j < batch_count; j++) {
            // Try ATAPI/DVD first if device is CD-ROM/DVD
            int r = -1;
            if (device_type == DEVICE_TYPE_ATAPI_CDROM || device_type == DEVICE_TYPE_ATAPI_DVD) {
                char blk2048[2048];
                uint32_t lba2048 = (start_lba + i + j) / 4;
                uint32_t off = ((start_lba + i + j) % 4) * 512;
                if (atapi_read_block_2048(lba2048, blk2048) == 0) {
                    for (int b = 0; b < 512; b++) tmp[b] = blk2048[off + b];
                    r = 0;
                }
            }
            
            // If ATAPI failed or not ATAPI device, try ATA
            if (r != 0) {
                uint8_t prev = ramdisk_enabled;
                ramdisk_enabled = 0;
                r = disk_read_sector(start_lba + i + j, tmp);
                ramdisk_enabled = prev;
            }
            
            if (r != 0) {
                read_errors++;
                // Zero fill on error instead of stopping
                for (int b = 0; b < 512; b++) tmp[b] = 0;
            } else {
                read_errors = 0; // reset error counter on successful read
            }
            memcpy(ramdisk_buffer + ((start_lba + i + j) * 512), tmp, 512);
        }

        i += batch_count;

        uint32_t done = i;
        if (total == 0) total = 1; // avoid div by zero
        uint32_t percent = (done * 100) / total;
        if (percent >= last_shown_percent + 5 || done == total) {
            last_shown_percent = percent;
            char bar[31];
            uint32_t filled = (percent * bar_width) / 100;
            for (uint32_t j = 0; j < bar_width; j++) bar[j] = (j < filled) ? '#' : '-';
            bar[bar_width] = '\0';

            print("  ["); print(bar); print("] ");
            // Percent
            char pbuf[4]; int p = percent; int ppos = 0;
            if (p == 0) pbuf[ppos++] = '0';
            else {
                char rev[4]; int rp = 0;
                while (p > 0 && rp < 3) { rev[rp++] = '0' + (p % 10); p /= 10; }
                while (rp--) pbuf[ppos++] = rev[rp];
            }
            pbuf[ppos] = 0; print(pbuf); print("%  ");
            // MB
            uint32_t mb_done = (done * 512) / (1024*1024);
            uint32_t mb_total = (total * 512) / (1024*1024);
            char nbuf[16]; int npos = 0;
            if (mb_done == 0) nbuf[npos++] = '0';
            else {
                char revn[16]; int rn = 0;
                while (mb_done > 0) { revn[rn++] = '0' + (mb_done % 10); mb_done /= 10; }
                while (rn--) nbuf[npos++] = revn[rn];
            }
            nbuf[npos] = 0; print(nbuf);
            print("MB/");
            npos = 0;
            if (mb_total == 0) nbuf[npos++] = '0';
            else {
                char revt[16]; int rt = 0;
                while (mb_total > 0) { revt[rt++] = '0' + (mb_total % 10); mb_total /= 10; }
                while (rt--) nbuf[npos++] = revt[rt];
            }
            nbuf[npos] = 0; print(nbuf); print("MB\n");
        }
    }

    print_color("RAM preload complete. Operating on RAM (no writes to ISO)\n", VGA_COLOR_LIGHT_GREEN);
}

void fs_init() {
    ramdisk_init_auto();
    if (!ramdisk_enabled) {
        print("Trying fallback 4MB RAM disk...\n");
        ramdisk_init(8192); // 4MB
    }

    if (!ramdisk_enabled) {
        print_color("FATAL: RAM disk initialization failed. Cannot proceed!\n", VGA_COLOR_RED);
        return;
    }

    print("Ramdisk initialized, will check for boot media after USB init\n");
    
    // NOTE: CD/DVD boot detection moved to fs_late_init() to check USB first
}

// Late filesystem init - call AFTER usb_init() to enable USB boot
void fs_late_init() {
    // Check for USB boot device first
    print("Checking for USB boot device...\n");
    for (volatile int i = 0; i < 5000000; i++);  // Brief delay for enumeration
    
    usb_device_t* usb_boot = find_usb_boot_device();
    if (usb_boot) {
        print_color("USB boot device detected!\n", VGA_COLOR_LIGHT_CYAN);
        z_printf("USB: vendor=0x%x product=0x%x\n", usb_boot->vendor_id, usb_boot->product_id);
        print_color("USB boot enabled - disk reads will use USB device\n", VGA_COLOR_LIGHT_GREEN);
        
        // Mark as USB boot BEFORE reading ISO
        boot_from_usb = 1;
        
        // Now try to detect ISO on USB
        print("Detecting ISO filesystem on USB...\n");
        uint32_t iso_blocks = 0;
        if (iso_get_volume_size_blocks(&iso_blocks) == 0) {
            print_color("ISO filesystem detected on USB!\n", VGA_COLOR_LIGHT_GREEN);
            char sbuf[16]; int pos = 0; uint32_t v = iso_blocks;
            if (v == 0) sbuf[pos++] = '0';
            else {
                char rev[16]; int rp = 0;
                while (v) { rev[rp++] = '0' + (v % 10); v /= 10; }
                while (rp--) sbuf[pos++] = rev[rp];
            }
            sbuf[pos] = 0; print("ISO size: "); print(sbuf); print(" blocks\n");
        } else {
            print_color("WARNING: No ISO detected on USB, trying raw filesystem\n", VGA_COLOR_YELLOW);
        }
        
        // Init filesystem header
        fs_ensure_header_initialized();
        
        print_color("USB boot complete - system ready!\n", VGA_COLOR_LIGHT_GREEN);
        return;
    }
    
    // No USB, try CD/DVD
    print("No USB boot device, trying CD/DVD...\n");
    
    device_type = DEVICE_TYPE_ATAPI_CDROM;
    is_atapi_device = 1;
    
    DeviceType detected = detect_device();
    if (detected != DEVICE_TYPE_NONE) {
        device_type = detected;
    }

    uint32_t iso_blocks = 0;
    
    if (iso_get_volume_size_blocks(&iso_blocks) == 0) {
        print_color("CD/DVD ISO detected!\n", VGA_COLOR_LIGHT_GREEN);
        
        uint32_t clone_sectors = iso_blocks * 4;
        if (clone_sectors == 0) {
            clone_sectors = 8192;
        }
        if (clone_sectors > ramdisk_total_sectors) {
            clone_sectors = ramdisk_total_sectors;
        }

        char sbuf[16]; int pos = 0; uint32_t v = clone_sectors;
        if (v == 0) sbuf[pos++] = '0';
        else {
            char rev[16]; int rp = 0;
            while (v) { rev[rp++] = '0' + (v % 10); v /= 10; }
            while (rp--) sbuf[pos++] = rev[rp];
        }
        sbuf[pos] = 0; print("Loading "); print(sbuf); print(" sectors from CD/DVD\n");

        ramdisk_preload_from_lba(0, clone_sectors);

        if (fs_disk_test() != 0) {
            print("CD/DVD filesystem test failed\n");
        } else {
            print_color("CD/DVD boot successful :)\n", VGA_COLOR_LIGHT_GREEN);
        }
    } else {
        print("shit...\n");
        fs_ensure_header_initialized();
        if (fs_disk_test() != 0) {
            print("shit\n");
        }
    }
}


// AFTER THIS FUCKING SUCKS

int fs_create_directory(char* path) {
    fs_ensure_header_initialized();
    
    // Use static global to avoid stack overflow
    if (fs_read_header(&g_fs_header_temp) != 0) return -1;
    if (g_fs_header_temp.num_files >= MAX_FILES) return -1;
    
    int slot = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!g_fs_header_temp.files[i].used) { slot = i; break; }
    }
    if (slot == -1) return -3;
    
    char* name = path;
    for (int i = 0; path[i] != '\0'; i++) if (path[i] == '/') name = &path[i + 1];
    if (name[0] == '\0') name = "root";
    
    strcpy(g_fs_header_temp.files[slot].name, name);
    strcpy(g_fs_header_temp.files[slot].path, path);
    g_fs_header_temp.files[slot].size = 0;
    g_fs_header_temp.files[slot].offset = 0;
    g_fs_header_temp.files[slot].used = 1;
    g_fs_header_temp.files[slot].is_directory = 1;
    g_fs_header_temp.num_files++;
    
    if (fs_write_header(&g_fs_header_temp) != 0) return -4;
    return 0;
}

int fs_create_file(char* path, char* data, uint32_t size) {
    fs_ensure_header_initialized();
    struct fs_header header;
    if (fs_read_header(&header) != 0) return -1;
    if (header.num_files >= MAX_FILES) return -1;
    if (size > MAX_FILE_SIZE) return -2;

    int slot = -1;
    for (int i = 0; i < MAX_FILES; i++)
        if (!header.files[i].used) { slot = i; break; }
    if (slot == -1) return -3;

    char* name = path;
    for (int i = 0; path[i]; i++) if (path[i] == '/') name = &path[i+1];
    if (!name[0]) name = "file";

    strcpy(header.files[slot].name, name);
    strcpy(header.files[slot].path, path);
    header.files[slot].size = size;
    header.files[slot].offset = 0; // unused now
    header.files[slot].used = 1;
    header.files[slot].is_directory = 0;
    header.num_files++;

    // write header separately from data
    if (fs_write_header(&header) != 0) return -4;

    // write file data to its own sector slot
    uint32_t data_lba = FS_SECTOR_START + FS_SECTOR_COUNT + slot;
    char* sector = (char*)kmalloc(512);
    if (!sector) return -5;
    for (int i = 0; i < 512; i++) sector[i] = 0;
    for (uint32_t i = 0; i < size && i < 512; i++) sector[i] = data[i];
    disk_write_sector(data_lba, sector);
    kfree(sector);
    return 0;
}

// Use ISO for read when not found in tiny FS
int fs_read_file(char* path, char* buffer, uint32_t max_size) {
    if (!path || !buffer) return -1;

    // Try TinyFS first
    struct fs_header header;
    if (fs_read_header(&header) == 0 && header.magic == FS_MAGIC) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (header.files[i].used && strcmp(header.files[i].path, path) == 0) {
                if (header.files[i].is_directory) return -1;
                
                uint32_t size = header.files[i].size;
                if (size > max_size) size = max_size;

                // read from per-slot sector instead of broken offset
                uint32_t data_lba = FS_SECTOR_START + FS_SECTOR_COUNT + i;
                char* sector = (char*)kmalloc(512);
                if (!sector) return -1;
                if (disk_read_sector(data_lba, sector) != 0) {
                    kfree(sector);
                    return -1;
                }
                for (uint32_t j = 0; j < size; j++) buffer[j] = sector[j];
                kfree(sector);
                return (int)size;
            }
        }
    }

    // Fallback to ISO data in RAM
    iso_extent e;
    int isdir = 0;
    if (iso_lookup_path(path, &e, &isdir) != 0 || isdir) return -1;

    uint32_t remaining = e.size;
    if (remaining > max_size) remaining = max_size;
    uint32_t blocks = (remaining + 2047) / 2048;
    uint32_t copied = 0;
    for (uint32_t b = 0; b < blocks; b++) {
        char blk[2048];
        if (iso_read_block2048(e.lba + b, blk) != 0) break;
        uint32_t tocpy = (remaining - copied > 2048) ? 2048 : (remaining - copied);
        for (uint32_t i = 0; i < tocpy; i++) buffer[copied + i] = blk[i];
        copied += tocpy;
    }
    return copied > 0 ? (int)copied : -1;
}

int fs_get_file_size(char* path) {
    // Check TinyFS first
    struct fs_header header;
    if (fs_read_header(&header) == 0 && header.magic == FS_MAGIC) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (header.files[i].used && strcmp(header.files[i].path, path) == 0) {
                return (int)header.files[i].size;
            }
        }
    }
    
    // Fallback to ISO image
    iso_extent e;
    int isdir = 0;
    if (iso_lookup_path(path, &e, &isdir) == 0 && !isdir) {
        return (int)e.size;
    }
    
    return -1;
}

int fs_write_file(char* name, char* data, uint32_t size) {
    fs_delete_file(name, 0);
    return fs_create_file(name, data, size);
}

int fs_delete_file(const char* path, int recursive) {
    struct fs_header header;
    if (fs_read_header(&header) != 0) return -1;
    int found = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.files[i].used && strcmp(header.files[i].path, path) == 0) {
            found = 1;
            // Eğer dizin ve recursive ise, altındaki her şeyi sil
            if (header.files[i].is_directory && recursive) {
                // Altındaki dosya ve dizinleri bul
                int pathlen = strlen(path);
                for (int j = 0; j < MAX_FILES; j++) {
                    if (header.files[j].used && strncmp(header.files[j].path, path, pathlen) == 0 && strcmp(header.files[j].path, path) != 0) {
                        
                        fs_delete_file(header.files[j].path, 1);
                    }
                }
            } else if (header.files[i].is_directory && !recursive) {
               
                int pathlen = strlen(path);
                for (int j = 0; j < MAX_FILES; j++) {
                    if (header.files[j].used && strncmp(header.files[j].path, path, pathlen) == 0 && strcmp(header.files[j].path, path) != 0) {
                        
                        return -3;
                    }
                }
            }
            header.files[i].used = 0;
            header.num_files--;
            if (fs_write_header(&header) != 0) return -2;
            return 0;
        }
    }
    if (!found) return -1;
    return 0;
}

static void iso_print_tree_recursive(iso_extent dir, char* prefix, int prefix_len, int depth) {
    if (depth > 16) return;
    static char blk[2048];
    uint32_t blocks = (dir.size + 2047) / 2048;
    for (uint32_t b=0;b<blocks;b++) {
        if (iso_read_block2048(dir.lba + b, blk)!=0) break; uint32_t off=0; while (off<2048) {
            uint8_t len=(uint8_t)blk[off+0]; if (len==0) break; unsigned char* dr=(unsigned char*)(blk+off); uint8_t flags=dr[25]; uint8_t fi_len=dr[32]; const char* fi=(const char*)(dr+33);
            if (!(fi_len==1&&(fi[0]==0||fi[0]==1))) {
                char name[64]; int nlen=(fi_len<63)?fi_len:63; for(int k=0;k<nlen;k++) name[k]=fi[k]; name[nlen]=0; for(int k=0;k<nlen;k++) if (name[k]==';'){ name[k]=0; break;}
                to_lowercase(name);
                char pathbuf[256]; int pos=0; for(int i=0;i<prefix_len && i<250;i++) pathbuf[pos++]=prefix[i]; if (pos==0 || pathbuf[pos-1]!='/') pathbuf[pos++]='/'; for(int i=0;i<nlen && pos<255;i++) pathbuf[pos++]=name[i]; pathbuf[pos]=0;
                print(pathbuf); print("\n");
                if (flags & 0x02) {
                    iso_extent next; next.lba = (uint32_t)dr[2] | ((uint32_t)dr[3] << 8) | ((uint32_t)dr[4] << 16) | ((uint32_t)dr[5] << 24);
                    next.size = (uint32_t)dr[10]| ((uint32_t)dr[11]<<8)| ((uint32_t)dr[12]<<16)| ((uint32_t)dr[13]<<24);
                    char newprefix[256]; int np=0; for(int i=0;i<pos && i<255;i++) newprefix[np++]=pathbuf[i]; newprefix[np]=0; iso_print_tree_recursive(next, newprefix, np, depth+1);
                }
            }
            off+=len;
        }
    }
}

static void iso_list_dir_extent(iso_extent dir) {
    uint32_t blocks = (dir.size + 2047) / 2048;
    for (uint32_t b=0;b<blocks;b++) {
        char blk[2048]; if (iso_read_block2048(dir.lba + b, blk)!=0) break; uint32_t off=0; while (off<2048) {
            uint8_t len=(uint8_t)blk[off+0]; if (len==0) break; unsigned char* dr=(unsigned char*)(blk+off); uint8_t flags=dr[25]; uint8_t fi_len=dr[32]; const char* fi=(const char*)(dr+33);
            if (!(fi_len==1&&(fi[0]==0||fi[0]==1))) { char name[64]; int nlen=(fi_len<63)?fi_len:63; for(int k=0;k<nlen;k++) name[k]=fi[k]; name[nlen]=0; for(int k=0;k<nlen;k++) if (name[k]==';'){ name[k]=0; break;} to_lowercase(name); print("  "); if (flags&0x02) print("[DIR] "); else print("[FILE] "); print(name); print("\n"); }
            off+=len; }
    }
}

void fs_list_all() {
    print("--- ISO contents ---\n");
    iso_extent root; if (iso_get_root(&root)==0) { char prefix[2] = "/"; iso_print_tree_recursive(root, prefix, 1, 0); }
    print("--- TinyFS contents ---\n");
    char diskbuf[4096]; for (int i=0;i<FS_SECTOR_COUNT;i++) { disk_read_sector(FS_SECTOR_START + i, diskbuf + i*512); }
    struct fs_header* header=(struct fs_header*)diskbuf;
    if (header->magic == FS_MAGIC) {
        for (int i=0;i<MAX_FILES;i++) {
            if (!header->files[i].used) continue;
            print(header->files[i].path);
            if (!header->files[i].is_directory) {
                print(" (");
                char size_str[16]; int size=header->files[i].size; int pos=0; if (size==0) size_str[pos++]='0'; else { while (size>0){ size_str[pos++]='0'+(size%10); size/=10; } } for (int j=pos-1;j>=0;j--) putchar(size_str[j]); print(" bytes)");
            }
            print("\n");
        }
    }
}

int fs_any_exists(char* path) {
    // Check TinyFS
    struct fs_header header; if (fs_read_header(&header) == 0 && header.magic == FS_MAGIC) {
        for (int i=0;i<MAX_FILES;i++) if (header.files[i].used && strcmp(header.files[i].path, path) == 0) return 1;
    }
    // Check ISO
    iso_extent e; int isdir=0; if (iso_lookup_path(path, &e, &isdir)==0) return 1;
    return 0;
}

int fs_any_is_directory(char* path) {
    struct fs_header header;

    if (fs_read_header(&header) == 0 && header.magic == FS_MAGIC) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (header.files[i].used &&
                strcmp(header.files[i].path, path) == 0)
                return header.files[i].is_directory;
        }
    }

    iso_extent e;
    int isdir = 0;

    if (iso_lookup_path(path, &e, &isdir) == 0)
        return isdir;

    return 0;
}
void fs_list_files(char* current_path) {
    // If path exists in ISO and is a directory, list ISO contents
    iso_extent e; int e_is_dir = 0; int iso_ok = (iso_lookup_path(current_path, &e, &e_is_dir) == 0 && e_is_dir);
    if (iso_ok) {
        print("Files in "); print(current_path); print(":\n");
        iso_list_dir_extent(e);
    } else if (current_path && current_path[0] == '/' && current_path[1] == '\0') {
        // List ISO root if path is exactly '/'
        iso_extent root; if (iso_get_root(&root)==0) { print("Files in /:\n"); iso_list_dir_extent(root); }
    } else {
        print("Files in "); print(current_path); print(":\n");
    }

    // Also list TinyFS entries within this path (direct children)
    char diskbuf[4096]; for (int i=0;i<FS_SECTOR_COUNT;i++) { disk_read_sector(FS_SECTOR_START + i, diskbuf + i*512); }
    struct fs_header* header=(struct fs_header*)diskbuf;
    int path_len = strlen(current_path);
    char norm_path[64]; strcpy(norm_path, current_path);
    if (!(path_len == 1 && current_path[0] == '/') && current_path[path_len-1] != '/') { norm_path[path_len] = '/'; norm_path[path_len+1] = 0; path_len++; }

    for (int i=0;i<MAX_FILES;i++) {
        if (!header->files[i].used) continue;
        char* fpath = header->files[i].path;
        if (strncmp(fpath, norm_path, path_len) == 0) {
            char* rest = fpath + path_len;
            char* next_slash = strchr(rest, '/');
            if (!next_slash || next_slash[1] == 0) {
                print("  "); if (header->files[i].is_directory) print("[DIR] "); else print("[FILE] ");
                print(header->files[i].name);
                if (!header->files[i].is_directory) {
                    print(" ("); char size_str[16]; int size=header->files[i].size; int pos=0; if (size==0) size_str[pos++]='0'; else { while (size>0){ size_str[pos++]='0'+(size%10); size/=10; } } for(int j=pos-1;j>=0;j--) putchar(size_str[j]); print(" bytes)");
                }
                print("\n");
            }
        }
    }
}

static int fs_read_header(struct fs_header* header) {
    char buffer[4096];
    for (int i = 0; i < FS_SECTOR_COUNT; i++) {
        if (disk_read_sector(FS_SECTOR_START + i, buffer + i * 512) != 0)
            return -1;
    }
    for (int i = 0; i < sizeof(struct fs_header); i++)
        ((char*)header)[i] = buffer[i];
    return 0;
}

static int fs_write_header(const struct fs_header* header) {
    char buffer[4096] = {0};
    for (int i = 0; i < sizeof(struct fs_header); i++)
        buffer[i] = ((const char*)header)[i];
    for (int i = 0; i < FS_SECTOR_COUNT; i++) {
        if (disk_write_sector(FS_SECTOR_START + i, buffer + i * 512) != 0)
            return -1;
    }
    return 0;
}

struct fs_header* get_fs_header(void) {
    static struct fs_header cached_header;
    fs_read_header(&cached_header);
    return &cached_header;
}



// MEEE AND MA MONKEEEE yeee MONKE EATS THE BANANEEEEE

int fs_chdir(char *path)
{
    if (!path || !path[0])
        return -1;

    char resolved_path[256];

    /* resolve relative path */
    if (path[0] != '/') {
        extern char kernel_cwd[256];

        int i = 0;
        while (kernel_cwd[i] && i < 255) {
            resolved_path[i] = kernel_cwd[i];
            i++;
        }

        if (i > 1 && resolved_path[i - 1] != '/')
            resolved_path[i++] = '/';

        int j = 0;
        while (path[j] && i < 255)
            resolved_path[i++] = path[j++];

        resolved_path[i] = '\0';
    } else {
        int i = 0;
        while (path[i] && i < 255) {
            resolved_path[i] = path[i];
            i++;
        }
        resolved_path[i] = '\0';
    }

    /* remove trailing slash except for root */
    int len = 0;
    while (resolved_path[len])
        len++;

    while (len > 1 && resolved_path[len - 1] == '/') {
        resolved_path[len - 1] = '\0';
        len--;
    }

    /* ask VFS whether it exists and is a directory */
    {
        uint32_t size = 0;
        int is_dir = 0;

        if (vfs_stat(resolved_path, &size, &is_dir) != 0)
            return -1;

        if (!is_dir)
            return -1;
    }

    /* success */
    {
        extern char kernel_cwd[256];

        int i = 0;
        while (resolved_path[i] && i < 255) {
            kernel_cwd[i] = resolved_path[i];
            i++;
        }

        kernel_cwd[i] = '\0';
    }

    return 0;
}

// for the VFS

int fs_readdir_index(char *dir_path, uint32_t idx,
                     char *name_out, int *is_dir_out) {
    iso_extent e; int isdir = 0;
    if (iso_lookup_path(dir_path, &e, &isdir) != 0 || !isdir) return -1;
    uint32_t count = 0;
    uint32_t blocks = (e.size + 2047) / 2048;
    for (uint32_t b = 0; b < blocks; b++) {
        char blk[2048];
        if (iso_read_block2048(e.lba + b, blk) != 0) break;
        uint32_t off = 0;
        while (off < 2048) {
            uint8_t len = (uint8_t)blk[off];
            if (len == 0) break;
            unsigned char *dr = (unsigned char *)(blk + off);
            uint8_t flags  = dr[25];
            uint8_t fi_len = dr[32];
            const char *fi = (const char *)(dr + 33);
            if (!(fi_len == 1 && (fi[0] == 0 || fi[0] == 1))) {
                if (count == idx) {
                    int nlen = (fi_len < 63) ? fi_len : 63;
                    for (int k = 0; k < nlen; k++) name_out[k] = fi[k];
                    name_out[nlen] = '\0';
                    for (int k = 0; k < nlen; k++)
                        if (name_out[k] == ';') { name_out[k] = '\0'; break; }
                    to_lowercase(name_out);
                    *is_dir_out = (flags & 0x02) ? 1 : 0;
                    return 1;
                }
                count++;
            }
            off += len;
        }
    }
    return 0;
}


// for VFS

void fs_create_directory_raw(const char *path) {
    extern uint8_t *ramdisk_buffer;
    extern uint8_t ramdisk_enabled;
    if (!ramdisk_enabled || !ramdisk_buffer) return;

    /* the header lives at sectors FS_SECTOR_START..FS_SECTOR_START+FS_SECTOR_COUNT */
    struct fs_header *header = (struct fs_header *)(ramdisk_buffer + FS_SECTOR_START * 512);

    /* if not initialized yet, do it now directly in the buffer */
    if (header->magic != FS_MAGIC) {
        for (int i = 0; i < sizeof(struct fs_header); i++)
            ((char *)header)[i] = 0;
        header->magic = FS_MAGIC;
        header->num_files = 1;
        header->files[0].used = 1;
        header->files[0].is_directory = 1;
        header->files[0].size = 0;
        header->files[0].offset = 0;
        /* name and path = "/" */
        header->files[0].name[0] = '/'; header->files[0].name[1] = '\0';
        header->files[0].path[0] = '/'; header->files[0].path[1] = '\0';
    }

    /* find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!header->files[i].used) { slot = i; break; }
    }
    if (slot == -1) return; /* no space */

    /* extract name from path */
    const char *name = path;
    for (int i = 0; path[i]; i++) if (path[i] == '/') name = &path[i+1];
    if (name[0] == '\0') name = "root";

    /* copy name */
    int k = 0;
    while (name[k] && k < MAX_FILENAME_LENGTH - 1) {
        header->files[slot].name[k] = name[k]; k++;
    }
    header->files[slot].name[k] = '\0';

    /* copy path */
    k = 0;
    while (path[k] && k < MAX_PATH_LENGTH - 1) {
        header->files[slot].path[k] = path[k]; k++;
    }
    header->files[slot].path[k] = '\0';

    header->files[slot].size        = 0;
    header->files[slot].offset      = 0;
    header->files[slot].used        = 1;
    header->files[slot].is_directory = 1;
    header->num_files++;
}

// yay 1944
