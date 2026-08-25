/*/
MAIN USB DRIVER FOR KUZUOS2
written by vujuvuju for KuzuOS2 :)
*/
#include "vga.h" // include vga for print n shi since ts will run at ring 0
#include "usb.h" // ofc usb.h duhh
#include "z_utils.h" // for za printf for bar0 and other ints
#include "keyboardusb.h" // usb keyboard için yeni yazdığımız seksçuvalı//
#include "keymap_loader.h" // for load_us_keymap
#include "lsusb.h" // for lsusb app to work and display the bus number yayyy
#include "xchi.h" // YAYYYYYY SUPPORTED NOW

#define PCI_CONFIG_ADDR  0xCF8 // no idea
#define PCI_CONFIG_DATA  0xCFC // still no idea
usb_device_t* usb_alloc_device(void); // just a definition to not fuk shi
#define USB_CMD 0X00 // for reset
#define USB_STS 0X04 // for checking if reset is done
#define DMA_BASE 0x200000
unsigned int  echibase; // to hold the adress from bar0
static unsigned int next_usb_addr = 1; // needs to be here because when its at bottom C compiler moans like a bottom 
unsigned int caplength; // just init çünkü şuanda echibase halen dutluk 
static int ehci_found = 0;
unsigned int opbase; //hem echibase hem caplength hala dutluk
void usbconf(unsigned int addr);
unsigned int ports = 0;
unsigned int usb_bus = 0; // for lsusb app

static inline void outl(unsigned short port, unsigned int val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port)); // sooo does something? 
}

static inline unsigned int inl(unsigned short port) {
    unsigned int ret; // return value holdah
    
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port)); // so does this? 
    return ret;
}

static unsigned int ishallreadpcie(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (1 << 31) | ((unsigned int)bus << 16) | ((unsigned int)slot << 11) | ((unsigned int)func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, address); // outl ligmaballs
    return inl(PCI_CONFIG_DATA); // returns the pci data i think
}


inline void initechi(void){
    caplength = *((volatile unsigned char*)echibase); // şuan artık echibase dutluk değil ondan caplengthi initledik
    opbase = echibase + caplength; // ame thing


}

void usb_scan(void)// scans usb 
{

    int found = 0; // checks if a usb is found so that we can display a non found message without FLOODING THE GODDAMN SCREEN
    for(uint16_t bus = 0; bus < 256; bus++) {

        for(unsigned char slot = 0; slot < 32; slot++) {
            for(unsigned int func = 0; func < 8; func++){ // func so that we scan everrryyythinggggggg yeeee
            unsigned int classinfo = ishallreadpcie(bus, slot, func, 0x08); // pci classdan okuyo işte SEKSI MI SEKSI METHODUMLA HEHEHEHE
            unsigned char class = (classinfo >> 24) & 0xFF; // class infoyu classa çeviriyor
            unsigned char subclass = (classinfo >> 16) & 0xFF; // subclassa abomine oldu
            unsigned char prog_if = (classinfo >> 8) & 0xFF; // program interface i dont know what it is but its there and yeaaaaa

            if(class == 0x0C && subclass == 0x03){

                print_color("I FOUND A USBBB CONTROLLERRRRR YAYYAYYAYAYAYAYAYAY\n", VGA_COLOR_LIGHT_GREEN); // found a usb controller yayyy
                found++;
                unsigned int bar0 = ishallreadpcie(bus, slot, func, 0x10); // find bar0 with my hot af method and my even hotter func 
                bar0 &= ~0xF;// idk garbage colection i think
                echibase = bar0; 
                usb_bus = bus; // indeed for the lsusb to display it 
                z_printf("BAR0 = 0x%x\n", echibase); // we print the bar0 value for me lol
                z_printf("bus = %d\n", bus); 
                if(prog_if == 0x00){

                print_color("i found eine kleine UHCI controller\n", VGA_COLOR_LIGHT_GREEN); // found a uhci controller yayyy
            
                } else if(prog_if == 0x20){
		
                print_color("i found eine kleine EHCI controller\n", VGA_COLOR_LIGHT_GREEN); // found a ehci controller yayyy
                ehci_found = 1; // ts exists so that i dont reset thy ehci unless i do have a bloody ehci
                } else if(prog_if == 0x30){

                print_color("i found eine kleine XHCI controller\n", VGA_COLOR_LIGHT_GREEN); // found a xhci controller yayyy
		xchi_init(bus, slot, func);
		// I SUPPORT XCHI????? I DIDNT KNOW TS BUT SUREEEEEEEE I SUPPOSE
               }
               
            }
            }     
            }
        }    
 
    if(found == 0){ // keine eine usb :(
        print_color("NO USBBB CONTROLLER FOUND :((( \n", VGA_COLOR_LIGHT_RED);
    
    } 

}

