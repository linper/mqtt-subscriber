include $(TOPDIR)/rules.mk

PKG_NAME:=mqtt_sub
PKG_VERSION:=1

include $(INCLUDE_DIR)/package.mk

define Package/mqtt_sub
	SECTION:=net
	CATEGORY:=Network
	TITLE:=MQTT Subscriber
	DEPENDS:=+libuci +libmosquitto +libsqlite3 +libjson-c
endef

define Package/mqtt_sub/description
	MQTT Subscriber
endef

define Package/mqtt_sub/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/share/mqtt_sub
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqtt_sub $(1)/usr/sbin/mqtt_sub
	$(INSTALL_BIN) ./files/mqtt_sub.init $(1)/etc/init.d/mqtt_sub
	$(INSTALL_CONF) ./files/mqtt_sub.config $(1)/etc/config/mqtt_sub
endef

$(eval $(call BuildPackage,mqtt_sub))
