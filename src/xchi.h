/*
 YAYYY XCHI DRIVERS FOR KUZUOS2 ARE COMING JAHOY JAHOY
 ofc since im a lazy piece of shit i copiied these fromm claude DIRECTLY into ur sexy system.
 BUT DOOO NOTTT WORRRYYY ofc imma change n write the .c file myself and edit this guy accordingly lol
 
 */

#ifndef XCHI_H
#define XCHI_H

#include "usb.h"

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef unsigned long      uintptr_t; // 64-bit build: unsigned long is 8 bytes

#define XCHI_CAPLENGTH   0x00 // u8
#define XCHI_HCIVERSION  0x02 // u16
#define XCHI_HCSPARAMS1  0x04
#define XCHI_HCSPARAMS2  0x08
#define XCHI_HCSPARAMS3  0x0C
#define XCHI_HCCPARAMS1  0x10
#define XCHI_DBOFF       0x14
#define XCHI_RTSOFF      0x18
#define XCHI_HCCPARAMS2  0x1C

#define XCHI_OP_USBCMD   0x00
#define XCHI_OP_USBSTS   0x04
#define XCHI_OP_PAGESIZE 0x08
#define XCHI_OP_DNCTRL   0x14
#define XCHI_OP_CRCR     0x18 // 64-bit
#define XCHI_OP_DCBAAP   0x30 // 64-bit
#define XCHI_OP_CONFIG   0x38
#define XCHI_OP_PORTSC(n) (0x400 + ((n) * 0x10)) // n = 0-based port index

/* USBCMD bits */
#define XCHI_CMD_RUN     (1 << 0)
#define XCHI_CMD_HCRESET (1 << 1)
#define XCHI_CMD_INTE    (1 << 2) // interrupter enable (leave off, we poll)

/* USBSTS bits */
#define XCHI_STS_HCH     (1 << 0) // halted
#define XCHI_STS_CNR     (1 << 11) // controller not ready

/* PORTSC bits */
#define XCHI_PORTSC_CCS  (1 << 0)  // current connect status
#define XCHI_PORTSC_PED  (1 << 1)  // port enabled
#define XCHI_PORTSC_PR   (1 << 4)  // port reset
#define XCHI_PORTSC_PLS_SHIFT 5
#define XCHI_PORTSC_PP   (1 << 9)  // port power
#define XCHI_PORTSC_SPEED_SHIFT 10 // 4 bits
#define XCHI_PORTSC_CSC  (1 << 17) // connect status change
#define XCHI_PORTSC_PRC  (1 << 21) // port reset change
#define XCHI_PORTSC_WRITE_1_CLEAR_MASK 0x00FE0002 // change bits, write-1-to-clear; PED also W1C-ish, handled carefully in code

/* ---- Runtime register offsets (from rtbase = xchi_base + rtsoff) ---- */
#define XCHI_RT_IR0      0x20 // interrupter register set 0
#define XCHI_IR_IMAN     0x00
#define XCHI_IR_IMOD     0x04
#define XCHI_IR_ERSTSZ   0x08
#define XCHI_IR_ERSTBA   0x10 // 64-bit
#define XCHI_IR_ERDP     0x18 // 64-bit

/* ---- Doorbell registers (from dbbase = xchi_base + dboff) ---- */
#define XCHI_DB(slot)    ((slot) * 4)

/* ---- TRB types ---- */
#define TRB_TYPE_NORMAL         1
#define TRB_TYPE_SETUP_STAGE    2
#define TRB_TYPE_DATA_STAGE     3
#define TRB_TYPE_STATUS_STAGE   4
#define TRB_TYPE_LINK           6
#define TRB_TYPE_ENABLE_SLOT    9
#define TRB_TYPE_DISABLE_SLOT   10
#define TRB_TYPE_ADDRESS_DEVICE 11
#define TRB_TYPE_CONFIG_EP      12
#define TRB_TYPE_EVAL_CONTEXT   13
#define TRB_TYPE_NOOP_CMD       23
#define TRB_TYPE_TRANSFER_EVENT 32
#define TRB_TYPE_CMD_COMPLETION 33
#define TRB_TYPE_PORT_STATUS_CHANGE 34

