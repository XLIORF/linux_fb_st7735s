# Linu内核目录配置
SDK_DIR = /home
KERN_DIR = $(SDK_DIR)/sysdrv/source/kernel

# 工程文件配置
PROJECT_NAME := st7735s
PROJCET_DIR := $(shell pwd)

SRC_DIR := ${PROJCET_DIR}/src
BUILD_DIR := ${PROJCET_DIR}/build
OBJ_DIR := $(BUILD_DIR)/obj

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))


obj-m += ${PROJECT_NAME}.o
CROSS_COMPILE :=$(SDK_DIR)/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-
CC := $(CROSS_COMPILE)gcc
ARCH := arm

# 编译标志
EXTRA_CFLAGS += -I$(PROJCET_DIR) -I$(PROJCET_DIR)/../include/
DTS_INCLUDE := -I /home/sysdrv/source/kernel/arch/arm/boot/dts \
			-I /home/sysdrv/source/kernel/include \
			-I /home/media/sysutils/include


all: module app

module:
	bear -- make -C src/driver ARCH=${ARCH} CROSS_COMPILE=$(CROSS_COMPILE) $(INCLUDES) KERN_DIR=$(KERN_DIR)

app: 
	bear -- make -C src/app CC=${CC}

.PHONY: upload
upload:
	make -C src/app CC=${CC} upload
	make -C src/driver CC=${CC} upload

.PHONY: clean
clean:
	# make ARCH=${ARCH} CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules clean
	make -C src/app CC=${CC} clean
	make -C src/driver CC=${CC} clean