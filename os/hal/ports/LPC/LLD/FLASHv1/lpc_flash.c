#include "lpc_flash.h"

#include <string.h>

#define FLASH_REG(offset) (*(volatile uint32_t *)(0x40034000u + (offset)))
enum {
    FLASH_COMMAND = 0, FLASH_START = 0x10, FLASH_STOP = 0x14,
    FLASH_DATA = 0x80, FLASH_STATUS = 0xfe0, FLASH_CLEAR = 0xfe8,
    FLASH_SET_READ_MODE = 2, FLASH_READ = 3, FLASH_ERASE = 4, FLASH_LOAD = 8, FLASH_PROGRAM = 12,
    FLASH_DONE = 4, FLASH_STATUS_MASK = 15, FLASH_CLEAR_ALL_STATUS = 31,
    FLASH_READ_TIMING_MASK = 0xf,
    WORD_SIZE = 16,
};

static bool execute_command(unsigned opcode) {
    FLASH_REG(FLASH_CLEAR) = FLASH_CLEAR_ALL_STATUS;
    FLASH_REG(FLASH_COMMAND) = opcode;
    for (unsigned poll = 0; poll < 100000; ++poll) {
        unsigned status = FLASH_REG(FLASH_STATUS) & FLASH_STATUS_MASK;
        if (status & FLASH_DONE) {
            return status == FLASH_DONE;
        }
    }
    return false;
}

static bool command(unsigned opcode, unsigned address, unsigned stop) {
    FLASH_REG(FLASH_START) = address / WORD_SIZE;
    FLASH_REG(FLASH_STOP) = stop / WORD_SIZE;
    return execute_command(opcode);
}

bool lpc_flash_set_read_timing(unsigned timing) {
    FLASH_REG(FLASH_DATA) = (FLASH_REG(FLASH_DATA) & ~FLASH_READ_TIMING_MASK) |
                            (timing & FLASH_READ_TIMING_MASK);
    return execute_command(FLASH_SET_READ_MODE);
}

bool lpc_flash_read_page(uint32_t address, void *page) {
    for (size_t offset = 0; offset < LPC_FLASH_PAGE_SIZE; offset += WORD_SIZE) {
        if (!command(FLASH_READ, address + offset, address + offset)) {
            return false;
        }
        for (size_t i = 0; i < WORD_SIZE; i += sizeof(uint32_t)) {
            uint32_t value = FLASH_REG(FLASH_DATA + i);
            memcpy((uint8_t *)page + offset + i, &value, sizeof(value));
        }
    }
    return true;
}

bool lpc_flash_write_page(uint32_t address, const void *page) {
    if (!command(FLASH_ERASE, address, address)) {
        return false;
    }
    for (size_t offset = 0; offset < LPC_FLASH_PAGE_SIZE; offset += WORD_SIZE) {
        for (size_t i = 0; i < WORD_SIZE; i += sizeof(uint32_t)) {
            uint32_t value;
            memcpy(&value, (const uint8_t *)page + offset + i, sizeof(value));
            FLASH_REG(FLASH_DATA + i) = value;
        }
        if (!command(FLASH_LOAD, address + offset, address + offset)) {
            return false;
        }
    }
    return command(FLASH_PROGRAM, address, address);
}
