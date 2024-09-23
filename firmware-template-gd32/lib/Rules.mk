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

$(info $$BOARD [${BOARD}])
$(info $$ENET_PHY [${ENET_PHY}])

SRCDIR=src src/gd32 $(EXTRA_SRCDIR)

DEFINES:=$(addprefix -D,$(DEFINES))
DEFINES+=-D_TIME_STAMP_YEAR_=$(shell date  +"%Y") -D_TIME_STAMP_MONTH_=$(shell date  +"%-m") -D_TIME_STAMP_DAY_=$(shell date  +"%-d")

MCU=GD32F407RE

include ../firmware-template-gd32/Board.mk
include ../firmware-template-gd32/Mcu.mk
include ../firmware-template-gd32/Includes.mk
include ../firmware-template-gd32/Artnet.mk
include ../firmware-template-gd32/Validate.mk

INCLUDES+=-I../lib-configstore/include -I../lib-device/include -I../lib-display/include -I../lib-flash/include -I../lib-flashcode/include -I../lib-hal/include -I../lib-lightset/include -I../lib-network/include

COPS=-DGD32 -D$(FAMILY_UCA) -D$(LINE_UC) -D$(MCU) -D$(BOARD) -DPHY_TYPE=$(ENET_PHY)
COPS+=$(strip $(DEFINES)) $(MAKE_FLAGS) $(INCLUDES)
COPS+=$(strip $(ARMOPS) $(CMSISOPS))
COPS+=-Os -nostartfiles -ffreestanding -nostdlib
COPS+=-fstack-usage
COPS+=-ffunction-sections -fdata-sections
COPS+=-Wall -Werror -Wpedantic -Wextra -Wunused -Wsign-conversion -Wconversion -Wduplicated-cond -Wlogical-op

CPPOPS=-std=c++20
CPPOPS+=-Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference -fno-rtti -fno-exceptions -fno-unwind-tables
CPPOPS+=-Wuseless-cast -Wold-style-cast
CPPOPS+=-fno-threadsafe-statics

BUILD=build_gd32/
BUILD_DIRS:=$(addprefix build_gd32/,$(SRCDIR))
$(info $$BUILD_DIRS [${BUILD_DIRS}])

C_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.c,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.c)))
CPP_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.cpp,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.cpp)))
ASM_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.S,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.S)))

EXTRA_C_OBJECTS=$(patsubst %.c,$(BUILD)%.o,$(EXTRA_C_SOURCE_FILES))
EXTRA_C_DIRECTORIES=$(shell dirname $(EXTRA_C_SOURCE_FILES))
EXTRA_BUILD_DIRS:=$(addsuffix $(EXTRA_C_DIRECTORIES), $(BUILD))

OBJECTS:=$(strip $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS) $(EXTRA_C_OBJECTS))

CURR_DIR:=$(notdir $(patsubst %/,%,$(CURDIR)))
LIB_NAME:=$(patsubst lib-%,%,$(CURR_DIR))
TARGET=lib_gd32/lib$(LIB_NAME).a

$(info $$DEFINES [${DEFINES}])
$(info $$MAKE_FLAGS [${MAKE_FLAGS}])
$(info $$OBJECTS [${OBJECTS}])
$(info $$TARGET [${TARGET}])

define compile-objects
$(info $1)
$(BUILD)$1/%.o: $1/%.c
	$(CC) $(COPS) -c $$< -o $$@
	
$(BUILD)$1/%.o: $1/%.cpp
	$(CPP) $(COPS) $(CPPOPS) -c $$< -o $$@
	
$(BUILD)$1/%.o: $1/%.S
	$(CC) $(COPS) -D__ASSEMBLY__ -c $$< -o $$@
endef

all : builddirs $(TARGET)

.PHONY: clean builddirs

builddirs:
	mkdir -p $(BUILD_DIRS)
	mkdir -p $(EXTRA_BUILD_DIRS)
	mkdir -p lib_gd32

clean:
	rm -rf build_gd32
	rm -rf lib_gd32
	
$(BUILD)%.o: %.c
	$(CC) $(COPS) -c $< -o $@

$(BUILD_DIRS) :	
	mkdir -p $(BUILD_DIRS)
	mkdir -p lib_gd32
	
$(TARGET): Makefile.GD32 $(OBJECTS)
	$(AR) -r $(TARGET) $(OBJECTS)
	$(PREFIX)objdump -d $(TARGET) | $(PREFIX)c++filt > lib_gd32/lib.list
	
$(foreach bdir,$(SRCDIR),$(eval $(call compile-objects,$(bdir))))