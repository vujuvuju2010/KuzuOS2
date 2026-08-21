#ifndef E1000_H
#define E1000_H

#include "net.h"

// PCI vendor/device
#define E1000_VENDOR_ID  0x8086
#define E1000_DEVICE_ID  0x100E   // 82540EM — what QEMU emulates

// e1000 MMIO register offsets
#define E1000_REG_CTRL      0x0000
#define E1000_REG_STATUS    0x0008
#define E1000_REG_EECD      0x0010
#define E1000_REG_EERD      0x0014
#define E1000_REG_ICR       0x00C0   // interrupt cause read
#define E1000_REG_IMS       0x00D0   // interrupt mask set
#define E1000_REG_IMC       0x00D8   // interrupt mask clear
#define E1000_REG_RCTL      0x0100   // receive control
#define E1000_REG_TCTL      0x0400   // transmit control
#define E1000_REG_TIPG      0x0410   // tx inter-packet gap
#define E1000_REG_RDBAL     0x2800   // rx desc base low
#define E1000_REG_RDBAH     0x2804   // rx desc base high
#define E1000_REG_RDLEN     0x2808   // rx desc ring length
#define E1000_REG_RDH       0x2810   // rx desc head
#define E1000_REG_RDT       0x2818   // rx desc tail
#define E1000_REG_TDBAL     0x3800   // tx desc base low
#define E1000_REG_TDBAH     0x3804
#define E1000_REG_TDLEN     0x3808
#define E1000_REG_TDH       0x3810
#define E1000_REG_TDT       0x3818
#define E1000_REG_MTA       0x5200   // multicast table (128 dwords)
#define E1000_REG_RAL0      0x5400   // receive addr low
#define E1000_REG_RAH0      0x5404   // receive addr high

// CTRL bits
#define E1000_CTRL_RST      (1 << 26)
#define E1000_CTRL_SLU      (1 << 6)   // set link up
#define E1000_CTRL_ASDE     (1 << 5)   // auto-speed detect

// RCTL bits
#define E1000_RCTL_EN       (1 << 1)
#define E1000_RCTL_SBP      (1 << 2)
#define E1000_RCTL_UPE      (1 << 3)   // unicast promisc
#define E1000_RCTL_MPE      (1 << 4)   // multicast promisc
#define E1000_RCTL_BAM      (1 << 15)  // broadcast accept
#define E1000_RCTL_BSIZE_2048 0        // buffer size 2048 (bits 17:16 = 00)
#define E1000_RCTL_SECRC    (1 << 26)  // strip CRC

// TCTL bits
#define E1000_TCTL_EN       (1 << 1)
#define E1000_TCTL_PSP      (1 << 3)   // pad short packets
#define E1000_TCTL_CT_SHIFT 4
#define E1000_TCTL_COLD_SHIFT 12

// Descriptor counts (must be multiples of 8)
#define E1000_NUM_RX_DESC   32
#define E1000_NUM_TX_DESC   32
#define E1000_BUFFER_SIZE   2048

// RX descriptor
typedef struct {
    uint32_t addr_low;
    uint32_t addr_high;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

// TX descriptor
typedef struct {
    uint32_t addr_low;
    uint32_t addr_high;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

// TX cmd bits
#define E1000_TXD_CMD_EOP   (1 << 0)  // end of packet
#define E1000_TXD_CMD_IFCS  (1 << 1)  // insert FCS/CRC
#define E1000_TXD_CMD_RS    (1 << 3)  // report status

// RX status bits
#define E1000_RXD_STAT_DD   (1 << 0)  // descriptor done
#define E1000_RXD_STAT_EOP  (1 << 1)  // end of packet

int  e1000_init(void);
int  e1000_send(uint8_t* data, uint16_t len);
void e1000_poll(void);   // call from IRQ or main loop

#endif
