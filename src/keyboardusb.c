#include "vga.h" // for print_color n sshii
#include "usb.h" // for usb 
#include "z_utils.h" // for prinf and other z_ things
#include "keyboardusb.h" // yee
#include "keyboard.h" // for consts
#include "keymap_loader.h" // for keymaps

extern char keyboard_buffer[];
extern int buffer_head;
extern int buffer_tail;
/*
void usbkeyboard(void){

    usb_setup_t* setup = (usb_setup_t*)dma_alloc(sizeof(usb_setup_t));

    unsigned char* tuff = (unsigned char*)dma_alloc(9);



    // WE SHALL GET ZA DESCRIPTOR(CONFIGURATION)
    // dont mind these numbers they are required idk what they are either thee shall ask chatgpt
    setup->bmreq = 0x80;
    setup->breq = 0x06;
    setup->wval = 0x0200;
    setup->widx = 0;
    setup->wlen = 9;

    int r = ehci_control(1, setup, tuff, 9, 1);

    if(r != 0){

        print_color("i just sharted :(", VGA_COLOR_RED);
        return;
    }

    print_color("first 9 bytes are these nüttesacks", VGA_COLOR_GREEN);

    for(int i=0;i<9;i++){

        z_printf("0x%x ", tuff[i]);
    }
    z_printf("\n");

    unsigned short totallen = tuff[2] | (tuff[3] << 8);
    z_printf("total len = %d\n", totallen);

    unsigned char* full = (unsigned char*)dma_alloc(totallen);

    // AGAINNN

    setup->wlen = totallen;

    r = ehci_control(1, setup, full, totallen, 1);

    if(r!=0){

        print_color("ur fuked", VGA_COLOR_LIGHT_RED);
    }
    */
int usbkeyboard(usb_device_t* dev)
{
    usb_setup_t* setup =
        dma_alloc(sizeof(usb_setup_t));

    unsigned char* cfg9 =
        dma_alloc(9);

    setup->bmreq = 0x80;
    setup->breq  = 0x06;
    setup->wval  = 0x0200;
    setup->widx  = 0;
    setup->wlen  = 9;

    if(ehci_control(dev->addr, setup, cfg9, 9, 1) != 0)
    {
        z_printf("cfg9 failed\n");
        return -1;
    }

    unsigned short total =
        cfg9[2] | (cfg9[3] << 8);

    unsigned char* full =
        dma_alloc(total);

    setup->wlen = total;

    if(ehci_control(dev->addr, setup, full, total, 1) != 0)
    {
        z_printf("full cfg failed\n");
        return -1;
    }

    int off = 0;

    while(off < total)
    {
        unsigned char len  = full[off];
        unsigned char type = full[off + 1];

        if(len == 0)
            break;

        // interface descriptor
        if(type == 4)
        {
            dev->interface_number = full[off + 2];

            dev->class    = full[off + 5];
            dev->subclass = full[off + 6];
            dev->protocol = full[off + 7];

            z_printf(
                "IF class=%x subclass=%x proto=%x\n",
                dev->class,
                dev->subclass,
                dev->protocol
            );
        }

        // endpoint descriptor
        if(type == 5)
        {
            unsigned char ep =
                full[off + 2];

            unsigned char attr =
                full[off + 3];

            unsigned short maxpkt =
                full[off + 4]
                |
                (full[off + 5] << 8);

            // interrupt IN endpoint
            if((ep & 0x80) && ((attr & 3) == 3))
            {
                dev->endpoint_interrupt_in =
                    ep & 0x0F;

                dev->endpoint_max_packet =
                    maxpkt;

                z_printf(
                    "INT EP=%d max=%d\n",
                    dev->endpoint_interrupt_in,
                    dev->endpoint_max_packet
                );
            }
        }

        off += len;
    }

    return 0;
}



void usbconf(unsigned int addr){

    usb_setup_t* s = dma_alloc(sizeof(usb_setup_t));
    
    // these are the USB keyboard config paramteters which can be readed in detail at USB spec 2 page around 100 to 200 after the mechanival and electrical standarts 
    s->bmreq = 0x00; // BM REQUEST TYPE 
    s->breq = 0x09; // dokuzzz
    s->wval = 1;
    s->widx = 0; // sıfıııırrr
    s->wlen = 0; // GENEEE SIFIRRR

    ehci_control(addr, s, 0, 0, 0);

    print_color("YO USB IS READY TWINNNNN", VGA_COLOR_GREEN);
}


