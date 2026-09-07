TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

NAME        := BootSound
BUILD       := build
SOURCES     := .
INCLUDES    := .

SYS_TITLE_ID := 4200000000000077

CFLAGS      := -O2 -Wall -I$(INCLUDES) $(ARCH)
CXXFLAGS    := $(CFLAGS) -fno-exceptions -fno-rtti
LIBS        := -lnx

all: gui sysmodule

gui:
	@echo "GUI (NRO) derleniyor..."
	@$(MAKE) $(OUTPUT).nro BUILD=build_gui CXXFLAGS="$(CXXFLAGS)"

sysmodule:
	@echo "Sysmodule derleniyor..."
	@$(MAKE) $(OUTPUT).nsp BUILD=build_sys CXXFLAGS="$(CXXFLAGS) -DBUILD_SYSMODULE" TITLEID=$(SYS_TITLE_ID)

clean:
	rm -rf build_gui build_sys BootSound.nro BootSound.nsp

include $(DEVKITPRO)/libnx/switch_rules