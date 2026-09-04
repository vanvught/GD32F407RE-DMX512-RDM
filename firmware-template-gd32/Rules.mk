$(info "Rules.mk")

PREFIX ?= arm-none-eabi-

CC      = $(PREFIX)gcc
CPP     = $(PREFIX)g++
AS      = $(CC)
LD      = $(PREFIX)gcc
AR      = $(PREFIX)gcc-ar
RANLIB  = $(PREFIX)gcc-ranlib
NM      = $(PREFIX)gcc-nm

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
DEFINES+=-DHTTPD_CONTENT_SIZE=2048
DEFINES+=-DTCP_MAX_TCBS_ALLOWED=10
DEFINES+=-DCONFIG_NETWORK_MEMORY_BLOCKS=12
DEFINES+=-DPHY_TYPE=$(ENET_PHY)

include ../common/make/gd32/Board.mk
include ../common/make/gd32/Mcu.mk
include ../firmware-template/libs.mk
include ../common/make/DmxNodeNodeType.mk
include ../common/make/DmxNodeOutputType.mk
include ../common/make/gd32/Includes.mk
include ../common/make/Artnet.mk
include ../common/make/gd32/mbedtls.mk
include ../common/make/gd32/Validate.mk

LIBS+=clib gd32

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

COPS=-DGD32 -D$(FAMILY_UCA) -D$(LINE_UC) -D$(MCU) -D$(BOARD) 
COPS+=$(strip $(DEFINES) $(MAKE_FLAGS) $(INCLUDES) $(LIBINCDIRS))
COPS+=$(strip $(ARMOPS) $(CMSISOPS))
COPS+=-Os -nostartfiles -ffreestanding -nostdlib
COPS+=-fstack-usage
COPS+=-ffunction-sections -fdata-sections
COPS+=-Wall -Werror -Wpedantic -Wextra -Wunused -Wsign-conversion -Wconversion -Wduplicated-cond -Wlogical-op
COPS+=--specs=nosys.specs
COPS+=-flto=auto

include ../common/make/CppOps.mk
include ../common/make/LdOps.mk

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
# Startup and support objects
#

# Assemble the MCU startup code.
$(BUILD)startup_$(LINE).o : $(FIRMWARE_DIR)/startup_$(LINE).S
	$(AS) $(COPS) -D__ASSEMBLY__ -c $(FIRMWARE_DIR)/startup_$(LINE).S -o $(BUILD)startup_$(LINE).o

# Compile the common HardFault handler.
$(BUILD)hardfault_handler.o : $(FIRMWARE_DIR)/hardfault_handler.cpp	
	$(CPP) $(COPS) $(CPPOPS) -c $(FIRMWARE_DIR)/hardfault_handler.cpp -o $(BUILD)hardfault_handler.o

# Compile the common debug Stack handler.
$(BUILD)stack_debug_init.o : $(FIRMWARE_DIR)/stack_debug_init.cpp	
	$(CPP) $(COPS) $(CPPOPS) -c $(FIRMWARE_DIR)/stack_debug_init.cpp -o $(BUILD)stack_debug_init.o

#
# Link the ELF image
#	
	
# Link all object files together with the dependent libraries.
# A linker map and a demangled disassembly listing are generated
# for debugging and analysis.
$(BUILD)main.elf: \
		Makefile.GD32 \
		$(LINKER) \
		$(BUILD)startup_$(LINE).o \
		$(BUILD)hardfault_handler.o \
		$(BUILD)stack_debug_init.o \
		$(OBJECTS) \
		$(LIBDEP) \
		| builddirs
	  $(LD) \
		$(BUILD)startup_$(LINE).o \
		$(BUILD)hardfault_handler.o \
		$(BUILD)stack_debug_init.o \
		$(OBJECTS) \
		-T $(LINKER) \
		$(LDOPS) \
		-o $@ \
		$(LIBGD32) \
		$(LDLIBS) \
		-lgcc

	# Generate a demangled disassembly listing.
	$(PREFIX)objdump -D $@ | $(PREFIX)c++filt > $(LIST)

	# Display the memory usage by section.
	$(PREFIX)size -A -x $@

#
# Create the binary firmware image
#

# Convert the ELF image into a binary image. RAM-only sections are
# removed because they are initialized at runtime rather than stored
# in flash.
$(TARGET): $(BUILD)main.elf
	$(PREFIX)objcopy $< \
		-O binary \
		$@ \
		--remove-section=.tcmsram* \
		--remove-section=.sram1* \
		--remove-section=.sram2* \
		--remove-section=.ramadd* \
		--remove-section=.bkpsram*

$(foreach bdir,$(SRCDIR),$(eval $(call compile-objects,$(bdir))))
