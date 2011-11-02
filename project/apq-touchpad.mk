# top level project rules for the apq8064 project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := apq-touchpad

MODULES += app/moboot
MODULES += lib/bootlinux
MODULES += lib/uimage
MODULES += lib/atags
MODULES += lib/zlib
MODULES += lib/fs
MODULES += lib/fs/ext2
MODULES += lib/gfx
MODULES += lib/gfxconsole
MODULES += lib/tga
MODULES += lib/ramdisk

DEBUG := 0

#DEFINES += WITH_DEBUG_DCC=1
#DEFINES += WITH_DEBUG_UART=1
DEFINES += WITH_DEBUG_FBCON=1
DEFINES += WITH_DEV_FBCON=1
DEFINES += DISPLAY_TYPE_TOUCHPAD=1
DEFINES += _EMMC_BOOT=1
DEFINES += DISABLE_MMC_DEBUG_SPEW
DEFINES += HANDLE_LINUX_KERNEL_ARGS
