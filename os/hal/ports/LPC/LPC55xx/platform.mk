include $(CHIBIOS_CONTRIB)/os/hal/ports/LPC/LPC55xx/drivers.mk
include $(CHIBIOS_CONTRIB)/os/hal/ports/LPC/LLD/IP3511HS/driver.mk

PLATFORMSRC += $(LPC55_PORT)/hal_lld.c $(LPC55_PORT)/hal_st_lld.c
PLATFORMSRC += $(LPC55_PORT)/lpc55_usb_phy.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/common/ARMCMx/nvic.c
PLATFORMINC += $(CHIBIOS)/os/hal/ports/common/ARMCMx
PLATFORMINC += $(CHIBIOS_CONTRIB)/os/common/startup/ARMCMx/devices/LPC55xx
PLATFORMINC += $(CHIBIOS)/os/common/ext/ARM/CMSIS/Core/Include

ALLCSRC += $(PLATFORMSRC)
ALLINC += $(PLATFORMINC)
