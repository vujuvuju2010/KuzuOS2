/*
USB DISKS N SHII DRIVERRR FOR KUZUOS2 
written y vujuvuju with helps of claude and gpt 51 :)) iykyk i guesss HEHEHEHEHEHE

made while listening to MEGADETHHH WATCHHIMBECAVMEAGOWDDDD


*/
#define USB_STS 0x04 // FOR THE GODDAMN DEBUG LMAOOOOOOOOOO
#include "usb.h"
#include "vga.h" // still rng 0 remember?
#include "z_utils.h" // printf my beloved dear  note from the future: we are in ring 0 why bother w ring 0??? 
#define QTD_CERR(x) ((x) << 10) // QTDCERR LAZIM
#include "fatfs/ff.h"
#include "xchi.h"
static FATFS usb_fs;

// MASSIVE transferrrr
static int ehci_bulk(unsigned int addr, unsigned int ep, void* buf, unsigned int len, int is_in){

    qtd_t* qtd = dma_alloc(sizeof(qtd_t));
    qh_t* qh = dma_alloc(sizeof(qh_t)); 

    unsigned char* p = (unsigned char*)qtd;
    for(int i=0;i<sizeof(qtd_t);i++)p[i] = 0;
    p = (unsigned char*)qh;
    for(int i = 0; i < sizeof(qh_t); i++) p[i] = 0;

    qtd->next = 0x01;
    qtd->alt_next = 0x01; // ALT?? LINKIN PARK MENTIONEDDD
    qtd->token = (is_in? QTD_PID_IN : QTD_PID_OUT) | QTD_ACTIVE | QTD_IOC | QTD_CERR(3) | (len << 16) | (1 << 31); // DT=1 always, QH will override if needed

    qtd->tuff[0] = (unsigned int)buf;
    // these are for BÜYÜK VERIIII big data
    qtd->tuff[1] = ((unsigned int)buf + 0x1000) & ~0xFFF;
    qtd->tuff[2] = ((unsigned int)buf + 0x2000) & ~0xFFF;

    qh->next = (unsigned int)async_head | 0x02;
    qh->epchar = (addr & 0x7F) | (ep << 8) | (2 << 12) | (0 << 14) | (512 << 16) | (2 << 28) | (1 << 14); // DTC=1: use DT from QTD
    qh->epcaps = 0;
    qh->next_qtd = (unsigned int)qtd; // YA BUNU HIÇ ANLAMIOM AW NIE UNSIGNED INT OLARAK CASTLEDIK YA AMK HIÇMI 0 IN ALTINE DÜŞMEYECEK BU DEĞER
    qh->alt_qtd = 0x01; // THOSE WHO DIEDDD ARE JUSTIFIEDDD
    qh->token = 0; //nüll
    

    // written by 52
    // funfact: i got the admin acc for 52 ;)
   async_head->next = (unsigned int)qh | 0x02;  // link QH into async list

int timeout = 8000000; // Increased timeout for USB 2.0 write operations
while ((qtd->token & QTD_ACTIVE) && --timeout) {
    // Tiny delay every 128 iterations
    if((timeout & 0x7F) == 0) {
        for (volatile int w = 0; w < 5; w++);
    }
}

for (volatile int w = 0; w < 100; w++); // Small stabilization delay

async_head->next = (unsigned int)async_head | 0x02; // unlink

// Brief wait after unlink
for (volatile int w = 0; w < 1000; w++);

if (timeout == 0) {
    print_color("msc bulk shat itself :( \n", VGA_COLOR_RED);
    z_printf("ehci_bulk timeout: addr=%d ep=%d len=%u is_in=%d token=0x%x\n", 
             addr, ep, len, is_in, qtd->token);
    return -1;
}
if (qtd->token & (1 << 7)) { // halted
    z_printf("bulk halted, token=0x%x\n", qtd->token);
    return -1;
}
// z_printf("bulk ok: token=0x%x len=%u is_in=%d buf=%x\n", qtd->token, len, is_in, (unsigned)buf);
return 0;
}

