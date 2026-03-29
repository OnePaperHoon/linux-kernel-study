# Week 5 Day 2: Module Parameters
# Makefile for params module

obj-m := params.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	@echo "========================================="
	@echo "Building params.ko kernel module..."
	@echo "========================================="
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo ""
	@echo "Build complete! Generated files:"
	@ls -lh params.ko 2>/dev/null || echo "Build failed!"

clean:
	@echo "Cleaning build files..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@echo "Clean complete!"

# 파라미터 없이 로드
load:
	@echo "========================================="
	@echo "Loading module (default parameters)..."
	@echo "========================================="
	sudo insmod params.ko
	@echo ""
	@echo "Kernel log:"
	dmesg | tail -15

# 파라미터와 함께 로드
load-custom:
	@echo "========================================="
	@echo "Loading module with custom parameters..."
	@echo "========================================="
	sudo insmod params.ko count=3 name="TNS_Lab" debug=true
	@echo ""
	@echo "Kernel log:"
	dmesg | tail -15

# 언로드
unload:
	@echo "========================================="
	@echo "Unloading module..."
	@echo "========================================="
	sudo rmmod params
	@echo ""
	@echo "Kernel log:"
	dmesg | tail -10

# 모듈 정보
info:
	@echo "========================================="
	@echo "Module Information:"
	@echo "========================================="
	modinfo params.ko

# /sys 확인
sysfs:
	@echo "========================================="
	@echo "Module parameters in /sys:"
	@echo "========================================="
	@ls -la /sys/module/params/parameters/ 2>/dev/null || echo "Module not loaded"
	@echo ""
	@echo "Parameter values:"
	@cat /sys/module/params/parameters/* 2>/dev/null || echo "Module not loaded"

# 파라미터 런타임 변경
change-param:
	@echo "========================================="
	@echo "Changing parameters at runtime..."
	@echo "========================================="
	@echo "Before:"
	@cat /sys/module/params/parameters/count
	@echo ""
	@echo "Changing count to 5..."
	@echo 5 | sudo tee /sys/module/params/parameters/count
	@echo ""
	@echo "After:"
	@cat /sys/module/params/parameters/count

help:
	@echo "Available targets:"
	@echo "  make              - Build module"
	@echo "  make clean        - Clean build files"
	@echo "  make load         - Load with default params"
	@echo "  make load-custom  - Load with custom params"
	@echo "  make unload       - Unload module"
	@echo "  make info         - Show module info"
	@echo "  make sysfs        - Check /sys parameters"
	@echo "  make change-param - Change parameter at runtime"

.PHONY: all clean load load-custom unload info sysfs change-param help
