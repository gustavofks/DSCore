.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
include $(DEVKITARM)/ds_rules

export TARGET := dscore
export DSCORE_VERSION := $(shell cat VERSION)
TWILIGHT      := $(CURDIR)/third_party/twilight

.PHONY: all bootloader bootstub arm7 arm9 clean dist

all: $(TARGET).nds

data:
	@mkdir -p data

bootloader: data
	@$(MAKE) -C $(TWILIGHT)/universal/bootloader_app LOADBIN=$(CURDIR)/data/load.bin

bootstub: data
	@$(MAKE) -C $(TWILIGHT)/universal/bootstub BOOTSTUB=$(CURDIR)/data/bootstub.bin

arm7:
	@$(MAKE) -C arm7

arm9: bootloader bootstub
	@$(MAKE) -C arm9

$(TARGET).nds: arm7 arm9
	ndstool -u 00030004 -g DSCR 01 "DSCORE" -c $@ -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf \
		-b icon.bmp "DSCore;Nintendo DSi frontend;DSCore"

# Release zip: the same layout as the repository, so tools/deploy.ps1 works from inside it.
DIST := dist/DSCore-$(DSCORE_VERSION)
dist: $(TARGET).nds
	@rm -rf $(DIST) $(DIST).zip
	@mkdir -p $(DIST)/tools $(DIST)/themes
	@cp $(TARGET).nds README.md LICENSE INSTALL.txt $(DIST)/
	@cp themes/*.ini $(DIST)/themes/
	@cp tools/deploy.ps1 tools/restore.ps1 tools/fetch_covers.py $(DIST)/tools/
	@cd dist && zip -qr DSCore-$(DSCORE_VERSION).zip DSCore-$(DSCORE_VERSION)
	@echo "built $(DIST).zip"

clean:
	@$(MAKE) -C arm9 clean
	@$(MAKE) -C arm7 clean
	@$(MAKE) -C $(TWILIGHT)/universal/bootloader_app clean
	@$(MAKE) -C $(TWILIGHT)/universal/bootstub clean
	@rm -rf data dist $(TARGET).nds
