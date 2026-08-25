/* 
 XCHI DRIVERS FOR KUZUOS2 YAY
 ofc as of always:
 written by Vujuvuju(also known as god)
 
 
 */
// NOTE: u32, u64, u16 and u8 are defined in xchi.h SO DONT WORRY

#include "xchi.h"
#include "z_utils.h" // imma use ts instead of vga jus because we doing x64
#include "vga.h" // ehm ehm..

#define XCHI_PHYS(virt) ((u64)(uintptr_t)(virt))	//needs to change if i do ts fr

extern void* dma_alloc(unsigned int size); // from usb.c ofc ofc

static unsigned int xchi_base = 0;
static unsigned int xchi_op = 0; // operational regiser deafult
static unsigned int xchi_rt = 0; // runtime regiser
static unsigned int xchi_db = 0; // kapıkulbu dıdıp TISSS
static unsigned int xchi_ports = 0;
static unsigned int xchi_max_sluts = 0;

static unsigned int xchi_context_size = XCHI_CTX_SIZE; //32 unless CZS says fuhnaw
static unsigned int xchi_bus = 0;
static trb_t* event_ring = 0;
static u32 cmd_cycle = 1;
static trb_t* cmd_ring = 0;
static u32  cmd_enq_idx = 0;
static u32 event_deq_idx = 0;
static u32 event_css = 1; // OMG 1
static trb_t* erst = 0;
static u64* dcbaa = 0; // device context
		       //
		       //
static void xchi_handle_connect(unsigned int port);
static void xchi_handle_disconnect(unsigned int port);


xchi_device_t xchi_devices[MAX_XCHI_DEVICES]; // cant be static since used in syscall.c for lsusb of such

static inline u32 xchi_cap_read32(unsigned int reg){
	return *((volatile u32*)(xchi_base + reg));
}

static inline u16 xchi_cap_read16(unsigned int reg){
	return *((volatile u16*)(xchi_base + reg));
}

static inline u8 xchi_cap_read8(unsigned int reg){
    return *((volatile u8*)(xchi_base + reg));
}

static inline u32 xchi_op_read32(unsigned int reg){
    return *((volatile u32*)(xchi_op + reg));
}
static inline void xchi_op_write32(unsigned int reg, u32 val){
    *((volatile u32*)(xchi_op + reg)) = val;
}


static inline u64 xchi_op_read64(unsigned int reg){
/*  yea we read two 32 bits so u bet ur sweet ass it gon be slow AF
	but ehh who gives a fah

*/
	u32 lo = xchi_op_read32(reg);
	u32 hi = xchi_op_read32(reg + 4); // hi-fi life ref
	return ((u64)hi << 32) | lo;	

}

static inline void xchi_op_write64(unsigned int reg, u64 val){
	xchi_op_write32(reg, (u32)(val & 0xFFFFFFFF));
	xchi_op_write32(reg + 4, (u32)(val >> 32));
}

static inline u32 xchi_rt_read32(unsigned int reg){
    return *((volatile u32*)(xchi_rt + reg));
}
static inline void xchi_rt_write32(unsigned int reg, u32 val){
    *((volatile u32*)(xchi_rt + reg)) = val;
}
static inline void xchi_rt_write64(unsigned int reg, u64 val){
	xchi_rt_write32(reg, (u32)(val & 0xFFFFFFFF));
	xchi_rt_write32(reg + 4, (u32)(val >> 32));
}

static inline u64 xchi_rt_read64(unsigned int reg){
    u32 lo = xchi_rt_read32(reg);
    u32 hi = xchi_rt_read32(reg + 4);
    return ((u64)hi << 32) | lo;
}


static inline void xchi_db_write(unsigned int slot, u32 val){
*((volatile u32*)(xchi_db + XCHI_DB(slot))) = val;

}

// pci config


#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static inline void xchi_outl(unsigned short port, unsigned int val){

__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

// I COPIED AFTER THISS SRYY IM BORED AS HELL 
static inline unsigned int xchi_inl(unsigned short port){
    unsigned int ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static unsigned int xchi_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset){
    unsigned int address = (1u << 31) | ((unsigned int)bus << 16) | ((unsigned int)slot << 11)
                          | ((unsigned int)func << 8) | (offset & 0xFC);
    xchi_outl(PCI_CONFIG_ADDR, address);
    return xchi_inl(PCI_CONFIG_DATA);
}
static void xchi_pci_write(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int val){
    unsigned int address = (1u << 31) | ((unsigned int)bus << 16) | ((unsigned int)slot << 11)
                          | ((unsigned int)func << 8) | (offset & 0xFC);
    xchi_outl(PCI_CONFIG_ADDR, address);
    xchi_outl(PCI_CONFIG_DATA, val);
}
 
/* xHCI BAR0 can be a 64-bit BAR (bit 2:1 of BAR0 low dword == 0b10), meaning BAR1 holds
   the high 32 bits. EHCI/UHCI BARs are always 32-bit, which is why usb.c's bar read
   doesn't need this. Get it wrong here and xchi_base silently truncates on a >4GB MMIO
   mapping - unlikely on real hardware/QEMU default layouts but worth doing right. */
