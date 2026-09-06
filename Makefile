TARGET_GUI := V1
TARGET_SYS := BootSoundSysmodule

BUILD_GUI := build/gui
BUILD_SYS := build/sysmodule

SOURCES := main.cpp
INCLUDES := -I. -I.github -I$(DEVKITPRO)/libnx/include
ARCH := -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE

COMMON_CXXFLAGS := -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections $(ARCH) $(INCLUDES)
COMMON_LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -L$(DEVKITPRO)/libnx/lib -g -Wl,--gc-sections

ifndef DEVKITPRO
$(error DEVKITPRO is not set. Install devkitPro/libnx before building)
endif

include $(DEVKITPRO)/libnx/switch_rules

.PHONY: all clean

all: $(TARGET_GUI).nro $(TARGET_SYS).nsp

$(BUILD_GUI)/main.o: $(SOURCES)
	@mkdir -p $(BUILD_GUI)
	$(CXX) $(COMMON_CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_SYS)/main.o: $(SOURCES)
	@mkdir -p $(BUILD_SYS)
	$(CXX) $(COMMON_CXXFLAGS) -DBUILD_SYSMODULE -MMD -MP -c $< -o $@

$(TARGET_GUI).elf: $(BUILD_GUI)/main.o
	$(CXX) $^ $(COMMON_LDFLAGS) -lnx -o $@

$(TARGET_SYS).elf: $(BUILD_SYS)/main.o
	$(CXX) $^ $(COMMON_LDFLAGS) -lnx -o $@

$(TARGET_GUI).nro: $(TARGET_GUI).elf $(TARGET_GUI).nacp
	elf2nro $< $@ --nacp=$(TARGET_GUI).nacp

$(TARGET_SYS).nsp: $(TARGET_SYS).elf
	elf2nsp $< $@

$(TARGET_GUI).nacp:
	@printf 'BootSound GUI\n' > $@

$(TARGET_SYS).nacp:
	@printf 'BootSound service\n' > $@

-include $(BUILD_GUI)/*.d $(BUILD_SYS)/*.d

clean:
	rm -rf build *.elf *.nro *.nsp *.nacp