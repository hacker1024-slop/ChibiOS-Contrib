#include "hal.h"

#if HAL_USE_USB == TRUE
#include <string.h>

#include "lpc55_usb_phy.h"

#define USB_RAM __attribute__((section(".usb_ram")))
#define PACKET_CAPACITY 512u

USBDriver USBD1;
static USB_RAM __attribute__((aligned(256))) ip3511_hs_endpoint_list_t endpoints;
static USB_RAM __attribute__((aligned(64))) uint8_t setup_packet[8];
static USB_RAM __attribute__((aligned(64))) uint8_t packets[IP3511_HS_ENDPOINTS * 2][PACKET_CAPACITY];

static uint32_t endpoint_type(const USBEndpointConfig *config) {
    return (config->ep_mode & USB_EP_MODE_TYPE) == USB_EP_MODE_TYPE_INTR ?
           IP3511_HS_EP_INTERRUPT_TYPE : 0;
}

static void arm_packet(USBDriver *usbp, usbep_t ep, bool in, uint16_t size) {
    unsigned index = ip3511_hs_index(ep, in);
    uint32_t command = 0;
    bool valid = ip3511_hs_buffer_command((uintptr_t)packets[index], size, &command);
    osalDbgAssert(valid && size <= PACKET_CAPACITY, "invalid USB packet");
    if (valid && size <= PACKET_CAPACITY) {
        __asm__ volatile("dmb" ::: "memory");
        endpoints.buffer[index][0] = command | endpoint_type(usbp->epc[ep]) | IP3511_HS_EP_ACTIVE;
    }
}

void usb_lld_start_in(USBDriver *usbp, usbep_t ep) {
    const USBEndpointConfig *config = usbp->epc[ep];
    USBInEndpointState *state = config->in_state;
    size_t remaining = state->txsize - state->txcnt;
    state->packet_size = remaining < config->in_maxsize ? remaining : config->in_maxsize;
    if (state->packet_size != 0) {
        memcpy(packets[ip3511_hs_index(ep, true)], state->txbuf + state->txcnt, state->packet_size);
    }
    arm_packet(usbp, ep, true, state->packet_size);
}

void usb_lld_start_out(USBDriver *usbp, usbep_t ep) {
    const USBEndpointConfig *config = usbp->epc[ep];
    USBOutEndpointState *state = config->out_state;
    size_t remaining = state->rxsize - state->rxcnt;
    state->packet_size = remaining < config->out_maxsize ? remaining : config->out_maxsize;
    arm_packet(usbp, ep, false, state->packet_size);
}

static void complete_in(USBDriver *usbp, usbep_t ep) {
    USBInEndpointState *state = usbp->epc[ep]->in_state;
    state->txcnt += state->packet_size;
    if (state->txcnt < state->txsize) {
        usb_lld_start_in(usbp, ep);
    } else {
        _usb_isr_invoke_in_cb(usbp, ep);
    }
}

static void complete_out(USBDriver *usbp, usbep_t ep) {
    unsigned index = ip3511_hs_index(ep, false);
    const USBEndpointConfig *config = usbp->epc[ep];
    USBOutEndpointState *state = config->out_state;
    uint16_t remaining = ip3511_hs_remaining(endpoints.buffer[index][0]);
    if (remaining > state->packet_size) {
        return;
    }
    uint16_t received = state->packet_size - remaining;
    if (received != 0) {
        memcpy(state->rxbuf + state->rxcnt, packets[index], received);
    }
    state->rxcnt += received;
    if (received == config->out_maxsize && state->rxcnt < state->rxsize) {
        usb_lld_start_out(usbp, ep);
    } else {
        _usb_isr_invoke_out_cb(usbp, ep);
    }
}

void usb_lld_read_setup(USBDriver *usbp, usbep_t ep, uint8_t *buffer) {
    (void)usbp;
    (void)ep;
    memcpy(buffer, setup_packet, sizeof(setup_packet));
}

static void service_endpoint_interrupts(USBDriver *usbp, uint32_t interrupts) {
    for (usbep_t ep = 0; ep <= USB_MAX_ENDPOINTS; ++ep) {
        const USBEndpointConfig *config = usbp->epc[ep];
        if (config == NULL) {
            continue;
        }
        if (config->out_state != NULL && (interrupts & (1u << ip3511_hs_index(ep, false)))) {
            complete_out(usbp, ep);
        }
        if (config->in_state != NULL && (interrupts & (1u << ip3511_hs_index(ep, true)))) {
            complete_in(usbp, ep);
        }
    }
}

void ip3511_hs_serve_interrupt(USBDriver *usbp) {
    uint32_t state = IP3511_HS->command_status;
    uint32_t interrupts = IP3511_HS->interrupt_status & IP3511_HS->interrupt_enable;
    IP3511_HS->interrupt_status = interrupts;

    if (interrupts & IP3511_HS_DEVICE_INTERRUPT) {
        ip3511_hs_update_device(0, 0, state & (IP3511_HS_CHANGE_FLAGS & ~IP3511_HS_SETUP_RECEIVED));
        if (state & IP3511_HS_RESET_CHANGE) {
            _usb_reset(usbp);
            return;
        }
        if (state & IP3511_HS_SUSPEND_CHANGE) {
            if (state & IP3511_HS_SUSPENDED) {
                _usb_suspend(usbp);
            } else {
                _usb_wakeup(usbp);
            }
        }
    }
    if ((interrupts & 1u) && (state & IP3511_HS_SETUP_RECEIVED)) {
        endpoints.buffer[0][0] = 0;
        endpoints.buffer[1][0] = 0;
        ip3511_hs_update_device(0, 0, IP3511_HS_SETUP_RECEIVED);
        _usb_isr_invoke_setup_cb(usbp, 0);
        interrupts &= ~3u;
    }
    service_endpoint_interrupts(usbp, interrupts);
    if (interrupts & IP3511_HS_SOF_INTERRUPT) {
        _usb_isr_invoke_sof_cb(usbp);
    }
}