static u64 xchi_read_bar0(unsigned char bus, unsigned char slot, unsigned char func){
    unsigned int bar0 = xchi_pci_read(bus, slot, func, 0x10);
    unsigned int bar_type = (bar0 >> 1) & 0x3; // bits 2:1
    u64 base = bar0 & ~0xFULL;
    if(bar_type == 0x2){ // 64-bit BAR
        unsigned int bar1 = xchi_pci_read(bus, slot, func, 0x14);
        base |= ((u64)bar1 << 32);
    }
    return base;
}
 
static void xchi_pci_enable_busmaster(unsigned char bus, unsigned char slot, unsigned char func){
    unsigned int cmd = xchi_pci_read(bus, slot, func, 0x04);
    cmd |= (1 << 2) /* bus master */ | (1 << 1) /* memory space */;
    xchi_pci_write(bus, slot, func, 0x04, cmd);
}
 
/* ---- ring helpers ---- */
 
static void xchi_ring_zero(trb_t* ring, unsigned int count){
    unsigned char* p = (unsigned char*)ring;
    for(unsigned int i = 0; i < count * sizeof(trb_t); i++) p[i] = 0;
}
 
/* build a Link TRB at the last slot of a ring so hardware wraps back to index 0.
   toggle_cycle=1 for the command/transfer rings (producer must flip its own cycle bit
   on wrap), 0 is never actually used here but kept for clarity. */
static void xchi_ring_add_link(trb_t* ring, unsigned int count, u32 initial_cycle){
    trb_t* link = &ring[count - 1];
    u64 phys = XCHI_PHYS(ring);
    link->param_lo = (u32)(phys & 0xFFFFFFFF);
    link->param_hi = (u32)(phys >> 32);
    link->status = 0;
    link->control = (initial_cycle & TRB_CYCLE) | (1 << 1) /* toggle cycle */;
    TRB_SET_TYPE(link, TRB_TYPE_LINK);
}
 
/* ---- command ring ---- */
 
static void xchi_init_command_ring(void){
    cmd_ring = (trb_t*)dma_alloc(sizeof(trb_t) * XCHI_RING_SIZE + 64);
    /* align to 64 bytes - dma_alloc only guarantees 32, so nudge up if needed */
    cmd_ring = (trb_t*)(((uintptr_t)cmd_ring + 63) & ~(uintptr_t)63);
    xchi_ring_zero(cmd_ring, XCHI_RING_SIZE);
    xchi_ring_add_link(cmd_ring, XCHI_RING_SIZE, 1);
    cmd_enq_idx = 0;
    cmd_cycle = 1;
 
    u64 crcr = XCHI_PHYS(cmd_ring) | 1 /* ring cycle state = 1 */;
    xchi_op_write64(XCHI_OP_CRCR, crcr);
}
 
/* enqueue a command TRB (caller fills param_lo/hi/status/type BEFORE calling this -
   this function only stamps the cycle bit and rings the doorbell) */
static trb_t* xchi_cmd_enqueue_slot(void){
    trb_t* trb = &cmd_ring[cmd_enq_idx];
    return trb;
}
 
static void xchi_cmd_commit(trb_t* trb){
    trb->control = (trb->control & ~TRB_CYCLE) | (cmd_cycle & TRB_CYCLE);
    cmd_enq_idx++;
    if(cmd_enq_idx == XCHI_RING_SIZE - 1){ // hit the link TRB slot
        cmd_ring[XCHI_RING_SIZE - 1].control = (cmd_ring[XCHI_RING_SIZE - 1].control & ~TRB_CYCLE) | (cmd_cycle & TRB_CYCLE);
        cmd_enq_idx = 0;
        cmd_cycle ^= 1;
    }
    xchi_db_write(0, 0); // doorbell 0, target 0 = command ring
}
 
/* ---- event ring ---- */
 
static void xchi_init_event_ring(void){
    event_ring = (trb_t*)dma_alloc(sizeof(trb_t) * XCHI_RING_SIZE + 64);
    event_ring = (trb_t*)(((uintptr_t)event_ring + 63) & ~(uintptr_t)63);
    xchi_ring_zero(event_ring, XCHI_RING_SIZE);
    /* NOTE: event ring does NOT use a link TRB - it's a plain circular buffer described
       by the ERST + ERDP. hardware wraps on its own once it produces ERSTSZ*count events. */
 
    erst = (trb_t*)dma_alloc(64); // one ERST entry: 16 bytes (base ptr 64-bit + size + rsvd), pad to 64 for alignment
    erst = (trb_t*)(((uintptr_t)erst + 63) & ~(uintptr_t)63);
    u32* erst_entry = (u32*)erst;
    u64 ring_phys = XCHI_PHYS(event_ring);
    erst_entry[0] = (u32)(ring_phys & 0xFFFFFFFF);
    erst_entry[1] = (u32)(ring_phys >> 32);
    erst_entry[2] = XCHI_RING_SIZE; // segment size (in TRBs)
    erst_entry[3] = 0; // reserved
 
    event_deq_idx = 0;
    event_css = 1;
 
    xchi_rt_write32(XCHI_RT_IR0 + XCHI_IR_ERSTSZ, 1); // 1 segment
    xchi_rt_write64(XCHI_RT_IR0 + XCHI_IR_ERDP, XCHI_PHYS(event_ring));
    xchi_rt_write64(XCHI_RT_IR0 + XCHI_IR_ERSTBA, XCHI_PHYS(erst));
}
 
