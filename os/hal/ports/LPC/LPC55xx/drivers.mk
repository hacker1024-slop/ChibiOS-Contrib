LPC55_PORT := $(CHIBIOS_CONTRIB)/os/hal/ports/LPC/LPC55xx
PLATFORMSRC += $(LPC55_PORT)/hal_i2c_lld.c
PLATFORMINC += $(LPC55_PORT)
include $(CHIBIOS_CONTRIB)/os/hal/ports/LPC/LLD/FLASHv1/driver.mk
