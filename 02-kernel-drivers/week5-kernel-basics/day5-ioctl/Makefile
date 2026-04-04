
obj-m := chardev_ioctl.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# 사용자 프로그램
USER_PROG := test_ioctl

all: module userspace

module:
	@echo "========================================="
	@echo "Building kernel module..."
	@echo "========================================="
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo "Build complete!"

userspace:
	@echo "========================================="
	@echo "Building user program..."
	@echo "========================================="
	gcc -o $(USER_PROG) test_ioctl.c
	@echo "Build complete!"

clean:
	@echo "Cleaning..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(USER_PROG)

load:
	@echo "========================================="
	@echo "Loading module..."
	@echo "========================================="
	sudo insmod chardev_ioctl.ko
	@dmesg | tail -10

create-dev:
	@echo "========================================="
	@echo "Creating device file..."
	@echo "========================================="
	@dmesg | grep "ioctl_dev: Registered" | tail -1
	@echo ""
	@read -p "Enter major number: " MAJOR; \
	sudo mknod /dev/ioctl_dev c $$MAJOR 0; \
	sudo chmod 666 /dev/ioctl_dev
	@ls -la /dev/ioctl_dev

test:
	@echo "========================================="
	@echo "Running test program..."
	@echo "========================================="
	./$(USER_PROG)
	@echo ""
	@echo "Kernel log:"
	@dmesg | tail -20

unload:
	@echo "Unloading module..."
	sudo rmmod chardev_ioctl
	sudo rm -f /dev/ioctl_dev

help:
	@echo "Targets:"
	@echo "  make            - Build module and userspace"
	@echo "  make load       - Load module"
	@echo "  make create-dev - Create device file"
	@echo "  make test       - Run test program"
	@echo "  make unload     - Unload module"
	@echo "  make clean      - Clean"

.PHONY: all module userspace clean load create-dev test unload help    