/* poll for the next event TRB. returns 0 and fills *out if one is pending, -1 if the
   ring is empty (cycle bit doesn't match consumer cycle state). caller must call
   xchi_event_advance() after consuming it to move the dequeue pointer + ring ERDP. */
static int xchi_event_peek(trb_t** out){
    trb_t* trb = &event_ring[event_deq_idx];
    if((trb->control & TRB_CYCLE) != (event_css & TRB_CYCLE))
        return -1;
    *out = trb;
    return 0;
}
 
static void xchi_event_advance(void){
    event_deq_idx++;
    if(event_deq_idx == XCHI_RING_SIZE){
        event_deq_idx = 0;
        event_css ^= 1;
    }
    u64 erdp = XCHI_PHYS(&event_ring[event_deq_idx]) | (1 << 3) /* event handler busy, W1C */;
    xchi_rt_write64(XCHI_RT_IR0 + XCHI_IR_ERDP, erdp);
}
 
/* spin-wait for a specific command completion event matching a TRB we submitted.
   matches by physical address in the completion event's param field, same idea as
   your EHCI code polling QTD_ACTIVE - just a timeout loop, no interrupts. */
static int xchi_wait_command_completion(trb_t* submitted_trb, trb_t* out_event){
    u64 want_phys = XCHI_PHYS(submitted_trb);
    int timeout = 5000000;
    while(timeout--){
        trb_t* ev;
        if(xchi_event_peek(&ev) == 0){
            if(TRB_GET_TYPE(ev) == TRB_TYPE_CMD_COMPLETION){
                u64 ev_phys = ((u64)ev->param_hi << 32) | ev->param_lo;
                *out_event = *ev; // copy out before we advance / hw reuses the slot
                xchi_event_advance();
                if(ev_phys == want_phys)
                    return 0;
                /* completion for a different command (shouldn't happen since we're
                   fully serial here, but handle it rather than silently misattribute) */
                continue;
            } else {
                /* not what we're waiting for (e.g. port status change) - drain it */
                xchi_event_advance();
                continue;
            }
        }
    }
    z_printf("xchi: command completion timeout shat itself :(\n");
    return -1;
}
 
/* ---- generic command submission ----
   used by Enable Slot, Address Device, Configure Endpoint, etc. caller fills
   param_lo/hi (if the command needs an input context pointer) and passes the
   TRB type; this stamps cycle, rings the doorbell, and waits for completion. */
static int xchi_run_command(u32 param_lo, u32 param_hi, u32 status, u32 control_base, u32 type, trb_t* completion_out){
    trb_t* trb = xchi_cmd_enqueue_slot();
    trb->param_lo = param_lo;
    trb->param_hi = param_hi;
    trb->status = status;
    trb->control = control_base;  // Preserve slot ID or other control bits
    TRB_SET_TYPE(trb, type);
 
    xchi_cmd_commit(trb);  // Actually submit the command to the controller!
 
    if(xchi_wait_command_completion(trb, completion_out) != 0)
        return -1;
 
    u32 cc = TRB_GET_COMPLETION_CODE(completion_out);
    if(cc != 1){ // 1 = SUCCESS
        z_printf("xchi: command type=%d failed, completion code=%d\n", type, cc);
        return -1;
    }
    return 0;
}
 
/* ---- device context / slot management ---- */
 
/* speed value straight from PORTSC bits 13:10. xHCI slot context wants this
   verbatim in its speed field (1=full,2=low,3=high,4=super - matches USB3 spec
   table, but if your controller reports SuperSpeed+ variants (5/6) you may need
   to special-case max packet size below). */
static u32 xchi_default_max_packet(u32 speed){
    switch(speed){
        case 1: return 64;  // full speed
        case 2: return 8;   // low speed
        case 3: return 64;  // high speed
        case 4: return 512; // super speed
        default: return 8;  // safest default until we read the real descriptor
    }
}
 
/* allocate + zero a 64-byte-aligned block, since dma_alloc only guarantees 32 */
static void* xchi_alloc_aligned(unsigned int size){
    void* raw = dma_alloc(size + 64);
    void* aligned = (void*)(((uintptr_t)raw + 63) & ~(uintptr_t)63);
    unsigned char* p = (unsigned char*)aligned;
    for(unsigned int i = 0; i < size; i++) p[i] = 0;
    return aligned;
}
 
/* set up ep0 (control endpoint) transfer ring for a freshly enabled slot */
static void xchi_init_ep0_ring(xchi_device_t* xdev){
    xdev->ep0_ring = (trb_t*)xchi_alloc_aligned(sizeof(trb_t) * XCHI_RING_SIZE);
    xchi_ring_add_link(xdev->ep0_ring, XCHI_RING_SIZE, 1);
    xdev->ep0_enq_idx = 0;
    xdev->ep0_cycle = 1;
}
 
