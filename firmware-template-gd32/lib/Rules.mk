$(info "lib/Rules.mk")
$(info $$MAKE_FLAGS [${MAKE_FLAGS}])

PREFIX ?= arm-none-eabi-

CC	= $(PREFIX)gcc
CPP	= $(PREFIX)g++
AS	= $(CC)
LD	= $(PREFIX)ld
AR	= $(PREFIX)ar

BOARD?=BOARD_GD32F407RE
ENET_PHY?=DP83848
MCU=GD32F407RE

$(info $$BOARD [${BOARD}])
$(info $$ENET_PHY [${ENET_PHY}])

SRCDIR=src src/gd32 $(EXTRA_SRCDIR)

DEFINES:=$(addprefix -D,$(DEFINES))

include ../common/make/gd32/Board.mk
include ../common/make/gd32/Mcu.mk
include ../common/make/DmxNodeNodeType.mk
include ../common/make/DmxNodeOutputType.mk
include ../common/make/gd32/Includes.mk
include ../common/make/Artnet.mk
include ../common/make/gd32/Validate.mk

INCLUDES+=-I../lib-configstore/include -I../lib-device/include -I../lib-display/include -I../lib-flash/include -I../lib-flashcode/include -I../lib-board/include -I../lib-network/include

COPS=-DGD32 -D$(FAMILY_UCA) -D$(LINE_UC) -D$(MCU) -D$(BOARD) -DPHY_TYPE=$(ENET_PHY)
COPS+=$(strip $(DEFINES) $(MAKE_FLAGS) $(VALIDATE_FLAGS) $(INCLUDES))
COPS+=$(strip $(ARMOPS) $(CMSISOPS))
COPS+=-Os -nostartfiles -ffreestanding -nostdlib
COPS+=-fstack-usage
COPS+=-ffunction-sections -fdata-sections
COPS+=-Wall -Werror -Wpedantic -Wextra -Wunused -Wsign-conversion -Wduplicated-cond -Wlogical-op
ifndef FREE_RTOS_PORTABLE
COPS+=-Wconversion
endif

include ../common/make/CppOps.mk
include ../common/make/gd32/Gd32FirmwareOps.mk

BUILD=build_gd32/
BUILD_DIRS:=$(addprefix build_gd32/,$(SRCDIR))

include ../common/make/lib/Objects.mk

CURR_DIR:=$(notdir $(patsubst %/,%,$(CURDIR)))
LIB_NAME:=$(patsubst lib-%,%,$(CURR_DIR))
TARGET=lib_gd32/lib$(LIB_NAME).a

$(info $$BUILD_DIRS [${BUILD_DIRS}])
$(info $$EXTRA_C_BUILD_DIRS [${EXTRA_C_BUILD_DIRS}])
$(info $$EXTRA_CPP_BUILD_DIRS [${EXTRA_CPP_BUILD_DIRS}])
$(info $$DEFINES [${DEFINES}])
$(info $$MAKE_FLAGS [${MAKE_FLAGS}])
$(info $$OBJECTS [${OBJECTS}])
$(info $$TARGET [${TARGET}])

define compile-objects
$(info $1)
$(BUILD)$1/%.o: $1/%.c
	$(CC) -MD -MP $(COPS) $(GD32FIRMWAREOPS) -c $$< -o $$@

$(BUILD)$1/%.o: $1/%.cpp
	$(CPP) -MD -MP $(COPS) $(CPPOPS) -c $$< -o $$@

-include $(BUILD)$1/*.d

$(BUILD)$1/%.o: $1/%.S
	$(CC) $(COPS) -D__ASSEMBLY__ -c $$< -o $$@
endef

all : builddirs $(TARGET)

.PHONY: clean builddirs

BUILD_DIRS_ALL := $(BUILD_DIRS) $(EXTRA_C_BUILD_DIRS) $(EXTRA_CPP_BUILD_DIRS) lib_gd32

builddirs:
	mkdir -p $(BUILD_DIRS_ALL)

clean:
	rm -rf build_gd32
	rm -rf lib_gd32

$(TARGET): Makefile.GD32 $(OBJECTS)
	$(AR) -r $(TARGET) $(OBJECTS)
	$(PREFIX)objdump -d $(TARGET) | $(PREFIX)c++filt > lib_gd32/lib.list
	
$(foreach bdir,$(SRCDIR),$(eval $(call compile-objects,$(bdir))))