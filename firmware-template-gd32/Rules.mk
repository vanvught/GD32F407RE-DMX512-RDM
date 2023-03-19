PREFIX ?= arm-none-eabi-

CC	 = $(PREFIX)gcc
CPP	 = $(PREFIX)g++
AS	 = $(CC)
LD	 = $(PREFIX)ld
AR	 = $(PREFIX)ar

BOARD?=BOARD_GD32F407RE
ENET_PHY?=DP83848
FAMILY?=gd32f4xx
MCU?=gd32f407

MCU_UC:=$(shell echo $(MCU_UC) | tr a-w A-W)
FAMILY:=$(shell echo $(FAMILY) | tr A-Z a-z)
FAMILY_UC=$(shell echo $(FAMILY) | tr a-w A-W)

$(info $$FAMILY_UC [${MCU_UC}])
$(info $$FAMILY [${FAMILY}])
$(info $$FAMILY_UC [${FAMILY_UC}])

# Output 
TARGET=$(FAMILY).bin
LIST=$(FAMILY).list
MAP=$(FAMILY).map
BUILD=build_gd32/

# Input
SOURCE = ./
FIRMWARE_DIR = ./../firmware-template-gd32/
LINKER = $(FIRMWARE_DIR)/gd32f407re_flash.ld

include ../firmware-template/libs.mk

LIBS+=c++ c gd32

$(info [${LIBS}])
	
DEFINES:=$(addprefix -D,$(DEFINES))

ifeq ($(findstring DMX4,$(BOARD)), DMX4)
	DEFINES+=-DCONSOLE_I2C
endif

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

$(info $$LIBDEP [${LIBDEP}])

COPS=-DBARE_METAL -DGD32 -DGD32F407 -D$(BOARD) -DPHY_TYPE=$(ENET_PHY)
COPS+=$(DEFINES) $(MAKE_FLAGS) $(INCLUDES) $(LIBINCDIRS)
COPS+=-Os -mcpu=cortex-m4 -mthumb -g -mfloat-abi=hard -fsingle-precision-constant -mfpu=fpv4-sp-d16
COPS+=-DARM_MATH_CM4 -D__FPU_PRESENT=1
COPS+=-nostartfiles -ffreestanding -nostdlib
COPS+=-fstack-usage -Wstack-usage=10240
COPS+=-ffunction-sections -fdata-sections

CPPOPS=-std=c++11 
CPPOPS+=-Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference -fno-rtti -fno-exceptions -fno-unwind-tables
#CPPOPS+=-Wuseless-cast -Wold-style-cast
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

#
# Libraries
#

.PHONY: libdep $(LIBDEP)

lisdep: $(LIBDEP)

$(LIBDEP):
	$(MAKE) -f Makefile.GD32 $(MAKECMDGOALS) 'FAMILY=${FAMILY}' 'BOARD=${BOARD}' 'PHY_TYPE=${ENET_PHY}' 'MAKE_FLAGS=$(DEFINES)' -C $@ 

# Build bin

$(BUILD_DIRS) :
	mkdir -p $(BUILD_DIRS)

$(BUILD)startup_$(MCU).o : $(FIRMWARE_DIR)/startup_$(MCU).S
	$(AS) $(COPS) -D__ASSEMBLY__ -c $(FIRMWARE_DIR)/startup_$(MCU).S -o $(BUILD)startup_$(MCU).o
	
$(BUILD)main.elf: Makefile.GD32 $(LINKER) $(BUILD)startup_$(MCU).o $(OBJECTS) $(LIBDEP)
	$(LD) $(BUILD)startup_$(MCU).o $(OBJECTS) -Map $(MAP) -T $(LINKER) $(LDOPS) -o $(BUILD)main.elf $(LIBGD32) $(LDLIBS) $(PLATFORM_LIBGCC) -lgcc 
	$(PREFIX)objdump -D $(BUILD)main.elf | $(PREFIX)c++filt > $(LIST)
	$(PREFIX)size -A -x $(BUILD)main.elf

$(TARGET) : $(BUILD)main.elf 
	$(PREFIX)objcopy $(BUILD)main.elf --remove-section=.tcmsram* --remove-section=.bkpsram* -O binary $(TARGET)	
	
$(foreach bdir,$(SRCDIR),$(eval $(call compile-objects,$(bdir))))