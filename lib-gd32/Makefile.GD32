$(info "lib-gd32/Makefile.GD32")
$(info $$FAMILY [${FAMILY}])
$(info $$MAKE_FLAGS [${MAKE_FLAGS}])

ifneq ($(MAKE_FLAGS),)
  ifeq ($(findstring gd32f10x,$(FAMILY)), gd32f10x)
  	EXTRA_SRCDIR=gd32f10x/CMSIS/GD/GD32F10x/Source
  	EXTRA_SRCDIR+=gd32f10x/GD32F10x_standard_peripheral/Source
  	EXTRA_SRCDIR+=src/f
  endif

  ifeq ($(findstring gd32f20x,$(FAMILY)), gd32f20x)
  	EXTRA_SRCDIR=gd32f20x/CMSIS/GD/GD32F20x/Source
  	EXTRA_SRCDIR+=gd32f20x/GD32F20x_standard_peripheral/Source
  	ifeq ($(findstring ENABLE_USB_HOST,$(MAKE_FLAGS)), ENABLE_USB_HOST)		
  		EXTRA_SRCDIR+=gd32f20x/GD32F20x_usbfs_library/host/core/Source
  		
  		EXTRA_C_SOURCE_FILES=gd32f20x/GD32F20x_usbfs_library/driver/Source/drv_usb_core.c
  		EXTRA_C_SOURCE_FILES+=gd32f20x/GD32F20x_usbfs_library/driver/Source/drv_usb_host.c
  		EXTRA_C_SOURCE_FILES+=gd32f20x/GD32F20x_usbfs_library/driver/Source/drv_usbh_int.c
  		
  		EXTRA_INCLUDES+=gd32f20x/GD32F20x_usbfs_library/driver/Include
  		EXTRA_INCLUDES+=gd32f20x/GD32F20x_usbfs_library/host/core/Include
  		EXTRA_INCLUDES+=gd32f20x/GD32F20x_usbfs_library/ustd/common
  		
  		ifeq ($(findstring CONFIG_USB_HOST_MSC,$(MAKE_FLAGS)), CONFIG_USB_HOST_MSC)
  		EXTRA_SRCDIR+=device/usb/f
  			EXTRA_SRCDIR+=gd32f20x/GD32F20x_usbfs_library/host/class/msc/Source
  		
  			EXTRA_INCLUDES+=gd32f20x/GD32F20x_usbfs_library/host/class/msc/Include
  			EXTRA_INCLUDES+=gd32f20x/GD32F20x_usbfs_library/ustd/class/msc
  		
  			EXTRA_INCLUDES+=../lib-hal/ff14b/source
  		endif
  	endif
  	EXTRA_SRCDIR+=src/f
  endif

  ifeq ($(findstring gd32f30x,$(FAMILY)), gd32f30x)
  	EXTRA_SRCDIR=gd32f30x/CMSIS/GD/GD32F30x/Source
  	EXTRA_SRCDIR+=gd32f30x/GD32F30x_standard_peripheral/Source
  	EXTRA_SRCDIR+=src/f
  endif
  
  ifeq ($(findstring gd32f4xx,$(FAMILY)), gd32f4xx)
  	EXTRA_SRCDIR=gd32f4xx/CMSIS/GD/GD32F4xx/Source
  	EXTRA_SRCDIR+=gd32f4xx/GD32F4xx_standard_peripheral/Source
    ifeq ($(findstring ENABLE_USB_HOST,$(MAKE_FLAGS)), ENABLE_USB_HOST)
    	 		EXTRA_SRCDIR+=device/usb/f
    	EXTRA_SRCDIR+=gd32f4xx/GD32F4xx_usb_library/host/core/Source
    	
    	EXTRA_C_SOURCE_FILES=gd32f4xx/GD32F4xx_usb_library/driver/Source/drv_usb_core.c
    	EXTRA_C_SOURCE_FILES+=gd32f4xx/GD32F4xx_usb_library/driver/Source/drv_usb_host.c
    	EXTRA_C_SOURCE_FILES+=gd32f4xx/GD32F4xx_usb_library/driver/Source/drv_usbh_int.c
    	   	
    	EXTRA_INCLUDES+=gd32f4xx/GD32F4xx_usb_library/driver/Include
  		EXTRA_INCLUDES+=gd32f4xx/GD32F4xx_usb_library/host/core/Include
  		EXTRA_INCLUDES+=gd32f4xx/GD32F4xx_usb_library/ustd/common
  		
  		ifeq ($(findstring CONFIG_USB_HOST_MSC,$(MAKE_FLAGS)), CONFIG_USB_HOST_MSC)
  			EXTRA_SRCDIR+=gd32f4xx/GD32F4xx_usb_library/host/class/msc/Source
  		
  			EXTRA_INCLUDES+=gd32f4xx/GD32F4xx_usb_library/host/class/msc/Include
  			EXTRA_INCLUDES+=gd32f4xx/GD32F4xx_usb_library/ustd/class/msc
  			
  			EXTRA_INCLUDES+=../lib-hal/ff14b/source
  		endif
  		
  	endif
  	EXTRA_SRCDIR+=src/f
  endif
  
 	ifeq ($(findstring gd32h7xx,$(FAMILY)), gd32h7xx)
  		EXTRA_SRCDIR=gd32h7xx/CMSIS/GD/GD32H7xx/Source
  		EXTRA_SRCDIR+=gd32h7xx/GD32H7xx_standard_peripheral/Source
  	 	ifeq ($(findstring ENABLE_USB_HOST,$(MAKE_FLAGS)), ENABLE_USB_HOST)
  	 		EXTRA_SRCDIR+=device/usb/h
  			EXTRA_SRCDIR+=gd32h7xx/GD32H7xx_usbhs_library/host/core/Source
  		 	EXTRA_C_SOURCE_FILES=gd32h7xx/GD32H7xx_usbhs_library/driver/Source/drv_usb_core.c
    		EXTRA_C_SOURCE_FILES+=gd32h7xx/GD32H7xx_usbhs_library/driver/Source/drv_usb_host.c
    		EXTRA_C_SOURCE_FILES+=gd32h7xx/GD32H7xx_usbhs_library/driver/Source/drv_usbh_int.c 
	  	 	ifeq ($(findstring CONFIG_USB_HOST_MSC,$(MAKE_FLAGS)), CONFIG_USB_HOST_MSC)
	  	 		EXTRA_SRCDIR+=gd32h7xx/GD32H7xx_usbhs_library/host/class/msc/Source
					EXTRA_INCLUDES+=../lib-hal/ff14b/source
	  	 	endif
  		endif
		EXTRA_SRCDIR+=src/h
  	endif

  ifeq ($(findstring NO_EMAC,$(MAKE_FLAGS)), NO_EMAC)
  else
  	EXTRA_SRCDIR+=device/emac
  endif
   
  ifeq ($(findstring CONFIG_USE_SOFTUART0,$(MAKE_FLAGS)), CONFIG_USE_SOFTUART0)
  	EXTRA_SRCDIR+=src/softuart0
  else
   	ifeq ($(findstring CONSOLE_NULL,$(MAKE_FLAGS)), CONSOLE_NULL)
   	else
  		EXTRA_SRCDIR+=src/uart0
  	endif
  endif
  
  ifeq ($(findstring ENABLE_PHY_SWITCH,$(MAKE_FLAGS)), ENABLE_PHY_SWITCH)	
  	EXTRA_SRCDIR+=device/emac/dsa
  endif
  
  ifeq ($(findstring ENABLE_USB_HOST,$(MAKE_FLAGS)), ENABLE_USB_HOST)		
  	EXTRA_SRCDIR+=device/usb
  endif	