inline unsigned int echi_read(unsigned int reg){ // for reading the echi usb guy
        unsigned int val = *((volatile unsigned int*)(opbase + reg)); // reads the register n the offset
       // z_printf("Read 0x%x from register 0x%x\n", val, reg); // debug print for read value and register

        return val; // returns the value
    }

inline void echi_write(unsigned int reg, unsigned int val){ // for writing to the echi usb guy
 
    *((volatile unsigned int*)(opbase + reg)) = val; // writes the value to the register n the ofset

}

inline unsigned int getbar0forechi(void){ // getter for bar0 for the echi usb guy to use in the kernel imma pass this to echi_read
    return echibase; // returns the bar0 value for the echi usb guy
}
inline void reset_echi(void){ // resets the echi usb guy
    unsigned int cum = echi_read(USB_CMD); // read usb cmd and save it to CUMM
    cum |= (1 <<1); // set za HCRESET bit
    echi_write(USB_CMD, cum); // reseti command registerine yaziore
    unsigned int wait = 100000;
     while(echi_read(USB_CMD) & (1 << 1) && wait--); // HEYYY WAITTT I GOT A NEW COMPLAINTTT
    if(wait == 0){
        print_color("gng i just timedout :()", VGA_COLOR_LIGHT_RED);
    }
    else{
     print_color("i just reseted thy echi", VGA_COLOR_GREEN); // i think it explains itself lol
    }

    }      

inline void turnUSB(void){ // getting ma boi on 

    echi_write(USB_CMD, 0x00000001); // turns the usb on :)
}

inline void turnOffUSB(void){ // turns the usb off :(

    echi_write(USB_CMD, 0x00000000); // turns the usb off :(
}

inline void numberOfPorts(void){
    unsigned int cap = echi_read(0x00); // read the capabilities register to get the number of ports
    unsigned int caplen  = cap & 0xFF; // get the length of sumshit
    unsigned int op_base = echibase + caplen; // operation echiii lovess to love the ports of an U.S.B whoree
    unsigned int hccparams = echi_read(0x04); // im not sure if its 0x04 or 0x08 just try em both 
    ports = hccparams & 0x0F; // port sayısını alıyo
    // z_printf("Number of USB ports: %d\n", ports); // print the number of ports for debug
}

inline void resetport(int i){

    unsigned int reg = 0x44 +i * 4; // port registeri
    unsigned int port = echi_read(reg);


    echi_write(reg, port | 1  << 8);
    
    for(volatile int t = 0; t < 1000000; t++); // delay 


    echi_write(reg, echi_read(reg) & ~(1 << 8)); // reseti çak papiye
    while(echi_read(reg) & (1 << 8));  // fahhh

    echi_write(reg, echi_read(reg) | (1 << 1)); // resetledik şimdi portu geri açıoz
}

/*this guy is written by gpt
but dw its a replacment for the big hardcoded guy at kernel
*/

int usb_enumerate_device(usb_device_t* dev)
{
    unsigned char* desc =
        (unsigned char*)dma_alloc(18);

    usb_setup_t* setup =
        (usb_setup_t*)dma_alloc(sizeof(usb_setup_t));

    setup->bmreq = 0x80;
    setup->breq  = 0x06;
    setup->wval  = 0x0100;
    setup->widx  = 0;
    setup->wlen  = 18;

    if(ehci_control(0, setup, desc, 18, 1) != 0)
    {
        return -1;
    }

    dev->vendor_id =
        *(unsigned short*)(desc + 8);

    dev->product_id =
        *(unsigned short*)(desc + 10);

    dev->addr = next_usb_addr++;

    usbaddr(dev->addr);


    z_printf(
        "USB DEVICE addr=%d vid=0x%x pid=0x%x\n",
        dev->addr,
        dev->vendor_id,
        dev->product_id
    );

    return 0;
}

/*writeen by claude papi*/

