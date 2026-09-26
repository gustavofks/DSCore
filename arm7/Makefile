.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
include $(DEVKITARM)/ds_rules

UNIVERSAL := ../third_party/twilight/universal
TARGET    := dscore
BUILD     := build
SOURCES   := source $(UNIVERSAL)/arm7/source $(UNIVERSAL)/sdmmc/arm7/source
INCLUDES  := include build $(UNIVERSAL)/include $(UNIVERSAL)/arm7/include $(UNIVERSAL)/sdmmc/arm7/include

ARCH     := -march=armv4t -mthumb -mthumb-interwork
CFLAGS   := -g -Wall -O2 -mcpu=arm7tdmi -mtune=arm7tdmi -fomit-frame-pointer -ffast-math -flto $(ARCH)
CFLAGS   += $(INCLUDE) -DARM7
ASFLAGS  := -g $(ARCH) -flto
LDFLAGS   = -specs=ds_arm7.specs -g $(ARCH) -Wl,--nmagic -Wl,-Map,$(notdir $*).map
LIBS     := -lmm7 -lnds7
LIBDIRS  := $(LIBNDS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
CFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES        := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
export OFILES   := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export DEPSDIR  := $(CURDIR)/$(BUILD)
export OUTPUT   := $(CURDIR)/$(TARGET)
export LD       := $(CC)

.PHONY: all $(BUILD) clean

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) $(TARGET).elf

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).elf: $(OFILES) $(LIBNDS)/lib/libnds7.a

-include $(DEPENDS)

endif
