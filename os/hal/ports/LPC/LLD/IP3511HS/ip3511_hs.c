#include "ip3511_hs.h"

bool ip3511_hs_reset_endpoints(ip3511_hs_endpoint_list_t *list, uintptr_t setup) {
    uintptr_t list_address = (uintptr_t)list;
    uint32_t setup_command;
    if (list_address < IP3511_HS_DMA_BASE ||
        list_address > IP3511_HS_DMA_BASE + IP3511_HS_DMA_SIZE - sizeof(*list) ||
        (list_address & 255u) != 0 ||
        !ip3511_hs_buffer_command(setup, 8, &setup_command)) {
        return false;
    }

    IP3511_HS->interrupt_enable = 0;
    IP3511_HS->endpoint_list = list_address;
    IP3511_HS->data_buffer_base = IP3511_HS_DMA_BASE & ~UINT32_C(0x3fffff);
    IP3511_HS->in_use = 0;
    IP3511_HS->buffer_config = 0;
    IP3511_HS->skip = (1u << (IP3511_HS_ENDPOINTS * 2)) - 1;

    for (unsigned index = 0; index < IP3511_HS_ENDPOINTS * 2; ++index) {
        uint32_t state = index < 2 ? 0 : IP3511_HS_EP_DISABLED;
        list->buffer[index][0] = state;
        list->buffer[index][1] = state;
    }
    list->buffer[0][1] = setup_command & 0x7ffu;
    IP3511_HS->interrupt_status = IP3511_HS->interrupt_status;
    ip3511_hs_update_device(0, 0, IP3511_HS_SETUP_RECEIVED);
    IP3511_HS->interrupt_enable = IP3511_HS_DEVICE_INTERRUPT | 3u;
    return true;
}

void ip3511_hs_update_device(uint32_t mask, uint32_t value, uint32_t acknowledge) {
    /* Preserve pending events unless the caller explicitly acknowledges them. */
    uint32_t state = IP3511_HS->command_status & ~IP3511_HS_CHANGE_FLAGS;
    IP3511_HS->command_status = (state & ~mask) | (value & mask) |
                                (acknowledge & IP3511_HS_CHANGE_FLAGS);
}
