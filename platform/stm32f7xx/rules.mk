LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE ?= 0x08000000
MEMBASE ?= 0x20010000
# default memsize, specific STM32_CHIP may override this
# and target/project may have already overridden
MEMSIZE ?= 0x40000

ARCH := arm
ARM_CPU := cortex-m7

ifeq ($(STM32_CHIP),stm32f746)
GLOBAL_DEFINES += STM32F746xx
# XXX workaround for uppercasing in GLOBAL_DEFINES
GLOBAL_COMPILEFLAGS += -DSTM32F746xx
FOUND_CHIP := true
endif

ifeq ($(FOUND_CHIP),)
$(error unknown STM32F7xx chip $(STM32_CHIP))
endif

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/uart.c 

# use a two segment memory layout, where all of the read-only sections 
# of the binary reside in rom, and the read/write are in memory. The 
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set 
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	arch/arm/arm-m/systick \
	lib/cbuf

include $(LOCAL_DIR)/STM32F7xx_HAL_Driver/rules.mk $(LOCAL_DIR)/CMSIS/rules.mk

include make/module.mk