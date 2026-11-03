#pragma once

#include <stdbool.h>
#include <stdint.h>

#define IP3511_HS_ENDPOINTS 6u
#define IP3511_HS_DMA_BASE UINT32_C(0x40100000)
#define IP3511_HS_DMA_SIZE UINT32_C(0x4000)

typedef struct {
    volatile uint32_t command_status;
    volatile const uint32_t info;
    volatile uint32_t endpoint_list;
    volatile uint32_t data_buffer_base;
    volatile uint32_t lpm;
    volatile uint32_t skip;
    volatile uint32_t in_use;
    volatile uint32_t buffer_config;
    volatile uint32_t interrupt_status;
    volatile uint32_t interrupt_enable;
} ip3511_hs_registers_t;

#define IP3511_HS ((ip3511_hs_registers_t *)UINT32_C(0x40094000))
#define IP3511_HS_DEVICE_INTERRUPT (UINT32_C(1) << 31)
#define IP3511_HS_SOF_INTERRUPT (UINT32_C(1) << 30)
#define IP3511_HS_EP_ACTIVE (UINT32_C(1) << 31)
#define IP3511_HS_EP_DISABLED (UINT32_C(1) << 30)
#define IP3511_HS_EP_STALL (UINT32_C(1) << 29)
#define IP3511_HS_EP_RESET_TOGGLE (UINT32_C(1) << 28)
#define IP3511_HS_EP_INTERRUPT_TYPE (UINT32_C(3) << 26)

typedef struct {
    volatile uint32_t buffer[IP3511_HS_ENDPOINTS * 2][2];
} ip3511_hs_endpoint_list_t;

static inline unsigned ip3511_hs_index(unsigned endpoint, bool in) {
    return 2 * endpoint + in;
}

/* The DMA address is 64-byte aligned and the whole transfer is in USB SRAM. */
static inline bool ip3511_hs_buffer_command(uintptr_t address, uint16_t length,
                                            uint32_t *command) {
    if (address < IP3511_HS_DMA_BASE ||
        address >= IP3511_HS_DMA_BASE + IP3511_HS_DMA_SIZE ||
        (address & 63u) != 0 ||
        length > IP3511_HS_DMA_BASE + IP3511_HS_DMA_SIZE - address) {
        return false;
    }
    *command = ((address - IP3511_HS_DMA_BASE) >> 6) | ((uint32_t)length << 11);
    return true;
}

static inline uint16_t ip3511_hs_remaining(uint32_t command) {
    return (command >> 11) & 0x7fffu;
}

#define IP3511_HS_ADDRESS_MASK UINT32_C(0x7f)
#define IP3511_HS_DEVICE_ENABLE (UINT32_C(1) << 7)
#define IP3511_HS_SETUP_RECEIVED (UINT32_C(1) << 8)
#define IP3511_HS_CONNECT (UINT32_C(1) << 16)
#define IP3511_HS_SUSPENDED (UINT32_C(1) << 17)
#define IP3511_HS_CONNECT_CHANGE (UINT32_C(1) << 24)
#define IP3511_HS_SUSPEND_CHANGE (UINT32_C(1) << 25)
#define IP3511_HS_RESET_CHANGE (UINT32_C(1) << 26)
#define IP3511_HS_CHANGE_FLAGS (IP3511_HS_SETUP_RECEIVED | IP3511_HS_CONNECT_CHANGE | \
                                IP3511_HS_SUSPEND_CHANGE | IP3511_HS_RESET_CHANGE)

/* Setup DMA always uses the second buffer descriptor for control OUT. */
bool ip3511_hs_reset_endpoints(ip3511_hs_endpoint_list_t *list, uintptr_t setup);
void ip3511_hs_update_device(uint32_t mask, uint32_t value, uint32_t acknowledge);
