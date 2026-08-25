// USBBB START YAYYY 
#ifndef USB_H
#define USB_H

#define USB_DRIVER_NONE      0
#define USB_DRIVER_KEYBOARD  1
#define USB_DRIVER_MSC       2 

void usb_scan(void);
unsigned int echi_read(unsigned int reg);
void echi_write(unsigned int reg, unsigned int val);
unsigned int getbar0forechi(void); // getter for bar0 for the echi usb guy to use in the kernel imma pass this to echi_read
void reset_echi(void); // resets the echi usb guy
void turnUSB(void); // getting ma boi on
void numberOfPorts(void); // print the number of em ports
void scanports(void); // scan the ports to see if there is a device connected to them and print it out
void initechi(void);
void ehci_init_async(void);
void resetport(int i);
int usbaddr(unsigned int new_addr);
int ehci_interrupt_in(unsigned int addr,
                      unsigned int endpoint,
                      void* buffer,
                      unsigned int len);


// QH ve QTD zamanı ALLAHCANIMIALSADA KURTULSAMMMMMĞĞĞĞĞĞĞ HAĞĞĞĞH

extern unsigned int echibase;
extern unsigned int caplength;
extern unsigned int opbase;

typedef struct qtd{

    volatile unsigned int next; // next qtd işaret edenipici
    volatile unsigned int alt_next; // metalci pointer
    volatile unsigned int token; 
    volatile unsigned int tuff[5]; // tuff af shi gng
} __attribute__((packed)) qtd_t;

//QH kafacıkalrı
typedef struct qh{

    volatile unsigned int next; // horxiantal
    volatile unsigned int epchar; // endpoint
    volatile unsigned int epcaps; // gene endpoint
    volatile unsigned int current; // current qtd işaretpiçi

    // qtd mirror overlap için

    volatile unsigned int next_qtd; // vertical
    volatile unsigned int alt_qtd; // vertical metalci pointer
    volatile unsigned int token; // token for the qh
    volatile unsigned int tuff[5]; // tuff af shi gng

} __attribute__((packed)) qh_t;


#define QTD_PID_OUT    (0x0 << 8)
#define QTD_PID_IN     (0x1 << 8)
#define QTD_PID_SETUP  (0x2 << 8)
#define QTD_ACTIVE       (1 << 7)
#define QTD_IOC         (1 << 15)


typedef struct {

    unsigned char bmreq; // bm request type thing
    unsigned char breq; // b request
    unsigned short wval; // w value
    unsigned short widx; // w index
    unsigned short wlen; // w length
}__attribute__((packed)) usb_setup_t;

void* dma_alloc(unsigned int size);
int ehci_control(unsigned int addr, usb_setup_t* setup, 
                          void* data, unsigned int len, int is_in);


// TIMEEE FORRRRR USB DEVICE TRREEE

typedef struct usb_device {

    int used;

    int connected;

    unsigned int port;

    unsigned int addr;

    unsigned short vendor_id;
    unsigned short product_id;

    unsigned char class;
    unsigned char subclass;
    unsigned char protocol;

    unsigned char interface_number;

    unsigned char endpoint_interrupt_in;

    unsigned short endpoint_max_packet;

    int driver;
    unsigned int usb_bus; // for lsusb app

    // for usb massive storages

    unsigned char bulk_in;
    unsigned char bulk_out;
    unsigned int msc_tag; // tag?

    // from diskio.c new version
    unsigned int msc_sectors;
    unsigned int msc_sector_size;

} usb_device_t;

#define MAX_USB_DEVICES 16

extern usb_device_t usb_devices[MAX_USB_DEVICES];

void usb_bind_driver(usb_device_t* dev);

void usb_poll(void);

int ehci_interrupt_in(
    unsigned int addr,
    unsigned int endpoint,
    void* buffer,
    unsigned int len
);

extern qh_t* async_head;
void usbmsc_attach(usb_device_t* dev);
void usb_init(void);
int msc_read_sectors(usb_device_t* dev, unsigned int lba, void* buf, unsigned int count);
// USB MASS STORAGE TIMEEEE HELL YEEE IMMA READ YO FAT16 


#define USB_CLASS_MSC 0x08 // MSG?? CHINA MENTIONED
#define USB_CLASS_MSC_SCSI 0X06 // CHINAAA AGAINNN BABYY
#define USB_CLASS_MSC_BOT 0X50 // bulk only trasnportiçin gene msg ama bot

#define CBW_SIGNATURE 0x43425355 // i dunno
#define CSW_SIGNATURE  0x53425355 // i also dunno

typedef struct __attribute__((packed)) {
    unsigned int  signature;   // imzasal
    unsigned int  tag;
    unsigned int  transfer_len;
    unsigned char flags;       
    unsigned char lun;
    unsigned char cb_len;
    unsigned char cb[16];      // SCIS UR SUMSHI
} usb_cbw_t;

typedef struct __attribute__((packed)) {
    unsigned int  signature;   // imzasal²
    unsigned int  tag;
    unsigned int  residue;
    unsigned char status;      // 0 = YEEEEE
} usb_csw_t;

// for fat32 shii
int msc_read_sector(usb_device_t* dev, unsigned int lba, void* buf);
int msc_write_sector(usb_device_t* dev, unsigned int lba, void* buf);
int msc_send_cum(usb_device_t* dev, unsigned char* cb, unsigned char cb_len, void* data, unsigned int data_len, int is_in);
#endif