/* Enable Slot -> Address Device for a device that just appeared on `port` (0-based).
   Returns 0 and fills *slot_id_out on success. This is the xHCI equivalent of your
   EHCI resetport() + usb_enumerate_device()'s SET_ADDRESS step, combined - xHCI does
   both the "give it an address" and "allocate host-side tracking" in one Address
   Device command rather than a raw control transfer. */
static int xchi_enable_and_address(unsigned int port, u32 speed, xchi_device_t** out_dev){
    trb_t completion;
 
    /* Enable Slot */
    if(xchi_run_command(0, 0, 0, 0, TRB_TYPE_ENABLE_SLOT, &completion) != 0){
        print_color("xchi: enable slot shat itself :(\n", VGA_COLOR_RED);
        return -1;
    }
    u32 slot_id = TRB_GET_SLOT_ID(&completion);
    if(slot_id == 0 || slot_id > xchi_max_sluts){
        z_printf("xchi: bogus slot id %d\n", slot_id);
        return -1;
    }
    z_printf("xchi: enabled slot %d for port %d\n", slot_id, port);
 
    xchi_device_t* xdev = 0;
    for(unsigned int i = 0; i < MAX_XCHI_DEVICES; i++){
        if(!xchi_devices[i].used){
            xdev = &xchi_devices[i];
            break;
        }
    }
    if(!xdev){
        z_printf("xchi: no free xchi_device_t slots :(\n");
        return -1;
    }
    xdev->used = 1;
    xdev->slot_id = slot_id;
    xdev->port = port;
    xdev->speed = speed;
    xdev->max_packet_size = xchi_default_max_packet(speed);
    xdev->usb_bus = xchi_bus;
    xdev->route_string = 0; // direct root-hub attach; hub support would need real routing
 
    /* output device context - controller writes slot/ep state here after Address Device */
    xdev->output_ctx = xchi_alloc_aligned(xchi_context_size * 32); // room for slot + 31 eps
    dcbaa[slot_id] = XCHI_PHYS(xdev->output_ctx);
 
    /* input context: control block (drop/add flags) + slot ctx + ep0 ctx, all contiguous.
       layout for CSZ=0 (32-byte contexts): [input ctrl ctx][slot ctx][ep0 ctx][...unused] */
    unsigned char* input = (unsigned char*)xchi_alloc_aligned(xchi_context_size * 33);
    xdev->input_ctx = input;
 
    xchi_input_control_ctx_t* ctrl = (xchi_input_control_ctx_t*)input;
    ctrl->drop_flags = 0;
    ctrl->add_flags = (1 << 0) | (1 << 1); // A0 (slot ctx) | A1 (ep0 ctx)
    for (int i = 0; i < 6; i++) ctrl->rsvd[i] = 0; // zero reserved fields
 
    xchi_slot_ctx_t* slot_ctx = (xchi_slot_ctx_t*)(input + xchi_context_size);
    slot_ctx->route_speed = (xdev->route_string & 0xFFFFF) | (speed << 20) | (1 << 27); // Context Entries = 1 (slot + ep0)
    slot_ctx->max_exit_lat_rh = ((port + 1) << 16); // root hub port number is 1-based
    slot_ctx->tt_info = 0;
    slot_ctx->dev_addr_state = 0; // address/state filled by hardware after the command
    for (int i = 0; i < 4; i++) slot_ctx->rsvd[i] = 0; // zero reserved fields
 
    xchi_init_ep0_ring(xdev);
 
    xchi_ep_ctx_t* ep0_ctx = (xchi_ep_ctx_t*)(input + xchi_context_size * 2);
    ep0_ctx->ep_state = 0;
    ep0_ctx->ep_type_mps = (4 << 3) /* ep type = control */ | (xdev->max_packet_size << 16) | (3 << 1) /* CErr=3 */;
    u64 deq_phys = XCHI_PHYS(xdev->ep0_ring) | 1 /* DCS = 1, matches ep0_cycle initial value */;
    ep0_ctx->deq_lo = (u32)(deq_phys & 0xFFFFFFFF);
    ep0_ctx->deq_hi = (u32)(deq_phys >> 32);
    ep0_ctx->avg_trb_len_max_esit = 8; // average TRB length, 8 is the usual safe default for ep0
    for (int i = 0; i < 3; i++) ep0_ctx->rsvd[i] = 0; // zero reserved fields
 
    u64 input_phys = XCHI_PHYS(input);
    if(xchi_run_command((u32)(input_phys & 0xFFFFFFFF), (u32)(input_phys >> 32), 0,
                         (xdev->slot_id << 24), TRB_TYPE_ADDRESS_DEVICE, &completion) != 0){
        print_color("xchi: address device shat itself :(\n", VGA_COLOR_RED);
        xdev->used = 0;
        return -1;
    }
 
    print_color("xchi: device addressed, yayyyy\n", VGA_COLOR_GREEN);
    *out_dev = xdev;
    return 0;
}
 
