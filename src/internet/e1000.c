// KuzuOS2 e1000c drivers 
// based on the ChickenOS e1000 driver 
// which can be found at: https://github.com/blanham/ChickenOS/blob/master/src/device/net/e1000.c

#include "e1000.h"
#include "net.h"
#include "../memory.h"
#include "../vga.h"

// ---- PCI access via port I/O ----
#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) |
                    ((uint32_t)slot << 11) | ((uint32_t)func << 8) |
                    (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    return inl(PCI_CONFIG_DATA);
}

static void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) |
                    ((uint32_t)slot << 11) | ((uint32_t)func << 8) |
                    (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    outl(PCI_CONFIG_DATA, val);
}

// ---- MMIO helpers ----
static uint32_t e1000_mmio_base = 0;
static int e1000_initialized = 0; // Track if NIC is actually present

static inline uint32_t e1000_read(uint32_t reg) {
    return *((volatile uint32_t*)(e1000_mmio_base + reg));
}
static inline void e1000_write(uint32_t reg, uint32_t val) {
    *((volatile uint32_t*)(e1000_mmio_base + reg)) = val;
}

// ---- Descriptor rings ----
static e1000_rx_desc_t rx_descs[E1000_NUM_RX_DESC] __attribute__((aligned(16)));
static e1000_tx_desc_t tx_descs[E1000_NUM_TX_DESC] __attribute__((aligned(16)));

static uint8_t rx_buffers[E1000_NUM_RX_DESC][E1000_BUFFER_SIZE];
static uint8_t tx_buffers[E1000_NUM_TX_DESC][E1000_BUFFER_SIZE];

static uint32_t rx_tail = 0;
static uint32_t tx_tail = 0;

// ---- EEPROM read ----
static uint16_t e1000_eeprom_read(uint8_t addr) {
    e1000_write(E1000_REG_EERD, 1 | ((uint32_t)addr << 8));
    uint32_t val = 0;
    int timeout = 10000;
    while (timeout--) {
        val = e1000_read(E1000_REG_EERD);
        if (val & (1 << 4)) break;
    }
    if (!(val & (1 << 4))) {
        e1000_write(E1000_REG_EERD, 1 | ((uint32_t)addr << 2));
        timeout = 10000;
        while (timeout--) {
            val = e1000_read(E1000_REG_EERD);
            if (val & (1 << 1)) break;
        }
    }
    return (uint16_t)(val >> 16);
}

// ---- PCI scan ----
static int e1000_pci_find(uint8_t* out_bus, uint8_t* out_slot) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t id = pci_read((uint8_t)bus, slot, 0, 0);
            uint16_t vendor = id & 0xFFFF;
            uint16_t device = id >> 16;
            if (vendor == E1000_VENDOR_ID && device == E1000_DEVICE_ID) {
                *out_bus  = (uint8_t)bus;
                *out_slot = slot;
                return 1;
            }
        }
    }
    return 0;
}

