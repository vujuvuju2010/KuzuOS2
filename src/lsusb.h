#ifndef LSUSB_H
#define LSUSB_H

typedef struct usb_lsusb_entry {
    unsigned int   bus;
    unsigned int   addr;
    unsigned short vendor_id;
    unsigned short product_id;
    unsigned char  class;
    unsigned char  subclass;
    unsigned char  port;
    unsigned int usb_bus;

} usb_lsusb_entry_t;

#endif // LSUSB_H