void usb_parse_endpoints(usb_device_t* dev) {
    unsigned char* conf = (unsigned char*)dma_alloc(255);
    usb_setup_t* setup = (usb_setup_t*)dma_alloc(sizeof(usb_setup_t));
    setup->bmreq = 0x80;
    setup->breq  = 0x06;
    setup->wval  = 0x0200;
    setup->widx  = 0;
    setup->wlen  = 255;

    if(ehci_control(dev->addr, setup, conf, 255, 1) == 0) {
        int i = 0;
        while(i < 255) {
            unsigned char len  = conf[i];
            unsigned char type = conf[i+1];
            if(len == 0) break;

            if(type == 0x04) {
                dev->class    = conf[i+5];
                dev->subclass = conf[i+6];
                dev->protocol = conf[i+7];
            }
            if(type == 0x05) {
                unsigned char ep_addr = conf[i+2];
                unsigned char attr    = conf[i+3];
                if((attr & 0x03) == 0x02) {
                    if(ep_addr & 0x80)
                        dev->bulk_in  = ep_addr & 0x0F;
                    else
                        dev->bulk_out = ep_addr & 0x0F;
                }
            }
            i += len;
        }
        z_printf("class=%d bulk_in=%d bulk_out=%d\n",
                 dev->class, dev->bulk_in, dev->bulk_out);
    }
}

inline void scanports(void){

    for(int i = 0; i < ports; i++){

        unsigned int port = echi_read(0x44 + i * 4);
        z_printf("Port %d = 0x%x\n", i, port);

        if(port & 1){

            usb_device_t* dev = usb_alloc_device();
            dev->port = i; // INSIDE THE LOOP
            dev->usb_bus = usb_bus;  
            if(!dev){
                z_printf("no free usb for u big papi :(");
                continue;
            }
            z_printf("Device connected to port %d\n", i);
            resetport(i); // çak resedi
            unsigned int status = echi_read(0x44 +i * 4); // portun durumu
            if(usb_enumerate_device(dev) != 0)
                {
                    z_printf("enumeration exploded :(\n");
                    dev->used = 0;
                    continue;
                }
            usb_parse_endpoints(dev); // yayy
            
            usbconf(dev->addr);

            if(dev->class == 3){
                print_color("its a keyboarddd", VGA_COLOR_GREEN);
                usbkeyboard(dev);
            }
            // deleteded tje usb keyboard from here and into the if because the fuckass MSD wew getting a keyboard n shi like total shitshow on wheels
            dev->connected = 1;
            usb_bind_driver(dev);
            /*    
            if(status & (1 << 2)){

                z_printf("its higher then ozzy at 92\n"); // if its higher then ozzy at 92 print it for gods sake
             }
             else{

                z_printf("its not higher then ozzy at 92\n"); // if its lower then ozzy at 92
             }
                */
             }
            
        }
}


static unsigned int dma_ptr = DMA_BASE;

void* dma_alloc(unsigned int size){

    dma_ptr = (dma_ptr + 31) & ~31; // HEHEHEHHEE 31
    void* adres = (void*)dma_ptr;
    dma_ptr += size;
    return adres;
}

// dummy QH için
qh_t* async_head = 0; // AW BU NIE STATIC
void ehci_init_async(void){

    async_head = (qh_t*)dma_alloc(sizeof(qh_t));
    unsigned char* p = (unsigned char*)async_head;
    for(unsigned int i = 0; i < sizeof(qh_t); i++) p[i] = 0;

    // point to thyself 

    async_head->next = (unsigned int)async_head | (1 << 1); // tip qh eşit
    async_head->epchar = (1 << 15); // h biti reklamasyonun kafaya eşit
    async_head->epcaps = 0;
    async_head->next_qtd = 0x01; // terminasyon
    async_head->alt_qtd = 0x01;
    async_head->token = 0;

    echi_write(0x18, (unsigned int)(async_head));

    unsigned int cmd = echi_read(USB_CMD);
    cmd |= (1 << 5);
    echi_write(USB_CMD, cmd);
    while(!(echi_read(USB_STS) & (1 << 15)));
}


