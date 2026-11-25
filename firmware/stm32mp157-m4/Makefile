# SPDX-License-Identifier: GPL-2.0-only
ROOT := ../..
OPENWRT_GCC := $(firstword $(wildcard $(ROOT)/staging_dir/toolchain-arm_*/bin/arm-openwrt-linux-gcc))
CROSS_COMPILE ?= $(if $(OPENWRT_GCC),$(patsubst %gcc,%,$(OPENWRT_GCC)),arm-none-eabi-)
OUT := $(ROOT)/bin/firmware/stm32mp157-m4-smoke.elf

.PHONY: all
all: $(OUT)

$(OUT): startup.S retram.ld Makefile
	mkdir -p $(@D)
	$(CROSS_COMPILE)gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=soft \
		-nostdlib -static -no-pie -Wl,--build-id=none,-T,retram.ld \
		-o $@ startup.S