// back to being wwritten by me

int msc_send_cum(usb_device_t* dev, unsigned char* cb, unsigned char cb_len, void* data, unsigned int data_len, int is_in){

    usb_cbw_t* cbw = dma_alloc(sizeof(usb_cbw_t));
    usb_csw_t* csw = dma_alloc(sizeof(usb_csw_t));

    cbw->signature = CBW_SIGNATURE; // signutre is signature yes indeed 
    cbw->tag = dev->msc_tag++;
    cbw->transfer_len = data_len;
    cbw->flags = is_in? 0x80: 0x00;
    cbw->lun = 0; // nüll
    cbw->cb_len = cb_len; // YEEESS YESS CB_LEN EQQUALS TO CB LEN HOLYYY SHIIIITTTT
    for(int i=0;i<16;i++){
        cbw->cb[i] = (i < cb_len) ? cb[i] : 0;
    }

    // BİRİNİİİ AŞAMAĞĞĞĞĞ send cbw

    if(ehci_bulk(dev->addr, dev->bulk_out, cbw, 31, 0) != 0){ // heheğğğğğ sondan önceki ayığğğ

        print_color("birinci aşamağğğ\n", VGA_COLOR_RED);
        return -1; // fuku
    } 
    
    // Minimal delay for USB 2.0 stability
    for(volatile int t = 0; t < 1000; t++);
    
    // ikinci aşamağğ veri tükürme
if(data_len > 0){
    if(ehci_bulk(dev->addr, is_in ? dev->bulk_in : dev->bulk_out, data, data_len, is_in) !=0)
        return -1;
}

// phase 3 CSW
if(ehci_bulk(dev->addr, dev->bulk_in, csw, 13 ,1) !=0)
    return -1; // fukyu

if(csw->signature != CSW_SIGNATURE || csw->status != 0){
    z_printf("msc csw shat itself: sig=0x%x status=%d\n", csw->signature, csw->status);
    return -1;
}
// z_printf("csw ok: residue=%u\n", csw->residue);
return 0;
}


// SCSI READ (10) reads sector 1 at za given LBA so this shall spit out the first 512 bytes i guess

// Read multiple sectors from USB MSC device
int msc_read_sectors(usb_device_t* dev, unsigned int lba, void* buf, unsigned int count) {
    for (unsigned int i = 0; i < count; i++) {
        if (msc_read_sector(dev, lba + i, (char*)buf + (i * 512)) != 0) {
            return -1;
        }
    }
    return 0;
}

int msc_read_sector(usb_device_t* dev, unsigned int lba, void* buf){
    unsigned char cb[10] = {
        0x28,
        0,
        (lba >> 24) & 0xFF,
        (lba >> 16) & 0xFF,
        (lba >> 8) & 0xFF,
        lba & 0xFF,
        0,
        0, 1,
        0 
    };
    return msc_send_cum(dev, cb, 10, buf, 512, 1);
}

// SCSI WRITE 10 SECTOR AGAIINNN

int msc_write_sector(usb_device_t* dev, unsigned int lba, void* buf){
      unsigned char cb[10] = {
        0x2A,                          // WRITEEEEEEE 10
        0,
        (lba >> 24) & 0xFF,
        (lba >> 16) & 0xFF,
        (lba >> 8)  & 0xFF,
        lba & 0xFF,
        0,
        0, 1,
        0
    };
    return msc_send_cum(dev, cb, 10, buf, 512, 0);
}

// scsi inquirey drive ordami yoksa oe mi ona baxio

int msc_inquiry(usb_device_t* dev){

    unsigned char cb[6] = {0x12, 0, 0, 0, 36, 0};
    void* buf = dma_alloc(36);
    if(msc_send_cum(dev, cb, 6, buf, 36, 1) != 0)
        return -1;
    print_color("YAYAYAYAYAYAYYYY MSC IS THERE I INQUIRED IT\n", VGA_COLOR_GREEN);
    return 0;
}