/* ---- control transfers over the xHCI transfer ring ----
   equivalent of your ehci_control(). xHCI control transfers are 3 TRBs on the target
   endpoint's ring: Setup Stage (carries the 8-byte setup packet inline via IDT),
   Data Stage (optional), Status Stage. No QH/qTD linked-list dance needed - xHCI
   rings are just flat circular buffers per endpoint. */
static trb_t* xchi_ep0_enqueue_slot(xchi_device_t* xdev){
    trb_t* trb = &xdev->ep0_ring[xdev->ep0_enq_idx];
    return trb;
}
 
static void xchi_ep0_commit(xchi_device_t* xdev, trb_t* trb, int ring_doorbell){
    trb->control = (trb->control & ~TRB_CYCLE) | (xdev->ep0_cycle & TRB_CYCLE);
    xdev->ep0_enq_idx++;
    if(xdev->ep0_enq_idx == XCHI_RING_SIZE - 1){
        xdev->ep0_ring[XCHI_RING_SIZE - 1].control =
            (xdev->ep0_ring[XCHI_RING_SIZE - 1].control & ~TRB_CYCLE) | (xdev->ep0_cycle & TRB_CYCLE);
        xdev->ep0_enq_idx = 0;
        xdev->ep0_cycle ^= 1;
    }
    if(ring_doorbell)
        xchi_db_write(xdev->slot_id, 1); // doorbell target 1 = ep0 (DCI 1)
}
 
/* wait for a transfer event tied to this device's ep0 ring. same polling pattern as
   xchi_wait_command_completion, just matching TRB_TYPE_TRANSFER_EVENT instead. */
static int xchi_wait_transfer_completion(trb_t* out_event){
    int timeout = 5000000;
    while(timeout--){
        trb_t* ev;
        if(xchi_event_peek(&ev) == 0){
            if(TRB_GET_TYPE(ev) == TRB_TYPE_TRANSFER_EVENT){
                *out_event = *ev;
                xchi_event_advance();
                return 0;
            }
            xchi_event_advance(); // drain anything else (port status change etc.)
            continue;
        }
    }
    z_printf("xchi: transfer completion timeout shat itself :(\n");
    return -1;
}
 
int xchi_control_transfer(xchi_device_t* xdev, usb_setup_t* setup, void* data, unsigned int len, int is_in){
    trb_t completion;
 
    /* Setup Stage - setup packet goes inline in param_lo/hi via IDT, not a pointer */
    trb_t* setup_trb = xchi_ep0_enqueue_slot(xdev);
    /* NOTE: assumes setup->bmreq/breq/wval are the raw bmRequestType/bRequest/wValue
       values (same fields ehci_control() sets). xHCI wants them packed little-endian
       USB order: byte0=bmRequestType, byte1=bRequest, bytes2-3=wValue. */
    setup_trb->param_lo = (u32)(setup->bmreq & 0xFF) | ((u32)(setup->breq & 0xFF) << 8) | ((u32)setup->wval << 16);
    setup_trb->param_hi = (u32)setup->widx | ((u32)setup->wlen << 16);
    setup_trb->status = 8; // TRB transfer length = 8 (setup packet size)
    setup_trb->control = TRB_IDT;
    TRB_SET_TYPE(setup_trb, TRB_TYPE_SETUP_STAGE);
    /* transfer type field (bits 17:16 of control): 0=no data,2=OUT,3=IN */
    setup_trb->control |= (len == 0) ? 0 : (is_in ? (3 << 16) : (2 << 16));
    xchi_ep0_commit(xdev, setup_trb, 0);
 
    /* Data Stage - only if there's a data phase */
    if(len > 0){
        trb_t* data_trb = xchi_ep0_enqueue_slot(xdev);
        u64 phys = XCHI_PHYS(data);
        data_trb->param_lo = (u32)(phys & 0xFFFFFFFF);
        data_trb->param_hi = (u32)(phys >> 32);
        data_trb->status = len; // TRB transfer length
        data_trb->control = (is_in ? (1 << 16) : 0); // direction bit (bit 16): 1=IN
        TRB_SET_TYPE(data_trb, TRB_TYPE_DATA_STAGE);
        xchi_ep0_commit(xdev, data_trb, 0);
    }
 
    /* Status Stage - direction is opposite of data stage (or IN if no data phase) */
    trb_t* status_trb = xchi_ep0_enqueue_slot(xdev);
    status_trb->param_lo = 0;
    status_trb->param_hi = 0;
    status_trb->status = 0;
    status_trb->control = TRB_IOC; // we want a completion event for this one
    int status_dir_in = (len == 0) ? 1 : !is_in;
    status_trb->control |= (status_dir_in ? (1 << 16) : 0);
    TRB_SET_TYPE(status_trb, TRB_TYPE_STATUS_STAGE);
    xchi_ep0_commit(xdev, status_trb, 1); // ring the doorbell now that all 2-3 TRBs are queued
 
    if(xchi_wait_transfer_completion(&completion) != 0)
        return -1;
 
    u32 cc = TRB_GET_COMPLETION_CODE(&completion);
    if(cc != 1 && cc != 13 /* SHORT_PACKET, common and not fatal for control transfers */){
        z_printf("xchi: control transfer failed, completion code=%d\n", cc);
        return -1;
    }
    return 0;
}
 
