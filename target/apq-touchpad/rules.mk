LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include -I$(LK_TOP_DIR)/platform/msm_shared

PLATFORM := apq-touchpad

#MEMBASE := 0x40208000 		# kernel load address
MEMBASE := 0x50000000 		# kernel load address
MEMSIZE := 0x00400000 		# 4MB

SCRATCH_ADDR := 0x70000000
#SCRATCH_ADDR := 0x51000000
SCRATCH_SIZE := 64	 		#size in MB

KEYS_USE_GPIO_KEYPAD := 1

MODULES += \
	dev/ssbi \
	dev/pmic/pm8921 \
	lib/ptable

DEFINES += \
	MEMSIZE=$(MEMSIZE) \
	MEMBASE=$(MEMBASE) \
	SCRATCH_ADDR=$(SCRATCH_ADDR) \
	SCRATCH_SIZE=$(SCRATCH_SIZE)

OBJS += \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/atags.o \
	$(LOCAL_DIR)/keypad.o \
	$(LOCAL_DIR)/gpio.o \
	$(LOCAL_DIR)/gpiokeys.o
