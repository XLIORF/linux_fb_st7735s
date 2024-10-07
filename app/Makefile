# CC = /home/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc
CFLAGS = -Wall -Wextra -Werror

PROJECT_DIR := $(shell pwd)
SRCDIR = ${PROJECT_DIR}
BUILD := ${PROJECT_DIR}/build
OBJDIR = ${BUILD}/obj
BIN_DIR := ${BUILD}/bin
TARGET = app_st775s
 
SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))
 
$(TARGET): out_dir $(OBJECTS) 
	$(CC) $(CFLAGS) $(word 2, $^) -o ${BIN_DIR}/$@	# 只能在Linux下有效
 
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
 
out_dir: 
	mkdir -p ${OBJDIR}
	mkdir -p ${BIN_DIR}
 
upload: $(TARGET)
	scp ${BIN_DIR}/${TARGET} root@172.32.0.93:/root

.PHONY: clean
clean:
	rm -rf ${BUILD}