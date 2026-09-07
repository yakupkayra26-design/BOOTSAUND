override TARGET := SwitchTurkceOverlay
BUILD := build
SOURCES := src/main.cpp
INCLUDES := -I$(DEVKITPRO)/libnx/include
ARCH := -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE
CXXFLAGS := -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections $(ARCH) $(INCLUDES)
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -L$(DEVKITPRO)/libnx/lib -g -Wl,--gc-sections

ifndef DEVKITPRO
$(error DEVKITPRO is not set. Install devkitPro/libnx before building)
endif

include $(DEVKITPRO)/libnx/switch_rules

.PHONY: all clean

all: $(TARGET).nro

$(BUILD)/main.o: $(SOURCES)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TARGET).elf: $(BUILD)/main.o
	$(CXX) $^ $(LDFLAGS) -lnx -lstdc++ -o $@

$(TARGET).nro: $(TARGET).elf $(TARGET).nacp
	elf2nro $< $@ --nacp=$(TARGET).nacp

$(TARGET).nacp:
	@nacptool --create "Dil Koprusu" "CROX" "0.1.0" $@

-include $(BUILD)/*.d

clean:
	rm -rf $(BUILD) *.elf *.nro *.nacp