int ehci_control(unsigned int addr, usb_setup_t* setup, void* data, unsigned int len, int is_in ){

    // qtd ve qh leri dmadan aloke ediorez çekkk

    qtd_t* setup_qtd = (qtd_t*)dma_alloc(sizeof(qtd_t)); // setup için bir qtd alıyoruz
    qtd_t* data_qtd = (qtd_t*)dma_alloc(sizeof(qtd_t)); // data için bir qtd alıyoruz
    qtd_t* status_qtd = (qtd_t*)dma_alloc(sizeof(qtd_t));
    qh_t* qh         = (qh_t*)dma_alloc(sizeof(qh_t)); // AGAINNNNNNNNN DIN DIN DIN AGAINNNNN DIN DIN DINNNN AGAINNNN ~craft 
    
    // herşeyi zerola

    unsigned char* p;

    p = (unsigned char*)setup_qtd;
    for(int i = 0;i<sizeof(qtd_t);i++) p[i]=0;
    p = (unsigned char*)data_qtd;
    for(int i = 0;i<sizeof(qtd_t);i++) p[i]=0;
    p = (unsigned char*)status_qtd;
    for(int i = 0;i<sizeof(qtd_t);i++) p[i]=0;

    p = (unsigned char*)qh;
    for(int i = 0;i<sizeof(qh_t);i++) p[i]=0;
    // setup for qtd


    #define QTD_CERR(x) ((x) << 10)
    // setup_qtd->next = (unsigned int)data_qtd;
    setup_qtd->alt_next = 0x1; // terminasyonnn
    setup_qtd->token = QTD_PID_SETUP | QTD_ACTIVE | QTD_IOC | QTD_CERR(3) | (8 << 16); // setup pid ve active ve interrupt on completion
    setup_qtd->tuff[0] = (unsigned int)setup; // setup



    // QTD için veriisell palantir timeee babyyy

    if(len > 0){

        setup_qtd->next = (unsigned int)data_qtd; // setup qtd yi data qtdye pointle
        data_qtd->next = (unsigned int)status_qtd; // datayıda statuse pointle (bune amk ottoman circlejerk olduk berber aşçıya aşçı uşağa uşak berbe aw)
        data_qtd->alt_next = 0x1; // terminasyon
        data_qtd->token = (is_in ? QTD_PID_IN : QTD_PID_OUT) | QTD_ACTIVE | QTD_IOC | (len << 16) | QTD_CERR(3) ; // data parmaklamaca elleşmece numero uno :)))
        // data_qtd->token |= (1 << 31);
        data_qtd->tuff[0] = (unsigned int)data; // data bufferı
    } else {

        setup_qtd->next =  (unsigned int)status_qtd; // ANAYIBEKLIOYRUMAYOL
        // data_qtd->token = 0; // s(i)kip
    }

    //statıs qtd
    status_qtd->next = 0x1; // terminasyon
    status_qtd->alt_next = 0x1; // terminasyon²
    status_qtd->token = QTD_ACTIVE | (is_in ? QTD_PID_OUT : QTD_PID_IN) | QTD_IOC | QTD_CERR(3); // status pid ve active ve interrupt on completion
    
    /*
    // Qtd için statüs falan fistik cumburlop

    status_qtd->next = 0x1; // terminasyon
    status_qtd->alt_next = 0x1; // terminasyon²
    status_qtd->token = QTD_ACTIVE | (is_in ? QTD_PID_OUT : QTD_PID_IN) | QTD_IOC | QTD_CERR(3); // status pid ve active ve interrupt on completion
    */


    //QH TIMEEEE YEEEE

    qh->next = 0x1; // terminasyon
    qh->epchar = (addr & 0x7F) // adressel
                | (0 << 8)
                | (0 << 12) // eğer birşey bozulursa bu 0 ı 2 ile değiştir şuan sıfır olmasının sebebihigh speed olması ve statusun 0x1005 olması yani HIGHH AFF
                | (1 << 14) // DTC için 
                | (64 << 16);
                // NOO MOREE REKLAMASYONN NO MOREEE PAINNNNN DUMDUMDUMMMM crowbar refferance

    qh->epcaps = (1 << 30) | (1 << 23) | (1 << 16); // sokarım microframine al sana hız
    qh->current = 0; // current qtd başlangıçta sıfır
    qh->next_qtd = (unsigned int)setup_qtd; 
    qh->alt_qtd = 0x1; // terminasyon
    qh->token = 0; // normalde buraya endpoint info falan gelicek ama ben boş bırakıyorum şimdilik


    // qhyi kendine pointle 
    qh->next = (unsigned int)async_head | 0x02; // qhye selfuk at
    async_head->next = (unsigned int)qh | (1<<1); // qhyi asıl queueye atıyoz
    
    // debug valtiğğğ

    z_printf("ASYNCLISTADDR: 0x%x\n", echi_read(0x18));
    z_printf("qh epchar: 0x%x next_qtd: 0x%x\n", qh->epchar, qh->next_qtd);
    z_printf("setup token: 0x%x buf: 0x%x\n", setup_qtd->token, setup_qtd->tuff[0]);

    //async takvimi yee
    /*
    unsigned int cum2 = echi_read(USB_CMD); // komut registerini oku
    cum2 |= (1 << 5); // async schedule enable bitini set et
    echi_write(USB_CMD, cum2); // komut registerine yaz

    // GENE BEKLIOZ AW
    while(!(echi_read(USB_STS) & (1 << 15)));
    z_printf("ASYNC ISSS ALIVEEEEE EHHHHHHHĞĞĞĞĞĞĞ \n");
*/
    int timeout = 1000000; // timeout değeri, yeterince büyük bir sayı seçtim ama çok büyük olmasın diye
    while(((*(volatile unsigned int*)&status_qtd->token) & QTD_ACTIVE) && --timeout);
    // more debug

    z_printf("setup token after:  0x%x\n", setup_qtd->token);
    z_printf("data token after:   0x%x\n", data_qtd->token);
    z_printf("status token after: 0x%x\n", status_qtd->token);
    z_printf("USBSTS: 0x%x\n", echi_read(USB_STS));
    
    // asyncin içine et
    /*
    cum2 = echi_read(USB_CMD); // READDD
    cum2 &= ~(1 << 5); // async schedule disable
    echi_write(USB_CMD, cum2); // WRITEEEE
    while(echi_read(USB_STS) & (1 << 15)); // bi bekle abim otur çay kahve 
    */
    // yok sana tineout
    
    async_head->next = (unsigned int)async_head | 0x2;
     return 0; // jahoy
    
    }