// TEST UNIT READY - wait until the drive stops spinning up
static int msc_test_unit_ready(usb_device_t* dev){
    unsigned char cb[6] = {0x00, 0, 0, 0, 0, 0};
    // no data phase, just CBW + CSW
    return msc_send_cum(dev, cb, 6, 0, 0, 1);
}

// REQUEST SENSE - clear any pending error after a failed TUR
static int msc_request_sense(usb_device_t* dev){
    unsigned char cb[6] = {0x03, 0, 0, 0, 18, 0};
    void* buf = dma_alloc(18);
    return msc_send_cum(dev, cb, 6, buf, 18, 1);
}

void usbmsc_attach(usb_device_t* dev){
    print_color("I FOUND A USB MASS DEVICEEE AYAYAYAYAY IMMA ATTACH IT NOWW", VGA_COLOR_GREEN);
    z_printf("addr=%d bulk_in=%d bulk_out=%d\n", dev->addr, dev->bulk_in, dev->bulk_out);
    // sum debug
    unsigned int portstatus = echi_read(0x44 + dev->port * 4);
    z_printf("port status = 0x%x\n", portstatus);
    // time for sum of that config oi
    dev->msc_tag = 1;
    dev->driver = USB_DRIVER_MSC; // TODO: add to enum at usb_bind_driver in usb.c
				  // forgor

    if(msc_inquiry(dev) == 0){
        // spin up: retry TEST UNIT READY up to 10 times
        int ready = 0;
        for(int t = 0; t < 10; t++){
            if(msc_test_unit_ready(dev) == 0){ ready = 1; break; }
            msc_request_sense(dev); // clear the error
            for(volatile int w = 0; w < 2000000; w++); // ~200ms delay
        }
        if(!ready){
            print_color("drive never became ready :(\n", VGA_COLOR_RED);
            return;
        }
        // Mount to drive "0:" to match USB 3.0
        FRESULT res = f_mount(&usb_fs, "0:", 1);
        if(res != FR_OK)
            z_printf("fat32 sharted itself %d\n", res);
        else
            print_color("FAT32 MOUNTEDDDDDDDDD :))", VGA_COLOR_GREEN);
    }
}



/*

	AFTER THIS IS FOR XCHI DRIVERS IF SOMETHING GOES SIDEWAYS JUST COMENNT OUT AFTER TS KAY?

	ALL HAIL KUZUOS2 X64
*/

int msc_send_cum_xchi(xchi_device_t* xdev, unsigned char* cb, unsigned char cb_len, void* data, unsigned int data_len, int is_in){

    usb_cbw_t* cbw = dma_alloc(sizeof(usb_cbw_t));
    usb_csw_t* csw = dma_alloc(sizeof(usb_csw_t));

    cbw->signature = CBW_SIGNATURE;
    cbw->tag = xdev->msc_tag++;
    cbw->transfer_len = data_len;
    cbw->flags = is_in ? 0x80 : 0x00;
    cbw->lun = 0;
    cbw->cb_len = cb_len;
    for(int i = 0; i < 16; i++){
        cbw->cb[i] = (i < cb_len) ? cb[i] : 0;
    }

    // phase 1: send CBW (always OUT, always bulk_out)
    if(xchi_bulk_transfer(xdev, cbw, 31, 0) != 0){
        print_color("xchi msc: cbw phase shat itself\n", VGA_COLOR_RED);
        return -1;
    }

    // Minimal delay for USB stability
    for(volatile int t = 0; t < 1000; t++);

    // phase 2: data phase
    if(data_len > 0){
        if(xchi_bulk_transfer(xdev, data, data_len, is_in) != 0)
            return -1;
    }

    // phase 3: CSW (always IN, always bulk_in)
    if(xchi_bulk_transfer(xdev, csw, 13, 1) != 0)
        return -1;

    if(csw->signature != CSW_SIGNATURE || csw->status != 0){
        z_printf("xchi msc csw shat itself: sig=0x%x status=%d\n", csw->signature, csw->status);
        return -1;
    }
    return 0;
}

