#ifndef KEYBOARDUSB_H
#define KEYBOARDUSB_H

#include "usb.h"

int usbkeyboard(usb_device_t* dev);

void usbkbd_attach(usb_device_t* dev);

void usbkbd_poll(void);

void usbkbd_poll_device(usb_device_t* dev);

char usbkbd_convert(
    unsigned char hid,
    unsigned char modifiers
);

void load_us_keymap(void);

#endif