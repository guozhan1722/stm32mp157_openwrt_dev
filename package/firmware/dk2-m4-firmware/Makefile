# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk

PKG_NAME:=dk2-m4-firmware
PKG_VERSION:=1
PKG_RELEASE:=1

STM32CUBEIDE ?= stm32cubeide
M4_SOURCE := $(TOPDIR)/firmware/cube-m4
M4_IMPORT ?= .
M4_BUILD ?=
M4_ELF ?=

PKG_FILE_DEPENDS:=$(M4_SOURCE)
PKG_PREPARED_DEPENDS:=STM32CUBEIDE M4_IMPORT M4_BUILD M4_ELF

include $(INCLUDE_DIR)/package.mk

RSTRIP:=:
STRIP:=:

define Package/dk2-m4-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=STM32CubeIDE Cortex-M4 firmware for STM32MP157F-DK2
  DEPENDS:=@TARGET_stm32_stm32mp1_DEVICE_stm32mp157f-dk2
endef

define Build/Prepare
	test -d "$(M4_SOURCE)" || { echo "Place your CubeIDE source tree in firmware/cube-m4" >&2; exit 1; }
	mkdir -p "$(PKG_BUILD_DIR)/source"
	$(CP) "$(M4_SOURCE)/." "$(PKG_BUILD_DIR)/source/"
endef

define Build/Configure
endef

define Build/Compile
	$(SHELL) ./build-cube-m4.sh "$(STM32CUBEIDE)" "$(PKG_BUILD_DIR)" \
		"$(M4_IMPORT)" "$(M4_BUILD)" "$(M4_ELF)"
endef

define Package/dk2-m4-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/dk2-m4.elf $(1)/lib/firmware/dk2-m4.elf
endef

$(eval $(call BuildPackage,dk2-m4-firmware))
