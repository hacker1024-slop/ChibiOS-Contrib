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