#define TRB_CYCLE (1 << 0)
#define TRB_IOC   (1 << 5)   // interrupt on completion (unused, we poll)
#define TRB_IDT   (1 << 6)   // immediate data (setup stage)

/* generic 16-byte TRB, matches your qtd_t/qh_t "flat struct" style */
typedef struct {
    u32 param_lo;
    u32 param_hi;
    u32 status;
    u32 control; // bits 10-15 = TRB type, bit 0 = cycle
} __attribute__((packed)) trb_t;

#define TRB_SET_TYPE(trb, t) ((trb)->control = ((trb)->control & ~(0x3F << 10)) | ((t) << 10))
#define TRB_GET_TYPE(trb)    (((trb)->control >> 10) & 0x3F)
#define TRB_GET_COMPLETION_CODE(trb) (((trb)->status >> 24) & 0xFF)
#define TRB_GET_SLOT_ID(trb)  (((trb)->control >> 24) & 0xFF)
#define TRB_GET_PORT_ID(trb)  (((trb)->param_lo >> 24) & 0xFF) // port status change events only
							       
#define XCHI_RING_SIZE 256 
/* input context / device context sizes assume CSZ=0 (32-byte contexts), by far the common case.
   if HCCPARAMS1 CSZ bit is set, contexts are 64 bytes and everything below needs *2 -
   we check and print a warning at init if that's the case, since KuzuOS2 doesn't handle it yet. */
#define XCHI_CTX_SIZE 32

typedef struct {
    u32 drop_flags;
    u32 add_flags;
    u32 rsvd[5];
    u32 config_value; // rsvd7[7], reused as config/interface/alt in some layouts - kept minimal
} __attribute__((packed)) xchi_input_control_ctx_t;

/* slot context, 32 bytes (CSZ=0) */
typedef struct {
    u32 route_speed;   // route string[19:0] | speed[23:20] | ...
    u32 max_exit_lat_rh; // root hub port etc
    u32 tt_info;
    u32 dev_addr_state; // usb device address[7:0] | slot state[31:27]
    u32 rsvd[4];
} __attribute__((packed)) xchi_slot_ctx_t;

typedef struct {
    u32 ep_state;       // ep state[2:0] | interval[23:16] | ...
    u32 ep_type_mps;    // error count[2:1] | ep type[5:3] | max packet size[31:16]
    u32 deq_lo;         // dequeue ptr low | DCS bit
    u32 deq_hi;
    u32 avg_trb_len_max_esit;
    u32 rsvd[3];
} __attribute__((packed)) xchi_ep_ctx_t;


typedef struct {
    int used;
    u32 slot_id;
    u32 port;           // 0-based xHC port index
    u32 route_string;
    u32 speed;           
    u32 max_packet_size;
    void* input_ctx;     // virtual ptr to input context (64-byte aligned)
    void* output_ctx;    // virtual ptr to device context (64-byte aligned)
    trb_t* ep0_ring;      // control endpoint transfer ring
    u32 ep0_enq_idx;
    u32 ep0_cycle;        // producer cycle state for ep0 ring

    u8 class, subclass, protocol;
    u8 bulk_in_dci, bulk_out_dci;
    u16 bulk_in_mps, bulk_out_mps;
    trb_t* bulk_in_ring;
    trb_t* bulk_out_ring;
    u32 bulk_in_enq_idx, bulk_in_cycle;
    u32 bulk_out_enq_idx, bulk_out_cycle;
    u32 msc_tag;
    u16 vendor_id, product_id;
    u32 usb_bus;              // for lsusb 
    usb_device_t* generic; // link back to the usb.c-level device, once class/enum is done
} xchi_device_t;

#define MAX_XCHI_DEVICES 32

void xchi_init(unsigned int bus, unsigned int slot, unsigned int func);
void xchi_poll(void); // call from usb_poll() alongside the EHCI/keyboard poll
int xchi_enumerate_port(unsigned int port);
int xchi_control_transfer(xchi_device_t* xdev, usb_setup_t* setup, void* data, unsigned int len, int is_in);
int xchi_bulk_transfer(xchi_device_t* xdev, void* buf, unsigned int len, int is_in);
extern xchi_device_t xchi_devices[MAX_XCHI_DEVICES]; // for lsusb in syscall.c check out SYS_LSUSB for it
#endif
