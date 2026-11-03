#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LPC_REG(address) (*(volatile uint32_t *)(address))

static inline void lpc_gpio_input(uint8_t pin, uint32_t pull) {
    LPC_REG(0x4008e000u + 4u * (pin / 32)) &= ~(1u << (pin % 32));
    LPC_REG(0x40001000u + 4u * pin) = (1u << 8) | (pull << 4);
}

static inline void lpc_gpio_output(uint8_t pin) {
    LPC_REG(0x40001000u + 4u * pin) = 1u << 8;
    LPC_REG(0x4008e000u + 4u * (pin / 32)) |= 1u << (pin % 32);
}

static inline bool lpc_gpio_read(uint8_t pin) {
    return (LPC_REG(0x4008e100u + 4u * (pin / 32)) & (1u << (pin % 32))) != 0;
}

static inline void lpc_gpio_write(uint8_t pin, bool high) {
    uintptr_t base = high ? 0x4008e200u : 0x4008e280u;
    LPC_REG(base + 4u * (pin / 32)) = 1u << (pin % 32);
}