// USB HID usage -> Linux keycode
static const uint8_t hid_to_keycode[256] = {
    [0x04] = 30, [0x05] = 48, [0x06] = 46, [0x07] = 32, [0x08] = 18,
    [0x09] = 33, [0x0A] = 34, [0x0B] = 35, [0x0C] = 23, [0x0D] = 36,
    [0x0E] = 37, [0x0F] = 38, [0x10] = 50, [0x11] = 49, [0x12] = 24,
    [0x13] = 25, [0x14] = 16, [0x15] = 19, [0x16] = 31, [0x17] = 20,
    [0x18] = 22, [0x19] = 47, [0x1A] = 17, [0x1B] = 45, [0x1C] = 21,
    [0x1D] = 44, [0x1E] = 2,  [0x1F] = 3,  [0x20] = 4,  [0x21] = 5,
    [0x22] = 6,  [0x23] = 7,  [0x24] = 8,  [0x25] = 9,  [0x26] = 10,
    [0x27] = 11, [0x28] = 28, [0x29] = 1,  [0x2A] = 14, [0x2B] = 15,
    [0x2C] = 57, [0x2D] = 12, [0x2E] = 13, [0x2F] = 26, [0x30] = 27,
    [0x31] = 43, [0x32] = 86, [0x33] = 39, [0x34] = 40, [0x35] = 41,
    [0x36] = 51, [0x37] = 52, [0x38] = 53, [0x39] = 58, [0x3A] = 59,
    [0x3B] = 60, [0x3C] = 61, [0x3D] = 62, [0x3E] = 63, [0x3F] = 64,
    [0x40] = 65, [0x41] = 66, [0x42] = 67, [0x43] = 68, [0x44] = 87,
    [0x45] = 88, [0x46] = 99, [0x47] = 70, [0x48] = 119, [0x49] = 110,
    [0x4A] = 102, [0x4B] = 104, [0x4C] = 111, [0x4D] = 106, [0x4E] = 109,
    [0x4F] = 103, [0x50] = 108, [0x51] = 105, [0x52] = 107, [0x53] = 69,
    [0x54] = 98, [0x55] = 55, [0x56] = 74, [0x57] = 78, [0x58] = 96,
    [0x59] = 79, [0x5A] = 80, [0x5B] = 81, [0x5C] = 75, [0x5D] = 76,
    [0x5E] = 77, [0x5F] = 71, [0x60] = 72, [0x61] = 73, [0x62] = 82,
    [0x63] = 83, [0x64] = 99, [0x65] = 78, [0x66] = 74, [0x67] = 77,
    [0x68] = 71, [0x69] = 75, [0x6A] = 80, [0x6B] = 73, [0x6C] = 82,
    [0x6D] = 83, [0x6E] = 79, [0x6F] = 81, [0x70] = 96, [0x71] = 76,
    [0x72] = 72, [0x73] = 70, [0xE0] = 29, [0xE1] = 42, [0xE2] = 56,
    [0xE3] = 125, [0xE4] = 97, [0xE5] = 54, [0xE6] = 100, [0xE7] = 126,
};

char usbkbd_convert(unsigned char hid, unsigned char modifiers)
{
    uint8_t keycode = hid_to_keycode[hid];
    if (keycode == 0) return 0;

    int shift = (modifiers & 0x22) != 0;
    int ctrl = (modifiers & 0x11) != 0;
    int alt = (modifiers & 0x04) != 0;
    int altgr = (modifiers & 0x40) != 0;

    uint16_t c = keymap_get_char(keycode, shift, ctrl, alt, altgr);

    return (c > 0 && c < 256) ? (char)c : 0;
}


// bu gpt papiden hemde 55 :)
void usbkbd_push_char(char c)
{
    if(!c) return;

    int next =
        (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;

    if(next != buffer_tail)
    {
        keyboard_buffer[buffer_head] = c;
        buffer_head = next;
    }
}

void usbkbd_poll(void)
{
    static unsigned char report[8];
    static unsigned char old_key = 0;

    for(int i=0;i<8;i++)
        report[i] = 0;

    int r = ehci_interrupt_in(1, 1, report, 8);

    if(r != 0)
        return;

    unsigned char key = report[2];

    // no key pressed
    if(key == 0)
    {
        old_key = 0;
        return;
    }

    // prevent key repeat spam
    if(key == old_key)
        return;

    old_key = key;

    // Check for Ctrl+Z (stop process) and Ctrl+C (interrupt) before character conversion
    unsigned char modifiers = report[0];
    int ctrl = (modifiers & 0x11) != 0;  // Left or Right Ctrl
    
    // USB HID: 0x1D = Z key, 0x06 = C key
    if (ctrl && key == 0x1D) {  // Ctrl+Z
        extern volatile int ctrl_z_pressed;
        ctrl_z_pressed = 1;
        return;
    }
    
    if (ctrl && key == 0x06) {  // Ctrl+C
        usbkbd_push_char(3);  // ETX character
        return;
    }

    char c = usbkbd_convert(key, report[0]);

    if(c)
    {
        usbkbd_push_char(c); // delete if needed
    }
}

void usbkbd_attach(
    usb_device_t* dev
)
{
    dev->driver = USB_DRIVER_KEYBOARD; // attach keyboard
}


void usbkbd_poll_device(usb_device_t* dev)
{
    static unsigned char report[8];
    static unsigned char old_key = 0;

    for(int i = 0; i < 8; i++)
        report[i] = 0;

    int r = ehci_interrupt_in(
        dev->addr,
        dev->endpoint_interrupt_in,
        report,
        8
    );

    if(r != 0)
        return;

    unsigned char key = report[2];

    if(key == 0)
    {
        old_key = 0;
        return;
    }

    if(key == old_key)
        return;

    old_key = key;

    // Check for Ctrl+Z (stop process) and Ctrl+C (interrupt) before character conversion
    unsigned char modifiers = report[0];
    int ctrl = (modifiers & 0x11) != 0;  // Left or Right Ctrl
    
    // USB HID: 0x1D = Z key, 0x06 = C key
    if (ctrl && key == 0x1D) {  // Ctrl+Z
        extern volatile int ctrl_z_pressed;
        ctrl_z_pressed = 1;
        return;
    }
    
    if (ctrl && key == 0x06) {  // Ctrl+C
        usbkbd_push_char(3);  // ETX character
        return;
    }

    char c =
        usbkbd_convert(
            key,
            report[0]
        );

    if(c)
    {
        usbkbd_push_char(c);
    }
}