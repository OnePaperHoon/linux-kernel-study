# Week 5 Day 4: Character Device Driver
# Makefile for chardev_basic module

obj-m := chardev_basic.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	@echo "========================================="
	@echo "Building chardev_basic.ko..."
	@echo "========================================="
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo ""
	@echo "Build complete!"

clean:
	@echo "Cleaning..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 모듈 로드
load:
	@echo "========================================="
	@echo "Loading module..."
	@echo "========================================="
	sudo insmod chardev_basic.ko
	@echo ""
	@echo "Check dmesg for major number:"
	dmesg | tail -10

# 디바이스 파일 생성
create-dev:
	@echo "========================================="
	@echo "Creating device file..."
	@echo "========================================="
	@echo "Check dmesg for major number:"
	@dmesg | grep "chardev: Registered" | tail -1
	@echo ""
	@echo "Enter major number manually:"
	@read -p "Major number: " MAJOR; \
	sudo mknod /dev/onepaperhoon_dev c $$MAJOR 0; \
	sudo chmod 666 /dev/onepaperhoon_dev
	@ls -la /dev/onepaperhoon_dev

# 디바이스 테스트
test-write:
	@echo "========================================="
	@echo "Writing to device..."
	@echo "========================================="
	@echo "Hello from OnePaperHoon!" | sudo tee /dev/onepaperhoon_dev
	@echo ""
	@dmesg | tail -5

test-read:
	@echo "========================================="
	@echo "Reading from device..."
	@echo "========================================="
	sudo cat /dev/onepaperhoon_dev
	@echo ""
	@dmesg | tail -5

# 디바이스 파일 삭제
remove-dev:
	@echo "Removing device file..."
	sudo rm -f /dev/onepaperhoon_dev

# 모듈 언로드
unload:
	@echo "========================================="
	@echo "Unloading module..."
	@echo "========================================="
	sudo rmmod chardev_basic
	@dmesg | tail -5

# 모듈 정보
info:
	modinfo chardev_basic.ko

help:
	@echo "Available targets:"
	@echo "  make              - Build module"
	@echo "  make load         - Load module"
	@echo "  make create-dev   - Create device file"
	@echo "  make test-write   - Write test"
	@echo "  make test-read    - Read test"
	@echo "  make remove-dev   - Remove device file"
	@echo "  make unload       - Unload module"
	@echo "  make clean        - Clean build files"

.PHONY: all clean load create-dev test-write test-read remove-dev unload info help

