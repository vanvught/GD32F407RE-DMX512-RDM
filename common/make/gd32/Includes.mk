$(info "Includes.mk")

INCLUDES:=-I./include 
INCLUDES+=-I../common/include -I../include 
INCLUDES+=-I../firmware-template-gd32/include
INCLUDES+=-I../CMSIS/Core/Include
INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_standard_peripheral/Include
INCLUDES+=-I../lib-gd32/${FAMILY}/CMSIS/GD/${FAMILY_UC}/Include
INCLUDES+=-I../lib-gd32/include
INCLUDES+=$(addprefix -I,$(EXTRA_INCLUDES))

ALL_FLAGS := $(DEFINES) $(MAKE_FLAGS)
$(info $$ALL_FLAGS [${ALL_FLAGS}])

# $(call set_if_present,FLAG,VAR)  -> sets VAR=1 if FLAG or -DFLAG is present
set_if_present = $(eval $(2) := $(if $(filter $(1) -D$(1),$(ALL_FLAGS)),1,))

$(call set_if_present,ENABLE_USB_HOST,USB_HOST)
$(call set_if_present,CONFIG_USB_HOST_MSC,USB_HOST_MSC)
$(call set_if_present,ENABLE_USB_DEVICE,USB_DEVICE)
$(call set_if_present,CONFIG_USB_DEVICE_CDC,USB_DEVICE_CDC)
$(call set_if_present,CONFIG_USB_DEVICE_HID,USB_DEVICE_HID)

ifdef USB_HOST
	INCLUDES+=-I../lib-gd32/device/usb
	INCLUDES+=-I../lib-hal/device/usb/host/gd32
endif

ifdef USB_DEVICE
	INCLUDES+=-I../lib-gd32/device/usb
	INCLUDES+=-I../lib-hal/device/usb/device/gd32
endif

ifeq ($(findstring gd32f20x,$(FAMILY)), gd32f20x)
	ifdef USB_HOST
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/driver/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/host/core/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/ustd/common
		ifdef USB_HOST_MSC
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/host/class/msc/Include
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/ustd/class/msc
		endif
	endif
	ifdef USB_DEVICE
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/driver/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/device/core/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/ustd/common
		ifdef USB_DEVICE_CDC
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/device/class/cdc/Include
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/ustd/class/cdc
		endif
		ifdef USB_DEVICE_HID
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/device/class/hid/Include
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbfs_library/ustd/class/hid
		endif
	endif
endif

ifeq ($(findstring gd32f4xx,$(FAMILY)), gd32f4xx)
	ifdef USB_HOST	
  		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/driver/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/host/core/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/ustd/common
		ifdef USB_HOST_MSC
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/host/class/msc/Include
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/ustd/class/msc
		endif
	endif
	ifdef USB_DEVICE
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/driver/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/device/core/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/ustd/common
		ifdef USB_DEVICE_CDC
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/device/class/cdc/Include
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/ustd/class/cdc
		endif
		ifdef USB_DEVICE_HID
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/device/class/hid/Include
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usb_library/ustd/class/hid
		endif
	endif
endif

ifeq ($(findstring gd32h7xx,$(FAMILY)), gd32h7xx)
	ifdef USB_HOST	
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbhs_library/driver/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbhs_library/host/core/Include
		INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbhs_library/ustd/common
		ifdef USB_HOST_MSC
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbhs_library/host/class/msc/Include
			INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_usbhs_library/ustd/class/msc
		endif
	endif
endif

ifdef USB_HOST_MSC
		INCLUDES+=-I../lib-hal/ff14b/source
endif

INCLUDES:= $(strip -I../${PROJECT}/include $(sort $(INCLUDES)))
$(info $$INCLUDES [${INCLUDES}])