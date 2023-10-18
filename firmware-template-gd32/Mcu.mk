ifndef MCU
	$(error MCU is not set)
endif

MCU_UC=$(shell echo $(MCU) | rev | cut -c3- | rev )
MCU_LC=$(shell echo $(MCU_UC) | tr A-Z a-z )

$(info $$MCU [${MCU}])
$(info $$MCU_LC [${MCU_LC}])
$(info $$MCU_UC [${MCU_UC}])

ifeq ($(strip $(MCU)),GD32F103RC)
	LINKER=$(FIRMWARE_DIR)gd32f103rc_flash.ld
	FAMILY=gd32f10x
	LINE=gd32f10x_hd
endif

ifeq ($(strip $(MCU)),GD32F107RC)
	LINKER=$(FIRMWARE_DIR)gd32f107rc_flash.ld
	FAMILY=gd32f10x
	LINE=gd32f10x_cl
endif

ifeq ($(strip $(MCU)),GD32F207VC)
	LINKER=$(FIRMWARE_DIR)gd32f207vc_flash.ld
	FAMILY=gd32f20x
	LINE=gd32f20x_cl
endif
	
ifeq ($(strip $(MCU)),GD32F207RG)
	LINKER=$(FIRMWARE_DIR)gd32f207rg_flash.ld
	FAMILY=gd32f20x
	LINE=gd32f20x_cl
endif

ifeq ($(strip $(MCU)),GD32F303RC)
	LINKER=$(FIRMWARE_DIR)gd32f303rc_flash.ld
	FAMILY=gd32f30x
	LINE=gd32f30x_hd
endif

ifeq ($(strip $(MCU)),GD32F407RE) 
  LINKER=$(FIRMWARE_DIR)gd32f407re_flash.ld
  FAMILY=gd32f4xx
  LINE=gd32f407
endif

ifeq ($(strip $(MCU)),GD32F450VE) 
  LINKER=$(FIRMWARE_DIR)gd32f450ve_flash.ld
  FAMILY=gd32f4xx
  LINE=gd32f450
endif

ifeq ($(strip $(MCU)),GD32F450VI) 
  LINKER=$(FIRMWARE_DIR)gd32f450vi_flash.ld
  FAMILY=gd32f4xx
  LINE=gd32f450
endif

ifndef LINKER
	$(error MCU is not configured)
endif

FAMILY_UC=$(shell echo $(FAMILY) | tr a-w A-W)

$(info $$FAMILY [${FAMILY}])
$(info $$FAMILY_UC [${FAMILY_UC}])

LINE_UC=$(shell echo $(LINE) | tr a-z A-Z)

$(info $$LINE [${LINE}])
$(info $$LINE_UC [${LINE_UC}])