#include "hal.h"

void hal_lld_init(void) {
    nvicInit();
}