int msc_inquiry_xchi(xchi_device_t* xdev){
    unsigned char cb[6] = {0x12, 0, 0, 0, 36, 0};
    void* buf = dma_alloc(36);
    if(msc_send_cum_xchi(xdev, cb, 6, buf, 36, 1) != 0)
        return -1;
    print_color("YAYAYAYAYAYAYYYY XHCI MSC IS THERE I INQUIRED IT\n", VGA_COLOR_GREEN);
    return 0;
}

static int msc_test_unit_ready_xchi(xchi_device_t* xdev){
    unsigned char cb[6] = {0x00, 0, 0, 0, 0, 0};
    return msc_send_cum_xchi(xdev, cb, 6, 0, 0, 1);
}

static int msc_request_sense_xchi(xchi_device_t* xdev){
    unsigned char cb[6] = {0x03, 0, 0, 0, 18, 0};
    void* buf = dma_alloc(18);
    return msc_send_cum_xchi(xdev, cb, 6, buf, 18, 1);
}


void usbmsc_attach_xchi(xchi_device_t* xdev){
    print_color("I FOUND A USB3 MASS DEVICEEE AYAYAYAYAY IMMA ATTACH IT NOWW\n", VGA_COLOR_GREEN);
    z_printf("slot=%d bulk_in_dci=%d bulk_out_dci=%d\n", xdev->slot_id, xdev->bulk_in_dci, xdev->bulk_out_dci);

    if(!xdev->bulk_in_dci || !xdev->bulk_out_dci){
        print_color("xchi msc: missing bulk endpoint(s), bailing\n", VGA_COLOR_RED);
        return;
    }

    xdev->msc_tag = 1; // reuses the same field name as usb_device_t; add msc_tag to xchi_device_t if not present
    xdev->is_msc = 1; // Mark as mass storage device for boot detection

    if(msc_inquiry_xchi(xdev) == 0){
        int ready = 0;
        for(int t = 0; t < 10; t++){
            if(msc_test_unit_ready_xchi(xdev) == 0){ ready = 1; break; }
            msc_request_sense_xchi(xdev);
            for(volatile int w = 0; w < 2000000; w++);
        }
        if(!ready){
            print_color("xchi msc: drive never became ready :(\n", VGA_COLOR_RED);
            return;
        }
        
        print_color("USB 3.0 mass storage device ready for boot!\n", VGA_COLOR_LIGHT_GREEN);
        
        FRESULT res = f_mount(&usb_fs, "0:", 1);
        if(res != FR_OK)
            z_printf("fat32 sharted itself %d\n", res);
        else
            print_color("FAT32 MOUNTEDDDDDDDDD :))", VGA_COLOR_GREEN);
    }
}

// xHCI MSC sector read - used for USB 3.0 boot
int msc_read_sector_xchi(struct xchi_device_t* xdev, unsigned int lba, void* buffer){
    unsigned char cb[10] = {0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0};
    cb[2] = (lba >> 24) & 0xFF;
    cb[3] = (lba >> 16) & 0xFF;
    cb[4] = (lba >> 8) & 0xFF;
    cb[5] = lba & 0xFF;
    
    return msc_send_cum_xchi((xchi_device_t*)xdev, cb, 10, buffer, 512, 1);
}

// xHCI MSC sector write - for completeness
int msc_write_sector_xchi(struct xchi_device_t* xdev, unsigned int lba, void* buffer){
    unsigned char cb[10] = {0x2A, 0, 0, 0, 0, 0, 0, 0, 1, 0};
    cb[2] = (lba >> 24) & 0xFF;
    cb[3] = (lba >> 16) & 0xFF;
    cb[4] = (lba >> 8) & 0xFF;
    cb[5] = lba & 0xFF;
    
    return msc_send_cum_xchi((xchi_device_t*)xdev, cb, 10, buffer, 512, 0);
}