/* full bring-up for one connected port: reset -> wait for PRC -> enable slot ->
   address device -> GET_DESCRIPTOR(device) so we at least know vid/pid, matching
   what usb_enumerate_device() does for EHCI. hands back the xchi_device_t so the
   caller can go on to parse config descriptors / bind a driver, same shape as
   usb_parse_endpoints() + usb_bind_driver() in usb.c. */
int xchi_enumerate_port(unsigned int port){
    u32 portsc = xchi_op_read32(XCHI_OP_PORTSC(port));
 
    /* reset the port - same write-1-set-bit-then-poll-change-bit idea as your
       resetport(), but xHCI's PR/PRC are separate bits and change bits are W1C */
    portsc &= ~XCHI_PORTSC_WRITE_1_CLEAR_MASK;
    portsc |= XCHI_PORTSC_PR;
    xchi_op_write32(XCHI_OP_PORTSC(port), portsc);
 
    int timeout = 1000000;
    while(!(xchi_op_read32(XCHI_OP_PORTSC(port)) & XCHI_PORTSC_PRC) && --timeout);
    if(timeout == 0){
        print_color("xchi: port reset never completed :(\n", VGA_COLOR_RED);
        return -1;
    }
    /* clear PRC (W1C) */
    portsc = xchi_op_read32(XCHI_OP_PORTSC(port));
    xchi_op_write32(XCHI_OP_PORTSC(port), (portsc & ~XCHI_PORTSC_WRITE_1_CLEAR_MASK) | XCHI_PORTSC_PRC);
 
    portsc = xchi_op_read32(XCHI_OP_PORTSC(port));
    if(!(portsc & XCHI_PORTSC_PED)){
        print_color("xchi: port didn't enable after reset :(\n", VGA_COLOR_RED);
        return -1;
    }
    u32 speed = (portsc >> XCHI_PORTSC_SPEED_SHIFT) & 0xF;
    z_printf("xchi: port %d reset ok, speed=%d\n", port, speed);
 
    xchi_device_t* xdev;
    if(xchi_enable_and_address(port, speed, &xdev) != 0)
        return -1;
 
    /* GET_DESCRIPTOR (device), same 18-byte request usb_enumerate_device() makes */
    unsigned char* desc = (unsigned char*)dma_alloc(18);
    usb_setup_t* setup = (usb_setup_t*)dma_alloc(sizeof(usb_setup_t));
    setup->bmreq = 0x80;
    setup->breq  = 0x06;
    setup->wval  = 0x0100;
    setup->widx  = 0;
    setup->wlen  = 18;
 
    if(xchi_control_transfer(xdev, setup, desc, 18, 1) != 0){
        print_color("xchi: GET_DESCRIPTOR shat itself :(\n", VGA_COLOR_RED);
        xdev->used = 0;
        return -1;
    }
 
    xdev -> vendor_id  = *(u16*)(desc + 8);
    xdev -> product_id = *(u16*)(desc + 10);
    z_printf("XCHI DEVICE slot=%d vid=0x%x pid=0x%x speed=%d\n", xdev->slot_id, xdev -> vendor_id, xdev -> product_id, speed);
 
   
 
    return 0;
}
 
/* ---- controller bring-up ---- */
 
static int xchi_reset_controller(void){
    u32 cmd = xchi_op_read32(XCHI_OP_USBCMD);
    cmd &= ~XCHI_CMD_RUN;
    xchi_op_write32(XCHI_OP_USBCMD, cmd);
 
    int timeout = 1000000;
    while((xchi_op_read32(XCHI_OP_USBSTS) & XCHI_STS_HCH) == 0 && --timeout);
    if(timeout == 0){
        print_color("xchi: controller never halted :(\n", VGA_COLOR_RED);
        return -1;
    }
 
    cmd = xchi_op_read32(XCHI_OP_USBCMD);
    cmd |= XCHI_CMD_HCRESET;
    xchi_op_write32(XCHI_OP_USBCMD, cmd);
 
    timeout = 1000000;
    while((xchi_op_read32(XCHI_OP_USBCMD) & XCHI_CMD_HCRESET) && --timeout);
    if(timeout == 0){
        print_color("xchi: reset timed out :(\n", VGA_COLOR_RED);
        return -1;
    }
 
    timeout = 1000000;
    while((xchi_op_read32(XCHI_OP_USBSTS) & XCHI_STS_CNR) && --timeout);
    if(timeout == 0){
        print_color("xchi: controller not ready timeout :(\n", VGA_COLOR_RED);
        return -1;
    }
 
    print_color("xchi: reset thy controller\n", VGA_COLOR_GREEN);
    return 0;
}
 
