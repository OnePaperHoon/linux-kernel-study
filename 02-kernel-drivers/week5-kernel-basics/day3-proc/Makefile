# Week 5 Day 3: /proc filesystem
# Makefile for proc_basic module

obj-m := proc_basic.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	@echo "========================================="
	@echo "Building proc_basic.ko kernel module..."
	@echo "========================================="
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo ""
	@echo "Build complete!"
	@ls -lh proc_basic.ko 2>/dev/null || echo "Build failed!"

clean:
	@echo "Cleaning build files..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@echo "Clean complete!"

# 모듈 로드
load:
	@echo "========================================="
	@echo "Loading module..."
	@echo "========================================="
	sudo insmod proc_basic.ko
	@echo ""
	@echo "Module loaded! Checking dmesg:"
	dmesg | tail -10

# 모듈 언로드
unload:
	@echo "========================================="
	@echo "Unloading module..."
	@echo "========================================="
	sudo rmmod proc_basic
	@echo ""
	@echo "Module unloaded! Checking dmesg:"
	dmesg | tail -5

# /proc 파일 확인
check:
	@echo "========================================="
	@echo "Checking /proc file..."
	@echo "========================================="
	@ls -la /proc/onepaperhoon_info 2>/dev/null || echo "File not found!"
	@echo ""
	@echo "Reading /proc file:"
	@cat /proc/onepaperhoon_info 2>/dev/null || echo "Cannot read file!"

# /proc 파일 읽기
read:
	@cat /proc/onepaperhoon_info

# 카운터 리셋
reset:
	@echo "========================================="
	@echo "Resetting counter..."
	@echo "========================================="
	@echo "reset" | sudo tee /proc/onepaperhoon_info > /dev/null
	@echo ""
	@echo "Counter reset! Reading:"
	@cat /proc/onepaperhoon_info

# 모듈 정보
info:
	@echo "========================================="
	@echo "Module Information:"
	@echo "========================================="
	modinfo proc_basic.ko

# 로그 확인
log:
	@echo "========================================="
	@echo "Kernel log (last 20 lines):"
	@echo "========================================="
	dmesg | tail -20

help:
	@echo "Available targets:"
	@echo "  make         - Build module"
	@echo "  make clean   - Clean build files"
	@echo "  make load    - Load module"
	@echo "  make unload  - Unload module"
	@echo "  make check   - Check /proc file"
	@echo "  make read    - Read /proc file"
	@echo "  make reset   - Reset counter"
	@echo "  make info    - Show module info"
	@echo "  make log     - Show kernel log"

.PHONY: all clean load unload check read reset info log help
