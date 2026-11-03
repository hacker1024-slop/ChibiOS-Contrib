#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LPC_FLASH_PAGE_SIZE 512u

/* Address is a physical flash offset aligned to a page boundary. */
bool lpc_flash_read_page(uint32_t address, void *page);
bool lpc_flash_write_page(uint32_t address, const void *page);
