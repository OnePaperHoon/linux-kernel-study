# Week 6 Day 1: UART Communication
# Makefile

obj-m := uart_stm32.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	@echo "========================================="
	@echo "Building UART driver..."
	@echo "========================================="
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	@echo "Loading module..."
	sudo insmod uart_stm32.ko
	@dmesg | tail -10

create-dev:
	@echo "Creating device file..."
	@dmesg | grep "uart_stm32: Registered" | tail -1
	@read -p "Enter major number: " MAJOR; \
	sudo mknod /dev/stm32_uart c $$MAJOR 0; \
	sudo chmod 666 /dev/stm32_uart
	@ls -la /dev/stm32_uart

test-write:
	@echo "Sending to STM32..."
	@echo "Hello STM32!" > /dev/stm32_uart
	@sleep 1
	@dmesg | tail -5

test-read:
	@echo "Reading from STM32..."
	@cat /dev/stm32_uart

unload:
	@echo "Unloading module..."
	sudo rmmod uart_stm32
	sudo rm -f /dev/stm32_uart

.PHONY: all clean load create-dev test-write test-read unload