else
	ifeq ($(FAMILY),)
		ifneq (, $(shell test -d '../lib-gd32/gd32f10x' && echo -n yes))
			FAMILY=gd32f10x
			EXTRA_SRCDIR+=src/f
		endif
		ifneq (, $(shell test -d '../lib-gd32/gd32f30x' && echo -n yes))
			FAMILY=gd32f30x
			EXTRA_SRCDIR+=src/f
		endif
		ifneq (, $(shell test -d '../lib-gd32/gd32f4xx' && echo -n yes))
			FAMILY=gd32f4xx
			HAVE_SOFTUART=1
			EXTRA_SRCDIR+=src/f
		endif
		ifneq (, $(shell test -d '../lib-gd32/gd32h7xx' && echo -n yes))
			FAMILY=gd32h7xx
			HAVE_SOFTUART=1
			EXTRA_SRCDIR+=src/h
		endif
		ifneq (, $(shell test -d '../lib-gd32/gd32f20x' && echo -n yes))
			FAMILY=gd32f20x
			HAVE_SOFTUART=1
			EXTRA_SRCDIR+=src/f
		endif
	endif
	
	EXTRA_SRCDIR+=device/emac device/emac/dsa
	EXTRA_SRCDIR+=device/usb
	EXTRA_SRCDIR+=src/uart0 
	ifdef HAVE_SOFTUART
		EXTRA_SRCDIR+=src/softuart0
	endif
	
	DEFINES=ENABLE_USB_HOST CONFIG_USB_HOST_MSC
endif

$(info $$FAMILY [${FAMILY}])

include ../firmware-template-gd32/lib/Rules.mk