void usb_lld_init(void) {
    usbObjectInit(&USBD1);
}

void usb_lld_start(USBDriver *usbp) {
    if (usbp->state == USB_STOP) {
        lpc55_usb_phy_init();
        usb_lld_reset(usbp);
        ip3511_hs_update_device(IP3511_HS_DEVICE_ENABLE, IP3511_HS_DEVICE_ENABLE,
                                IP3511_HS_CHANGE_FLAGS);
        nvicEnableVector(47, LPC55_USB_IRQ_PRIORITY);
    }
}

void usb_lld_stop(USBDriver *usbp) {
    (void)usbp;
    nvicDisableVector(47);
    IP3511_HS->interrupt_enable = 0;
    ip3511_hs_update_device(IP3511_HS_DEVICE_ENABLE | IP3511_HS_CONNECT, 0,
                            IP3511_HS_CHANGE_FLAGS);
}

void usb_lld_reset(USBDriver *usbp) {
    static USBInEndpointState ep0_in;
    static USBOutEndpointState ep0_out;
    static const USBEndpointConfig ep0 = {
        USB_EP_MODE_TYPE_CTRL,
        _usb_ep0setup, _usb_ep0in, _usb_ep0out,
        64, 64, &ep0_in, &ep0_out, 1, NULL
    };
    bool valid = ip3511_hs_reset_endpoints(&endpoints, (uintptr_t)setup_packet);
    osalDbgAssert(valid, "invalid USB RAM placement");
    (void)valid;
    usbp->epc[0] = &ep0;
    if (usbp->config->sof_cb != NULL) {
        IP3511_HS->interrupt_enable |= IP3511_HS_SOF_INTERRUPT;
    }
}

void usb_lld_set_address(USBDriver *usbp) {
    ip3511_hs_update_device(IP3511_HS_ADDRESS_MASK, usbp->address, 0);
}

void usb_lld_init_endpoint(USBDriver *usbp, usbep_t ep) {
    const USBEndpointConfig *config = usbp->epc[ep];
    unsigned out = ip3511_hs_index(ep, false);
    unsigned in = ip3511_hs_index(ep, true);
    if (config->out_state != NULL) {
        endpoints.buffer[out][0] = endpoint_type(config);
        IP3511_HS->interrupt_enable |= 1u << out;
    }
    if (config->in_state != NULL) {
        endpoints.buffer[in][0] = endpoint_type(config);
        IP3511_HS->interrupt_enable |= 1u << in;
    }
}

void usb_lld_disable_endpoints(USBDriver *usbp) {
    (void)usbp;
    uint32_t mask = ((1u << (2 * IP3511_HS_ENDPOINTS)) - 1) & ~3u;
    IP3511_HS->interrupt_enable &= ~mask;
    IP3511_HS->skip = mask;
    for (unsigned index = 2; index < 2 * IP3511_HS_ENDPOINTS; ++index) {
        endpoints.buffer[index][0] = IP3511_HS_EP_DISABLED;
    }
    IP3511_HS->interrupt_status = mask;
}

static usbepstatus_t endpoint_status(unsigned index) {
    uint32_t command = endpoints.buffer[index][0];
    if (command & IP3511_HS_EP_DISABLED) {
        return EP_STATUS_DISABLED;
    }
    if (command & IP3511_HS_EP_STALL) {
        return EP_STATUS_STALLED;
    }
    return EP_STATUS_ACTIVE;
}

usbepstatus_t usb_lld_get_status_in(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    return endpoint_status(ip3511_hs_index(ep, true));
}

usbepstatus_t usb_lld_get_status_out(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    return endpoint_status(ip3511_hs_index(ep, false));
}

static void stall_endpoint(unsigned index) {
    IP3511_HS->skip = 1u << index;
    endpoints.buffer[index][0] |= IP3511_HS_EP_STALL;
}

static void clear_endpoint(unsigned index) {
    endpoints.buffer[index][0] = (endpoints.buffer[index][0] & ~IP3511_HS_EP_STALL) |
                                 IP3511_HS_EP_RESET_TOGGLE;
}

void usb_lld_stall_in(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    stall_endpoint(ip3511_hs_index(ep, true));
}

void usb_lld_stall_out(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    stall_endpoint(ip3511_hs_index(ep, false));
}

void usb_lld_clear_in(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    clear_endpoint(ip3511_hs_index(ep, true));
}

void usb_lld_clear_out(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    clear_endpoint(ip3511_hs_index(ep, false));
}

OSAL_IRQ_HANDLER(VectorFC) {
    OSAL_IRQ_PROLOGUE();
    ip3511_hs_serve_interrupt(&USBD1);
    OSAL_IRQ_EPILOGUE();
}

#endif
