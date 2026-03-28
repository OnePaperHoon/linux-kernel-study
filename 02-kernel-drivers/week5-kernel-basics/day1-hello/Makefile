# Week 5 Day 1: Hello World Kernel Module
# Makefile for building kernel module

# 모듈 이름 (hello.c → hello.ko)
obj-m := hello.o

# 커널 빌드 디렉토리
KDIR := /lib/modules/$(shell uname -r)/build

# 현재 디렉토리
PWD := $(shell pwd)

# 기본 타겟: 모듈 빌드
all:
	@echo "========================================="
	@echo "Building hello.ko kernel module..."
	@echo "========================================="
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo ""
	@echo "Build complete! Generated files:"
	@ls -lh hello.ko 2>/dev/null || echo "Build failed!"

# 빌드 파일 정리
clean:
	@echo "Cleaning build files..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@echo "Clean complete!"

# 모듈 로드
load:
	@echo "========================================="
	@echo "Loading kernel module..."
	@echo "========================================="
	sudo insmod hello.ko
	@echo ""
	@echo "Kernel log (last 15 lines):"
	@echo "========================================="
	dmesg | tail -15

# 모듈 언로드
unload:
	@echo "========================================="
	@echo "Unloading kernel module..."
	@echo "========================================="
	sudo rmmod hello
	@echo ""
	@echo "Kernel log (last 15 lines):"
	@echo "========================================="
	dmesg | tail -15

# 모듈 정보 출력
info:
	@echo "========================================="
	@echo "Module Information:"
	@echo "========================================="
	modinfo hello.ko

# 로드된 모듈 확인
list:
	@echo "========================================="
	@echo "Loaded modules (filtering 'hello'):"
	@echo "========================================="
	lsmod | grep hello || echo "Module not loaded"

# 전체 커널 로그 확인
log:
	@echo "========================================="
	@echo "Recent kernel messages:"
	@echo "========================================="
	dmesg | tail -30

# 도움말
help:
	@echo "Available targets:"
	@echo "  make        - Build kernel module"
	@echo "  make clean  - Clean build files"
	@echo "  make load   - Load module into kernel"
	@echo "  make unload - Remove module from kernel"
	@echo "  make info   - Show module information"
	@echo "  make list   - List loaded modules"
	@echo "  make log    - Show recent kernel log"
	@echo "  make help   - Show this help"

.PHONY: all clean load unload info list log help
