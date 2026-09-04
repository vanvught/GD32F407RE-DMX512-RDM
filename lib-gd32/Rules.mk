$(info "lib-gd32/Rules.mk")

ifneq ($(MAKE_FLAGS),)
	ifeq ($(findstring CONFIG_HAL_USE_SYSTICK,$(MAKE_FLAGS)), CONFIG_HAL_USE_SYSTICK)
		EXTRA_SRCDIR+=src/systick
	endif 
	
	ifeq ($(findstring CONFIG_USE_SOFTUART0,$(MAKE_FLAGS)), CONFIG_USE_SOFTUART0)
 		EXTRA_SRCDIR+=src/softuart0
 	else
   		ifeq ($(findstring CONFIG_CLIB_USE_NULL,$(MAKE_FLAGS)), CONFIG_CLIB_USE_NULL)
   		else
  			EXTRA_SRCDIR+=src/uart0
  		endif
  	endif
  	
  ifeq ($(findstring gd32f4xx,$(FAMILY)), gd32f4xx)
		EXTRA_SRCDIR+=src/f/fmc4
	else
		ifeq ($(findstring gd32h7xx,$(FAMILY)), gd32h7xx)
			EXTRA_SRCDIR+=src/h/fmc
		else
			EXTRA_SRCDIR+=src/f/fmc
		endif
	endif
  	
  ifeq ($(findstring NO_EMAC,$(MAKE_FLAGS)), NO_EMAC)
	else
		EXTRA_SRCDIR+=device/enet
   		ifeq ($(findstring CONFIG_NET_ENABLE_PTP,$(MAKE_FLAGS)), CONFIG_NET_ENABLE_PTP)	
 				EXTRA_SRCDIR+=device/enet/ptp
  		endif
  endif
else
	EXTRA_SRCDIR+=src/systick
	EXTRA_SRCDIR+=src/uart0
	EXTRA_SRCDIR+=src/softuart0
endif

$(info $$EXTRA_SRCDIR [${EXTRA_SRCDIR}])