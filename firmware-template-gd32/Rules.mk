$(info "Rules.mk")

PREFIX ?= arm-none-eabi-

CC	 = $(PREFIX)gcc
CPP	 = $(PREFIX)g++
AS	 = $(CC)
LD	 = $(PREFIX)ld
AR	 = $(PREFIX)ar

BOARD?=BOARD_GD32F407RE
ENET_PHY?=DP83848

# Output 
TARGET=$(FAMILY).bin
LIST=$(FAMILY).list
MAP=$(FAMILY).map
SIZE=$(FAMILY).size
BUILD=build_gd32/

# Input
SOURCE=./
FIRMWARE_DIR=./../firmware-template-gd32/

DEFINES:=$(addprefix -D,$(DEFINES))

ifeq ($(findstring ARTNET_VERSION=4,$(DEFINES)),ARTNET_VERSION=4)
	ifeq ($(findstring ARTNET_HAVE_DMXIN,$(DEFINES)),ARTNET_HAVE_DMXIN)
		DEFINES+=-DE131_HAVE_DMXIN
	endif
endif

MCU=GD32F407RE

include ../firmware-template-gd32/Mcu.mk
include ../firmware-template/libs.mk

LIBS+=c++ c gd32

include ../firmware-template-gd32/Includes.mk

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

$(info $$BOARD [${BOARD}])
$(info $$ENET_PHY [${ENET_PHY}])
$(info $$DEFINES [${DEFINES}])
$(info $$LIBS [${LIBS}])
$(info $$LIBDEP [${LIBDEP}])

COPS=-DBARE_METAL -DGD32 -DGD32F407 -D$(BOARD) -DPHY_TYPE=$(ENET_PHY)
COPS+=$(DEFINES) $(MAKE_FLAGS) $(INCLUDES) $(LIBINCDIRS)
COPS+=-Os -mcpu=cortex-m4 -mthumb -g -mfloat-abi=hard -fsingle-precision-constant -mfpu=fpv4-sp-d16 -DARM_MATH_CM4 -D__FPU_PRESENT=1
COPS+=-nostartfiles -ffreestanding -nostdlib
COPS+=-fstack-usage
COPS+=-ffunction-sections -fdata-sections
COPS+=-Wall -Werror -Wpedantic -Wextra -Wunused -Wsign-conversion -Wconversion
COPS+=-Wduplicated-cond -Wlogical-op

CPPOPS=-std=c++11
CPPOPS+=-Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference -fno-rtti -fno-exceptions -fno-unwind-tables
CPPOPS+=-Wuseless-cast -Wold-style-cast
CPPOPS+=-fno-threadsafe-statics

LDOPS=--gc-sections --print-gc-sections

PLATFORM_LIBGCC+= -L $(shell dirname `$(CC) $(COPS) -print-libgcc-file-name`)

$(info $$PLATFORM_LIBGCC [${PLATFORM_LIBGCC}])

C_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.c,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.c)))
C_OBJECTS+=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.cpp,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.cpp)))
ASM_OBJECTS=$(foreach sdir,$(SRCDIR),$(patsubst $(sdir)/%.S,$(BUILD)$(sdir)/%.o,$(wildcard $(sdir)/*.S)))

BUILD_DIRS:=$(addprefix $(BUILD),$(SRCDIR))

OBJECTS:=$(ASM_OBJECTS) $(C_OBJECTS)

define compile-objects
$(BUILD)$1/%.o: $(SOURCE)$1/%.cpp
	$(CPP) $(COPS) $(CPPOPS) -c $$< -o $$@	

$(BUILD)$1/%.o: $(SOURCE)$1/%.c
	$(CC) $(COPS) -c $$< -o $$@
	
$(BUILD)$1/%.o: $(SOURCE)$1/%.S
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
	rm -f $(SIZE)

#
# Libraries
#

.PHONY: libdep $(LIBDEP)

lisdep: $(LIBDEP)

$(LIBDEP):
	$(MAKE) -f Makefile.GD32 $(MAKECMDGOALS) 'FAMILY=${FAMILY}' 'MCU=${MCU}' 'BOARD=${BOARD}' 'ENET_PHY=${ENET_PHY}' 'MAKE_FLAGS=$(DEFINES)' -C $@ 

#
# Build bin
#

$(BUILD_DIRS) :
	mkdir -p $(BUILD_DIRS)

$(BUILD)startup_$(MCU_LC).o : $(FIRMWARE_DIR)/startup_$(MCU_LC).S
	$(AS) $(COPS) -D__ASSEMBLY__ -c $(FIRMWARE_DIR)/startup_$(MCU_LC).S -o $(BUILD)startup_$(MCU_LC).o
	
$(BUILD)main.elf: Makefile.GD32 $(LINKER) $(BUILD)startup_$(MCU_LC).o $(OBJECTS) $(LIBDEP)
	$(LD) $(BUILD)startup_$(MCU_LC).o $(OBJECTS) -Map $(MAP) -T $(LINKER) $(LDOPS) -o $(BUILD)main.elf $(LIBGD32) $(LDLIBS) $(PLATFORM_LIBGCC) -lgcc 
	$(PREFIX)objdump -D $(BUILD)main.elf | $(PREFIX)c++filt > $(LIST)
	$(PREFIX)size -A -x $(BUILD)main.elf > $(FAMILY).size
	$(MAKE) -f Makefile.GD32 calculate_unused_ram SIZE_FILE=$(FAMILY).size LINKER_SCRIPT=$(LINKER)

$(TARGET) : $(BUILD)main.elf 
	$(PREFIX)objcopy $(BUILD)main.elf --remove-section=.tcmsram* --remove-section=.sram1* --remove-section=.sram2* --remove-section=.bkpsram* -O binary $(TARGET)	
	
$(foreach bdir,$(SRCDIR),$(eval $(call compile-objects,$(bdir))))

.PHONY: calculate_unused_ram
calculate_unused_ram: $(FAMILY).size $(LINKER)
	@$(FIRMWARE_DIR)/calculate_unused_ram.sh $(FAMILY).size $(LINKER)
