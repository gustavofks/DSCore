.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
include $(DEVKITARM)/ds_rules

UNIVERSAL := ../third_party/twilight/universal
TARGET    := dscore
BUILD     := build
SOURCES   := source source/core source/gfx source/launch source/platform source/ui \
             $(UNIVERSAL)/source/common $(UNIVERSAL)/source/nds_loader $(UNIVERSAL)/source/lodepng \
             $(UNIVERSAL)/source/tonccpy $(UNIVERSAL)/arm9/source $(UNIVERSAL)/sdmmc/arm9/source
INCLUDES  := source $(UNIVERSAL)/include $(UNIVERSAL)/arm9/include $(UNIVERSAL)/sdmmc/arm9/include
DATA      := ../data

# _NO_MAIN_ALL drops TWiLight's shared main() (universal/arm9/source/mainAll.cpp); ours replaces it.
ARCH     := -mthumb -mthumb-interwork
CFLAGS   := -g -Wall -O2 -march=armv5te -mtune=arm946e-s -fomit-frame-pointer -ffast-math -flto $(ARCH)
CFLAGS   += $(INCLUDE) -DARM9 -DNO_PICO_LAUNCHER -DSTANDALONE -D_NO_MAIN_ALL -DCURRENT_SCREEN_MODE=0 \
            -DDSCORE_VERSION=\"$(DSCORE_VERSION)\"
CXXFLAGS := $(CFLAGS) -fno-exceptions -std=gnu++17
ASFLAGS  := -g $(ARCH) $(INCLUDE) -flto
# Same as TWiLight's universal/arm9/ds_arm9_twlm.specs (EWRAM from 0x02004000), with paths for this tree.
LDFLAGS   = -specs=../dscore.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS     := -lfat -lnds9
LIBDIRS  := $(LIBNDS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   := $(CURDIR)/$(TARGET)
export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR  := $(CURDIR)/$(BUILD)
CFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES        := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
export BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
export LD       := $(CXX)
export OFILES   := $(addsuffix .o,$(BINFILES)) $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE  := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) $(TARGET).elf

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).elf: $(OFILES)

# The sources that show kVersion (core/Version.h) are rebuilt when VERSION changes. (After the .elf
# rule, which must stay the default goal.)
main.o Views.o: ../../VERSION

# load_bin.h / bootstub_bin.h must exist before any source that includes them is compiled.
$(filter-out $(addsuffix .o,$(BINFILES)),$(OFILES)): $(addsuffix .o,$(BINFILES))

%.bin.o: %.bin
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)

endif
