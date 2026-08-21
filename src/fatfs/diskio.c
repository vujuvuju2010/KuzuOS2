/*
DISK IO FOR KUZUOS2 WRITTEN BY VUJUVUJU N CLAUDE (+ patched)
credist to the writers of ff.c and ffconf.c
*/

#include "ff.h"
#include "diskio.h"
#include "../filesystem.h"
#include "../usb.h"
#include "../vga.h"
#include "../z_utils.h"


// we assume these exist in your MSC code:
int msc_read_sector(usb_device_t* dev, unsigned int lba, void* buf);
int msc_write_sector(usb_device_t* dev, unsigned int lba, void* buf);
int msc_send_cum(usb_device_t* dev, unsigned char* cb, unsigned char cb_len,
                 void* data, unsigned int data_len, int is_in);


// ---- small helpers --------------------------------------------------

extern usb_device_t usb_devices[MAX_USB_DEVICES];

static usb_device_t* get_msc_dev(void) {
    for (int i = 0; i < MAX_USB_DEVICES; i++) {
        z_printf("  usb_devices[%d]: used=%d connected=%d driver=%d\n",
            i, usb_devices[i].used, usb_devices[i].connected, usb_devices[i].driver);
        if (usb_devices[i].used &&
            usb_devices[i].connected &&
            usb_devices[i].driver == USB_DRIVER_MSC)
            return &usb_devices[i];
    }
    return 0;
}

// READ CAPACITY (10)
static int msc_read_capacity(usb_device_t* dev,
                             unsigned int* block_count,
                             unsigned int* block_size)
{
    unsigned char cb[10] = {
        0x25, // READ CAPACITY (10)
        0,0,0,0,
        0,0,0,0,0
    };
    unsigned char* buf = (unsigned char*)dma_alloc(8);

    if (msc_send_cum(dev, cb, 10, buf, 8, 1) != 0)
        return -1;

    unsigned int last_lba = (buf[0] << 24) | (buf[1] << 16) |
                            (buf[2] << 8)  |  buf[3];
    unsigned int size     = (buf[4] << 24) | (buf[5] << 16) |
                            (buf[6] << 8)  |  buf[7];

    if (block_count) *block_count = last_lba + 1;
    if (block_size)  *block_size  = size;
    return 0;
}

// cache capacity in the device; add these to usb_device_t in usb.h:
//
//   unsigned int msc_sectors;
//   unsigned int msc_sector_size;
//
static void ensure_msc_capacity(usb_device_t* dev)
{
    if (dev->msc_sectors != 0 && dev->msc_sector_size != 0)
        return; // already known

    unsigned int blocks = 0, bsize = 0;
    if (msc_read_capacity(dev, &blocks, &bsize) == 0) {
        dev->msc_sectors     = blocks;
        dev->msc_sector_size = bsize;
        z_printf("MSC capacity: %u sectors, %u bytes/sector\n", blocks, bsize);
    } else {
        print_color("READ CAPACITY failed\n", VGA_COLOR_RED);
        // fallback, some sticks lie but almost all are 512
        dev->msc_sectors     = 0;
        dev->msc_sector_size = 512;
    }
}

// static DMA-safe sector buffer - allocated once at startup
static BYTE* sector_dma_buf = 0;

static BYTE* get_sector_buf(void) {
    if (!sector_dma_buf)
        sector_dma_buf = (BYTE*)dma_alloc(512);
    return sector_dma_buf;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    (void)pdrv;
    // for now: nothing fancy, USB already init'd elsewhere
    return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
    (void)pdrv;
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    z_printf("disk_read: pdrv=%d sector=%u count=%u\n", pdrv, (unsigned)sector, (unsigned)count);
    // pdrv 0 = USB, others = your ATA or whatever
    if (pdrv == 0) {
        usb_device_t* dev = get_msc_dev();
        if (!dev) {
            z_printf("disk_read: no MSC device found!\n");
            return RES_NOTRDY;
        }

        // make sure we know capacity / sector size
        ensure_msc_capacity(dev);

        BYTE* dma_buf = (BYTE*)dma_alloc(512);
        if (!dma_buf) return RES_ERROR;
        
        for (UINT i = 0; i < count; i++) {
            int r = msc_read_sector(dev, (unsigned int)(sector + i), dma_buf);
            z_printf("disk_read: msc_read_sector lba=%u ret=%d dma_buf=%x\n", (unsigned)(sector+i), r, (unsigned)dma_buf);
            if (r != 0)
                return RES_ERROR;
            // dump first 16 bytes of sector 0 for debug
            if (sector + i == 0) {
                z_printf("sector0: ");
                for (int d = 0; d < 16; d++) z_printf("%x ", dma_buf[d]);
                z_printf("\n");
                z_printf("sig: %x %x\n", dma_buf[510], dma_buf[511]);
            }
            // copy from DMA buffer to FatFS buffer
            for (int j = 0; j < 512; j++)
                buff[i * 512 + j] = dma_buf[j];
        }
        
        return RES_OK;
    }

    // fallback to your existing ATA driver (if you have one)
    for (UINT i = 0; i < count; i++) {
        if (disk_read_sector((LBA_t)(sector + i),
                             (char*)(buff + i * 512)) != 0)
            return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv == 0) {
        usb_device_t* dev = get_msc_dev();
        if (!dev) return RES_NOTRDY;

        ensure_msc_capacity(dev);

        // Use a DMA-safe bounce buffer
        BYTE* dma_buf = get_sector_buf();

        for (UINT i = 0; i < count; i++) {
            // copy from FatFS buffer into DMA buffer
            for (int j = 0; j < 512; j++)
                dma_buf[j] = buff[i * 512 + j];
            if (msc_write_sector(dev, (unsigned int)(sector + i), dma_buf) != 0)
                return RES_ERROR;
        }
        return RES_OK;
    }

    // fallback ATA
    for (UINT i = 0; i < count; i++) {
        if (disk_write_sector((LBA_t)(sector + i),
                              (char*)(buff + i * 512)) != 0)
            return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv == 0) { // USB MSC
        usb_device_t* dev = get_msc_dev();
        if (!dev) return RES_NOTRDY;

        ensure_msc_capacity(dev);

        switch (cmd) {
        case CTRL_SYNC:
            // everything is synchronous already
            return RES_OK;

        case GET_SECTOR_COUNT:
            if (!buff) return RES_PARERR;
            *(DWORD*)buff = dev->msc_sectors;
            return RES_OK;

        case GET_SECTOR_SIZE:
            if (!buff) return RES_PARERR;
            *(WORD*)buff = dev->msc_sector_size ? dev->msc_sector_size : 512;
            return RES_OK;

        case GET_BLOCK_SIZE:
            if (!buff) return RES_PARERR;
            *(DWORD*)buff = 1; // minimal erase block size in sectors
            return RES_OK;

        default:
            return RES_PARERR;
        }
    }

    // other drives (ATA etc) – adjust if you actually use them
    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_SIZE:
        if (!buff) return RES_PARERR;
        *(WORD*)buff = 512;
        return RES_OK;
    case GET_BLOCK_SIZE:
        if (!buff) return RES_PARERR;
        *(DWORD*)buff = 1;
        return RES_OK;
    case GET_SECTOR_COUNT:
        if (!buff) return RES_PARERR;
        *(DWORD*)buff = 0; // unknown for now
        return RES_OK;
    default:
        return RES_PARERR;
    }
}