int usbaddr(unsigned int new_addr){

    usb_setup_t* setup = (usb_setup_t*)dma_alloc(sizeof(usb_setup_t));

    setup->bmreq = 0x00; // FROMM MY HOSTTT TO DEVICEEE IIII LIKEEEE UUUUUUUU (UUUUUU(UUUUUU))
    setup->breq = 0x05; // new addr yknowww
    setup->wval = new_addr;
    setup->widx = 0;
    setup->wlen = 0;
    ehci_control(0, setup, 0, 0, 0); // ignore the return

    z_printf("şuan umarim cihazde bu adres var %d\n", new_addr);
    return 0;
}


// HEYYY WAITT I GOT A NEW EHCIII (KEYBOARD CONF TIMEE BABYYYYYY)

int ehci_interrupt_in(unsigned int addr,
                      unsigned int endpoint,
                      void* buffer,
                      unsigned int len){

                        qtd_t* qtd = dma_alloc(sizeof(qtd_t));
                        qh_t* qh = dma_alloc(sizeof(qh_t));

                        unsigned char* p;
                        p = (unsigned char*)qtd;
                        for(int i = 0;i<sizeof(qtd_t);i++) p[i]=0;

                        p = (unsigned char*)qh;
                        for(int i = 0; i < sizeof(qh_t);i++) p[i]=0;

                        qtd->next = 0x01;
                        qtd->alt_next = 0x01;

                        qtd->token = QTD_PID_IN | QTD_ACTIVE | (len << 16) | QTD_CERR(3);

                        qtd->tuff[0] = (unsigned int)buffer;

                        qh->next = (unsigned int)async_head | 0x02;

                        qh->epchar = (addr & 0x7F) | (endpoint << 8) | (2 << 12) | (1 << 14) | (8 << 16);

                        qh->epcaps = 0;
                        qh->current = 0;
                        qh->next_qtd = (unsigned int)qtd;
                        qh->alt_qtd = 0x1;
                        qh->token = 0;

                        async_head->next = (unsigned int)qh | 0x02;

                        int timeout = 10000000;

                        while((qtd->token & QTD_ACTIVE) && --timeout);

                        // z_printf("interrupt token: 0x%x\n", qtd->token);

                        if(timeout == 0){
                            return -1;
                        }

                        return 0;
                      }

usb_device_t usb_devices[MAX_USB_DEVICES];

usb_device_t* usb_alloc_device(void){

    for(int i = 0;i<MAX_USB_DEVICES;i++){

        if(!usb_devices[i].used){
            usb_devices[i].used = 1;
            return &usb_devices[i];
        }
    }
    return 0;
}


void usb_bind_driver(usb_device_t* dev)
{
    if(dev->class == 3)
    {
        usbkbd_attach(dev);
        return;
    }
    if(dev->class == USB_CLASS_MSC){
        usbmsc_attach(dev);
        return;
    }

}



void usb_poll(void)
{
    for(int i=0;i<MAX_USB_DEVICES;i++)
    {
        if(!usb_devices[i].used)
            continue;

        if(!usb_devices[i].connected)
            continue;

        switch(usb_devices[i].driver)
        {
            case USB_DRIVER_KEYBOARD:
                usbkbd_poll_device(
                    &usb_devices[i]
                );
                break;
            case USB_DRIVER_MSC:
                // buralar halen dutluk yeğenim
                break;
        }
    }
}

// main usb init fucntions MUST ALWAYS BE AT THE BOTTTOM


void usb_init(void)
{
    usb_scan();

    if(ehci_found){
    initechi();

    reset_echi();

    turnUSB();

    ehci_init_async();

    numberOfPorts();

    scanports();
    }
}
