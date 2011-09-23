LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/adler32.o \
	$(LOCAL_DIR)/compress.o \
	$(LOCAL_DIR)/crc32.o \
	$(LOCAL_DIR)/deflate.o \
	$(LOCAL_DIR)/infback.o \
	$(LOCAL_DIR)/inffast.o \
	$(LOCAL_DIR)/inflate.o \
	$(LOCAL_DIR)/inftrees.o \
	$(LOCAL_DIR)/trees.o \
	$(LOCAL_DIR)/uncompr.o \
	$(LOCAL_DIR)/zutil.o \
