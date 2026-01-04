$(info "FreeRTOS.mk")

ifndef ARMOPS
	$(error ARMOPS is not set)
endif

ifeq ($(findstring USE_FREE_RTOS,$(DEFINES)), USE_FREE_RTOS)
	USEFREERTOS=1
endif
ifeq ($(findstring USE_FREE_RTOS,$(MAKE_FLAGS)), USE_FREE_RTOS)
	USEFREERTOS=1
endif

ifdef USEFREERTOS
  INCLUDES+=-I../FreeRTOS/FreeRTOS-Kernel/include
	INCLUDES+=-I../firmware-template-gd32/include/FreeRTOS/
  
  ifeq ($(findstring cortex-m3,$(ARMOPS)), cortex-m3)
  	FREE_RTOS_PORTABLE=ARM_CM3
  endif
  
  ifeq ($(findstring cortex-m4,$(ARMOPS)), cortex-m4)
  	FREE_RTOS_PORTABLE=ARM_CM4F
  endif
  
  ifeq ($(findstring cortex-m7,$(ARMOPS)), cortex-m7)
  	FREE_RTOS_PORTABLE=ARM_CM7
  endif
  
  ifndef FREE_RTOS_PORTABLE
  	$(error FREE_RTOS_PORTABLE is not set)
  endif

  INCLUDES+=-I../FreeRTOS/FreeRTOS-Kernel/portable/GCC/$(FREE_RTOS_PORTABLE)
  DEFINES+=-D$(FREE_RTOS_PORTABLE)
   
  DEFINES+=-DCONFIG_TIME_USE_TIMER
endif