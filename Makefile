.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
include $(DEVKITARM)/ds_rules

export TARGET := dscore
TWILIGHT      := $(CURDIR)/third_party/twilight

.PHONY: all bootloader bootstub arm7 arm9 clean

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

clean:
	@$(MAKE) -C arm9 clean
	@$(MAKE) -C arm7 clean
	@$(MAKE) -C $(TWILIGHT)/universal/bootloader_app clean
	@$(MAKE) -C $(TWILIGHT)/universal/bootstub clean
	@rm -rf data $(TARGET).nds
