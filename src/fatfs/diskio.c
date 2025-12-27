#include "diskio.h"
#include "../filesystem.h" // disk_read_sector, disk_write_sector

DSTATUS disk_initialize(BYTE pdrv) { return 0; }
DSTATUS disk_status(BYTE pdrv) { return 0; }

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    for (UINT i = 0; i < count; i++) {
        if (disk_read_sector(sector + i, (char*)(buff + i * 512)) != 0)
            return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    for (UINT i = 0; i < count; i++) {
        if (disk_write_sector(sector + i, (char*)(buff + i * 512)) != 0)
            return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) { return RES_OK; } 