// ---- Init ----
int e1000_init(void) {
    uint8_t bus, slot;
    if (!e1000_pci_find(&bus, &slot)) {
        print_color("[e1000] NIC not found on PCI bus\n", VGA_COLOR_LIGHT_RED);
        e1000_initialized = 0;
        return -1;
    }
    print_color("[e1000] Found Intel 82540EM NIC\n", VGA_COLOR_LIGHT_GREEN);

    uint32_t cmd = pci_read(bus, slot, 0, 0x04);
    cmd |= (1 << 1) | (1 << 2);
    pci_write(bus, slot, 0, 0x04, cmd);
    uint32_t bar0 = pci_read(bus, slot, 0, 0x10);
    e1000_mmio_base = bar0 & ~0xF;
    print_color("[E1000] BAR0 read\n", VGA_COLOR_LIGHT_GREEN);
    uint32_t st = e1000_read(E1000_REG_STATUS);
    uint32_t ctrl = e1000_read(E1000_REG_CTRL);
    print_color("[E1000] STATUS ready\n", VGA_COLOR_LIGHT_GREEN);
    print_color("[E1000] CTRL ready\n", VGA_COLOR_LIGHT_GREEN);
    // emmio ma ass
    e1000_write(E1000_REG_CTRL, ctrl | E1000_CTRL_RST);
    int timeout1 = 100000;
    while (timeout1-- && (e1000_read(E1000_REG_CTRL) & E1000_CTRL_RST));
    if (e1000_read(E1000_REG_CTRL) & E1000_CTRL_RST) {
        print_color("[e1000] ERROR: reset did not clear\n", VGA_COLOR_LIGHT_RED);
        return -1;
    }
    e1000_write(E1000_REG_CTRL, e1000_read(E1000_REG_CTRL) | E1000_CTRL_RST);
    int timeout2 = 100000;
    while (timeout2-- && (e1000_read(E1000_REG_CTRL) & E1000_CTRL_RST));

    e1000_write(E1000_REG_CTRL,
        e1000_read(E1000_REG_CTRL) | E1000_CTRL_SLU | E1000_CTRL_ASDE);

    uint16_t mac0 = e1000_eeprom_read(0);
    uint16_t mac1 = e1000_eeprom_read(1);
    uint16_t mac2 = e1000_eeprom_read(2);
    net_mac.b[0] = mac0 & 0xFF; net_mac.b[1] = mac0 >> 8;
    net_mac.b[2] = mac1 & 0xFF; net_mac.b[3] = mac1 >> 8;
    net_mac.b[4] = mac2 & 0xFF; net_mac.b[5] = mac2 >> 8;

    for (int i = 0; i < 128; i++)
        e1000_write(E1000_REG_MTA + i * 4, 0);

    uint32_t ral = (uint32_t)net_mac.b[0] | ((uint32_t)net_mac.b[1] << 8) |
                   ((uint32_t)net_mac.b[2] << 16) | ((uint32_t)net_mac.b[3] << 24);
    uint32_t rah = (uint32_t)net_mac.b[4] | ((uint32_t)net_mac.b[5] << 8) | (1u << 31);
    e1000_write(E1000_REG_RAL0, ral);
    e1000_write(E1000_REG_RAH0, rah);

// RX ring
for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
    rx_descs[i].addr_low  = (uint32_t)rx_buffers[i];
    rx_descs[i].addr_high = 0;
    rx_descs[i].length    = 0;
    rx_descs[i].checksum  = 0;
    // no cso or cmd for RX because thats how god (me) intened
    rx_descs[i].status    = 0;   // not done yet
    rx_descs[i].errors    = 0;
    rx_descs[i].special   = 0;
}

e1000_write(E1000_REG_RDBAL, (uint32_t)rx_descs);
e1000_write(E1000_REG_RDBAH, 0);
e1000_write(E1000_REG_RDLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));
e1000_write(E1000_REG_RDH, 0);
e1000_write(E1000_REG_RDT, E1000_NUM_RX_DESC - 1); // NIC owns 0..N-1
rx_tail = 0;

// enable RX: 2048B buffers, broadcast, strip CRC
uint32_t rctl = E1000_RCTL_EN |
                E1000_RCTL_BAM |
                E1000_RCTL_SECRC |
                E1000_RCTL_BSIZE_2048;