static void xchi_setup_dcbaa(void){
    /* array of 64-bit pointers, size = (max slots + 1) * 8, 64-byte aligned */
    unsigned int bytes = (xchi_max_sluts + 1) * sizeof(u64);
    dcbaa = (u64*)dma_alloc(bytes + 64);
    dcbaa = (u64*)(((uintptr_t)dcbaa + 63) & ~(uintptr_t)63);
    for(unsigned int i = 0; i <= xchi_max_sluts; i++) dcbaa[i] = 0;
 
    xchi_op_write64(XCHI_OP_DCBAAP, XCHI_PHYS(dcbaa));
    xchi_op_write32(XCHI_OP_CONFIG, xchi_max_sluts); // MaxSlotsEn
}
 
void xchi_init(unsigned int bus, unsigned int slot, unsigned int func){
        xchi_bus = bus;
	print_color("xchi: found controller, bringing it up\n", VGA_COLOR_LIGHT_GREEN);
 
    xchi_pci_enable_busmaster((unsigned char)bus, (unsigned char)slot, (unsigned char)func);
 
    u64 bar_phys = xchi_read_bar0((unsigned char)bus, (unsigned char)slot, (unsigned char)func);
    /* same identity-map assumption as everywhere else: use the physical BAR value
       directly as the virtual MMIO pointer. if paging changes, this needs a
       phys_to_virt() (or an explicit MMIO mapping call) here. */
    xchi_base = (unsigned int)bar_phys;
    z_printf("xchi: BAR0 = 0x%x\n", (unsigned int)bar_phys);
 
    u8 caplen = xchi_cap_read8(XCHI_CAPLENGTH);
    xchi_op = xchi_base + caplen;
 
    u32 hcsparams1 = xchi_cap_read32(XCHI_HCSPARAMS1);
    xchi_max_sluts = hcsparams1 & 0xFF;
    xchi_ports = (hcsparams1 >> 24) & 0xFF;
    z_printf("xchi: max_slots=%d ports=%d\n", xchi_max_sluts, xchi_ports);
 
    u32 hccparams1 = xchi_cap_read32(XCHI_HCCPARAMS1);
    if(hccparams1 & (1 << 2)){ // CSZ bit
        xchi_context_size = 64;
        print_color("xchi: WARNING - 64-byte contexts (CSZ=1), this driver assumes 32-byte contexts, expect breakage\n", VGA_COLOR_LIGHT_RED);
    }
 
    u32 dboff = xchi_cap_read32(XCHI_DBOFF) & ~0x3;
    u32 rtsoff = xchi_cap_read32(XCHI_RTSOFF) & ~0x1F;
    xchi_db = xchi_base + dboff;
    xchi_rt = xchi_base + rtsoff;
 
    if(xchi_reset_controller() != 0)
        return;
 
    for(unsigned int i = 0; i < MAX_XCHI_DEVICES; i++) xchi_devices[i].used = 0;
 
    xchi_setup_dcbaa();
    xchi_init_command_ring();
    xchi_init_event_ring();
 
    u32 cmd = xchi_op_read32(XCHI_OP_USBCMD);
    cmd |= XCHI_CMD_RUN; // leave INTE off - polling only
    xchi_op_write32(XCHI_OP_USBCMD, cmd);
 
    int timeout = 1000000;
    while((xchi_op_read32(XCHI_OP_USBSTS) & XCHI_STS_HCH) && --timeout);
    if(timeout == 0){
        print_color("xchi: controller wouldn't start running :(\n", VGA_COLOR_RED);
        return;
    }
 
    print_color("xchi: controller is running, yayyyy\n", VGA_COLOR_GREEN);
 
    /* power on all ports - some hardware requires this explicitly before CCS is valid */
    for(unsigned int i = 0; i < xchi_ports; i++){
        u32 portsc = xchi_op_read32(XCHI_OP_PORTSC(i));
        portsc &= ~XCHI_PORTSC_WRITE_1_CLEAR_MASK; // don't accidentally clear change bits
        portsc |= XCHI_PORTSC_PP;
        xchi_op_write32(XCHI_OP_PORTSC(i), portsc);
    }
 
    for(volatile int w = 0; w < 2000000; w++); // let power settle, same style as your resetport() delay
 
    /* scan ports for already-connected devices (skeleton - enumeration wired in xchi_enumerate) */
    for(unsigned int i = 0; i < xchi_ports; i++){
        u32 portsc = xchi_op_read32(XCHI_OP_PORTSC(i));
        if(portsc & XCHI_PORTSC_CCS){
            z_printf("xchi: port %d has a device, portsc=0x%x\n", i, portsc);
            xchi_enumerate_port(i);
        }
    }
}
 

void xchi_poll(void){
    trb_t* ev;
    int guard = XCHI_RING_SIZE; // never loop more than one full ring per poll call
    while(guard-- && xchi_event_peek(&ev) == 0){
        if(TRB_GET_TYPE(ev) == TRB_TYPE_PORT_STATUS_CHANGE){
            u32 port = TRB_GET_PORT_ID(ev); // fixed: was TRB_GET_SLOT_ID, wrong field for this event type
            u32 portsc = xchi_op_read32(XCHI_OP_PORTSC(port - 1)); // PORTSC is 0-based, event port id is 1-based

            /* clear CSC (W1C) so it doesn't keep re-firing */
            xchi_op_write32(XCHI_OP_PORTSC(port - 1),
                (portsc & ~XCHI_PORTSC_WRITE_1_CLEAR_MASK) | XCHI_PORTSC_CSC);

            if(portsc & XCHI_PORTSC_CCS)
                xchi_handle_connect(port - 1);
            else
                xchi_handle_disconnect(port - 1);
        }
        xchi_event_advance();
    }
}



