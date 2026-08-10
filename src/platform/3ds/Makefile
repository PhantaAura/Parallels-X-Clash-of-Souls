# Parallels X 3.0R — Old 3DS XL gameplay-parity target
# Requires devkitPro/devkitARM with libctru, Citro3D and Citro2D installed.

ifeq ($(strip $(DEVKITPRO)),)
$(error "DEVKITPRO is not set. Install devkitPro's 3DS toolchain first, then reopen your terminal.")
endif

include $(DEVKITARM)/3ds_rules

TARGET      := parallels_x_3ds_golden_gate
BUILD       := build
SOURCES     := . ../../core ../../content
DATA        :=
INCLUDES    := ../.. ../../
ifeq ($(strip $(TOPDIR)),)
ROMFS       := $(CURDIR)/romfs
else
ROMFS       := $(TOPDIR)/romfs
endif
APP_TITLE       := Parallels X: Clash of Souls
APP_DESCRIPTION := Chapter 1 - Legacy Remake
APP_AUTHOR      := PhantaAura

ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS      := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections $(ARCH)
CFLAGS      += $(INCLUDE) -D__3DS__
CXXFLAGS    := $(CFLAGS) -std=gnu++17 -fexceptions -frtti
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS        := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS     := $(CTRULIB) $(PORTLIBS)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))

export LD := $(CXX)
export OFILES := $(PICAFILES:.v.pica=.shbin.o) $(addsuffix .o,$(BINFILES)) $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export HFILES := $(PICAFILES:.v.pica=_shbin.h)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean
all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh

else
DEPENDS := $(OFILES:.o=.d)

_3DSXFLAGS := --smdh=$(OUTPUT).smdh --romfs=$(ROMFS)

$(OUTPUT).3dsx : $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf : $(OFILES)
$(CPPFILES:.cpp=.o) : $(HFILES)

.PRECIOUS : %.shbin
%.shbin.o %_shbin.h : %.shbin
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

-include $(DEPENDS)
endif