e1000_write(E1000_REG_RCTL, rctl);

    // TX ring
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        tx_descs[i].addr_low  = (uint32_t)tx_buffers[i];
        tx_descs[i].addr_high = 0;
        tx_descs[i].status    = 1;
    }
    e1000_write(E1000_REG_TDBAL, (uint32_t)tx_descs);
    e1000_write(E1000_REG_TDBAH, 0);
    e1000_write(E1000_REG_TDLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    e1000_write(E1000_REG_TDH, 0);
    e1000_write(E1000_REG_TDT, 0);
    tx_tail = 0;
    e1000_write(E1000_REG_TCTL,
        E1000_TCTL_EN | E1000_TCTL_PSP |
        (0x0F << E1000_TCTL_CT_SHIFT) |
        (0x40 << E1000_TCTL_COLD_SHIFT));
    e1000_write(E1000_REG_TIPG, 0x0060200A);
    e1000_write(E1000_REG_IMS, (1 << 7));

    e1000_initialized = 1;
    print_color("[e1000] NIC initialized\n", VGA_COLOR_LIGHT_GREEN);
    return 0;
}
/*
// ---- Send ----
int e1000_send(uint8_t* data, uint16_t len) {
    if (len > E1000_BUFFER_SIZE) return -1;

    uint32_t i = tx_tail;
    int t = 100000;
    while (!(tx_descs[i].status & 0x1) && t--);
    if (!(tx_descs[i].status & 0x1)) return -1;

    uint8_t* dst = tx_buffers[i];
    for (uint16_t k = 0; k < len; k++) dst[k] = data[k];

    tx_descs[i].length = len;
    tx_descs[i].cmd    = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    tx_descs[i].status = 0;

    tx_tail = (tx_tail + 1) % E1000_NUM_TX_DESC;
    e1000_write(E1000_REG_TDT, tx_tail);
    return 0;
}
*/

int e1000_send(uint8_t* data, uint16_t len) {
    if (!e1000_initialized) {
        print_color("[e1000] send failed: NIC not initialized\n", VGA_COLOR_LIGHT_RED);
        return -1;
    }
    
    if (len > E1000_BUFFER_SIZE) return -1;

    // Very cheap debug: look at first byte (dest MAC[0]) or ethertype
    // Assuming standard Ethernet header:
    // dst[0..5], src[6..11], ethertype [12..13]
    uint16_t ethertype = (data[12] << 8) | data[13];
    if (ethertype == 0x0806) { // ARP
        print_color("[e1000] sending ARP\n", VGA_COLOR_LIGHT_BLUE);
    } else if (ethertype == 0x0800) { // IPv4
        print_color("[e1000] sending IPv4\n", VGA_COLOR_LIGHT_BLUE);
    }

    uint32_t i = tx_tail;
    int timeout = 100000;
    while (!(tx_descs[i].status & 0x1) && timeout--);
    if (!(tx_descs[i].status & 0x1)) {
        print_color("[e1000] TX desc busy timeout\n", VGA_COLOR_LIGHT_RED);
        return -1;
    }

    uint8_t* dst = tx_buffers[i];
    for (uint16_t k = 0; k < len; k++)
        dst[k] = data[k];

    tx_descs[i].length = len;
    tx_descs[i].cso    = 0;
    tx_descs[i].cmd    = E1000_TXD_CMD_EOP |
                         E1000_TXD_CMD_IFCS |
                         E1000_TXD_CMD_RS;
    tx_descs[i].status = 0;

    tx_tail = (tx_tail + 1) % E1000_NUM_TX_DESC;
    e1000_write(E1000_REG_TDT, tx_tail);

    return 0;
}
void e1000_poll(void) {
    if (!e1000_initialized) {
        return;
    }
    
    // Clear interrupt cause 
    e1000_read(E1000_REG_ICR);

    while (rx_descs[rx_tail].status & E1000_RXD_STAT_DD) {
        uint16_t len = rx_descs[rx_tail].length;

        // Optional debug:
        print_color("[e1000] got packet\n", VGA_COLOR_LIGHT_BLUE); // comment out if you wanna idk man

        net_receive(rx_buffers[rx_tail], len);

        rx_descs[rx_tail].status = 0;

        uint32_t old = rx_tail;
        rx_tail = (rx_tail + 1) % E1000_NUM_RX_DESC;

        e1000_write(E1000_REG_RDT, old);
    }
}