// delee after ts if it gives double 



/* find the xchi_device_t currently attached to a given port, if any - used on
   disconnect since PORTSC only tells us the port number, not the slot id */
static xchi_device_t* xchi_find_device_by_port(unsigned int port){
    for(unsigned int i = 0; i < MAX_XCHI_DEVICES; i++){
        if(xchi_devices[i].used && xchi_devices[i].port == port)
            return &xchi_devices[i];
    }
    return 0;
}

/* Disable Slot - tells the controller to free the slot's internal state. Do this
   BEFORE clearing our own bookkeeping, so a failed command leaves xdev->used=1
   and we don't leak/reuse a slot id the controller still thinks is active. */
static int xchi_disable_slot(xchi_device_t* xdev){
    trb_t completion;
    trb_t* trb = xchi_cmd_enqueue_slot();
    trb->param_lo = 0;
    trb->param_hi = 0;
    trb->status = 0;
    trb->control = (xdev->slot_id << 24);
    TRB_SET_TYPE(trb, TRB_TYPE_DISABLE_SLOT);

    if(xchi_wait_command_completion(trb, &completion) != 0)
        return -1;
    if(TRB_GET_COMPLETION_CODE(&completion) != 1){
        z_printf("xchi: disable slot %d failed\n", xdev->slot_id);
        return -1;
    }
    return 0;
}

/* handle a device disappearing from a port: tear down its slot and free the
   xchi_device_t. dma_alloc has no free(), so input_ctx/output_ctx/rings stay
   leaked in the DMA region - same limitation the EHCI code already has. */
static void xchi_handle_disconnect(unsigned int port){
    xchi_device_t* xdev = xchi_find_device_by_port(port);
    if(!xdev){
        z_printf("xchi: disconnect on port %d but no tracked device :shrug:\n", port);
        return;
    }
    z_printf("xchi: device on port %d (slot %d) unplugged\n", port, xdev->slot_id);

    xchi_disable_slot(xdev); // best-effort; even if it fails, drop our tracking below

    dcbaa[xdev->slot_id] = 0;
    xdev->used = 0;
    xdev->generic = 0;
}

/* handle a device appearing on a port that wasn't previously tracked */
static void xchi_handle_connect(unsigned int port){
    if(xchi_find_device_by_port(port) != 0){
        z_printf("xchi: connect event on port %d but already tracked, ignoring\n", port);
        return;
    }
    z_printf("xchi: device connected on port %d\n", port);
    xchi_enumerate_port(port);
}


// im retard and forgot ts
//
int xchi_bulk_transfer(xchi_device_t* xdev, void* buf, unsigned int len, int is_in){
    trb_t* ring;
    u32* enq_idx;
    u32* cycle;
    unsigned char dci;

    if(is_in){
        ring = xdev->bulk_in_ring;
        enq_idx = &xdev->bulk_in_enq_idx;
        cycle = &xdev->bulk_in_cycle;
        dci = xdev->bulk_in_dci;
    } else {
        ring = xdev->bulk_out_ring;
        enq_idx = &xdev->bulk_out_enq_idx;
        cycle = &xdev->bulk_out_cycle;
        dci = xdev->bulk_out_dci;
    }

    if(!ring || !dci){
        z_printf("xchi: bulk transfer on unconfigured endpoint :(\n");
        return -1;
    }

    trb_t* trb = &ring[*enq_idx];
    u64 phys = XCHI_PHYS(buf);
    trb->param_lo = (u32)(phys & 0xFFFFFFFF);
    trb->param_hi = (u32)(phys >> 32);
    trb->status = len;
    trb->control = TRB_IOC;
    TRB_SET_TYPE(trb, TRB_TYPE_NORMAL);
    trb->control = (trb->control & ~TRB_CYCLE) | (*cycle & TRB_CYCLE);

    (*enq_idx)++;
    if(*enq_idx == XCHI_RING_SIZE - 1){
        ring[XCHI_RING_SIZE - 1].control = (ring[XCHI_RING_SIZE - 1].control & ~TRB_CYCLE) | (*cycle & TRB_CYCLE);
        *enq_idx = 0;
        *cycle ^= 1;
    }

    xchi_db_write(xdev->slot_id, dci); // doorbell target = DCI for non-control endpoints

    trb_t completion;
    if(xchi_wait_transfer_completion(&completion) != 0)
        return -1;

    u32 cc = TRB_GET_COMPLETION_CODE(&completion);
    if(cc != 1 && cc != 13 /* SHORT_PACKET */){
        z_printf("xchi: bulk transfer failed, completion code=%d\n", cc);
        return -1;
    }
    return 0;
}
