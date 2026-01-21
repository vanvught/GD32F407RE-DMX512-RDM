$(info "Rules.mk")

PREFIX ?= arm-none-eabi-

CC	 = $(PREFIX)gcc
CPP	 = $(PREFIX)g++
AS	 = $(CC)
LD	 = $(PREFIX)ld
AR	 = $(PREFIX)ar

BOARD?=BOARD_GD32F407RE
ENET_PHY?=DP83848
MCU?=GD32F407RE

TARGET=$(FAMILY).bin
LIST=$(FAMILY).list
MAP=$(FAMILY).map
SIZE=$(FAMILY).size
BUILD=build_gd32/
FIRMWARE_DIR=./../firmware-template-gd32/

PROJECT=$(notdir $(patsubst %/,%,$(CURDIR)))
$(info $$PROJECT [${PROJECT}])

DEFINES:=$(addprefix -D,$(DEFINES))

include ../common/make/gd32/Board.mk
include ../common/make/gd32/Mcu.mk
include ../firmware-template/libs.mk
include ../common/make/DmxNodeNodeType.mk
include ../common/make/DmxNodeOutputType.mk
include ../common/make/gd32/Includes.mk
include ../common/make/Artnet.mk
include ../common/make/gd32/mbedtls.mk
include ../common/make/gd32/Validate.mk

LIBS+=gd32 clib

# The variable for the libraries include directory
LIBINCDIRS:=$(addprefix -I../lib-,$(LIBS))
LIBINCDIRS+=$(addsuffix /include, $(LIBINCDIRS))

# The variables for the ld -L flag
LIBGD32=$(addprefix -L../lib-,$(LIBS))
LIBGD32:=$(addsuffix /lib_gd32, $(LIBGD32))

# The variable for the ld -l flag
LDLIBS:=$(addprefix -l,$(LIBS))

# The variables for the dependency check
LIBDEP=$(addprefix ../lib-,$(LIBS))

DEFINES+=-DHTTPD_CONTENT_SIZE=2048
DEFINES+=-DTCP_MAX_TCBS_ALLOWED=10
DEFINES+=-DCONFIG_NETWORK_MEMORY_BLOCKS=12

COPS=-DGD32 -D$(FAMILY_UCA) -D$(LINE_UC) -D$(MCU) -D$(BOARD) -DPHY_TYPE=$(ENET_PHY)
COPS+=$(strip $(DEFINES) $(MAKE_FLAGS) $(INCLUDES) $(LIBINCDIRS))
COPS+=$(strip $(ARMOPS) $(CMSISOPS))
COPS+=-Os -nostartfiles -ffreestanding -nostdlib
COPS+=-fstack-usage
COPS+=-ffunction-sections -fdata-sections
COPS+=-Wall -Werror -Wpedantic -Wextra -Wunused -Wsign-conversion -Wconversion -Wduplicated-cond -Wlogical-op
COPS+=--specs=nosys.specs

include ../common/make/CppOps.mk

LDOPS=--gc-sections --print-gc-sections --print-memory-usage

PLATFORM_LIBGCC+= -L $(shell dirname `$(CC) $(COPS) -print-libgcc-file-name`)

$(info $$PLATFORM_LIBGCC [${PLATFORM_LIBGCC}])

C_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.c,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.c)))
CPP_OBJECTS+=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.cpp,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.cpp)))
ASM_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.S,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.S)))

BUILD_DIRS:=$(addprefix $(BUILD),$(SRCDIR))

OBJECTS:=$(strip $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS))

define compile-objects
$(BUILD)$1/%.o: $1/%.cpp
	$(CPP) $(COPS) $(CPPOPS) -c $$< -o $$@

$(BUILD)$1/%.o: $1/%.c
	$(CC) $(COPS) -c $$< -o $$@

$(BUILD)$1/%.o: $1/%.S
	$(CC) $(COPS) -D__ASSEMBLY__ -c $$< -o $$@
endef

all : builddirs prerequisites $(TARGET)

.PHONY: clean builddirs

builddirs:
	mkdir -p $(BUILD_DIRS)

.PHONY:  clean

clean: $(LIBDEP)
	rm -rf $(BUILD)
	rm -f $(TARGET)
	rm -f $(MAP)
	rm -f $(LIST)

#
# Libraries
#

.PHONY: libdep $(LIBDEP)

lisdep: $(LIBDEP)

$(LIBDEP):
	$(MAKE) -f Makefile.GD32 $(MAKECMDGOALS) 'PROJECT=${PROJECT}' 'FAMILY=${FAMILY}' 'MCU=${MCU}' 'BOARD=${BOARD}' 'ENET_PHY=${ENET_PHY}' 'MAKE_FLAGS=$(DEFINES)' -C $@

#
# Build bin
#

$(BUILD)startup_$(LINE).o : $(FIRMWARE_DIR)/startup_$(LINE).S
	$(AS) $(COPS) -D__ASSEMBLY__ -c $(FIRMWARE_DIR)/startup_$(LINE).S -o $(BUILD)startup_$(LINE).o

$(BUILD)hardfault_handler.o : $(FIRMWARE_DIR)/hardfault_handler.cpp	
	$(CPP) $(COPS) $(CPPOPS) -c $(FIRMWARE_DIR)/hardfault_handler.cpp -o $(BUILD)hardfault_handler.o

$(BUILD)main.elf: Makefile.GD32 $(LINKER) $(BUILD)startup_$(LINE).o $(BUILD)hardfault_handler.o $(OBJECTS) $(LIBDEP)
	$(LD) $(BUILD)startup_$(LINE).o $(BUILD)hardfault_handler.o $(OBJECTS) -Map $(MAP) -T $(LINKER) $(LDOPS) -o $(BUILD)main.elf $(LIBGD32) $(LDLIBS) $(PLATFORM_LIBGCC) -lgcc
	$(PREFIX)objdump -D $(BUILD)main.elf | $(PREFIX)c++filt > $(LIST)
	$(PREFIX)size -A -x $(BUILD)main.elf

$(TARGET) : $(BUILD)main.elf
	$(PREFIX)objcopy $(BUILD)main.elf -O binary $(TARGET) --remove-section=.tcmsram* --remove-section=.sram1* --remove-section=.sram2* --remove-section=.ramadd* --remove-section=.bkpsram*

$(foreach bdir,$(SRCDIR),$(eval $(call compile-objects,$(bdir))))
