include $(TOPDIR)/rules.mk

PKG_NAME:=dk2-hdmi-splash
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/dk2-hdmi-splash
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=STM32MP157F-DK2 HDMI startup picture
  DEPENDS:=@TARGET_stm32_stm32mp1
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/dk2-hdmi-splash $(PKG_BUILD_DIR)/dk2-hdmi-splash.c
endef

define Package/dk2-hdmi-splash/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dk2-hdmi-splash $(1)/usr/bin/
endef

$(eval $(call BuildPackage,dk2-hdmi-splash))
