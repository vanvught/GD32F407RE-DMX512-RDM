$(info "Validate.mk")
$(info $$MAKE_FLAGS [${MAKE_FLAGS}])
$(info $$DEFINES [${DEFINES}])

FLAGS:=$(MAKE_FLAGS)
ifeq ($(FLAGS),)
	FLAGS:=$(DEFINES)
endif

ifneq (,$(findstring OUTPUT_DMX_SEND,$(FLAGS))$(findstring CONFIG_RDM,$(FLAGS))$(findstring RDM_CONTROLLER,$(FLAGS))$(findstring LTC,$(FLAGS)))
	TIMER6_HAVE_IRQ_HANDLER=1
	ifneq (,$(findstring CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER,$(MAKE_FLAGS)))
		$(error CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER is set)
	endif
endif

ifndef TIMER6_HAVE_IRQ_HANDLER
	DEFINES+=-DCONFIG_TIMER6_HAVE_NO_IRQ_HANDLER
endif

ifneq ($(findstring USE_FREE_RTOS,$(FLAGS)),USE_FREE_RTOS)
	DEFINES+=-DCONFIG_HAL_USE_SYSTICK
else
	DEFINES+=-DCONFIG_HAL_USE_SYSTICK # Temporarily need to fix TIMER10
endif

ifeq ($(findstring ENABLE_TFTP_SERVER,$(FLAGS)),ENABLE_TFTP_SERVER)
  ifneq ($(findstring CONFIG_HAL_USE_SYSTICK,$(FLAGS)),CONFIG_HAL_USE_SYSTICK)
  	DEFINES+=-DCONFIG_HAL_USE_SYSTICK
  endif
endif

$(info $$DEFINES [${DEFINES}])
