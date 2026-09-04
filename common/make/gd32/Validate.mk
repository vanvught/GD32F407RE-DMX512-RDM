$(info "Validate.mk")
$(info $$MAKE_FLAGS [${MAKE_FLAGS}])
$(info $$DEFINES [${DEFINES}])

FLAGS:=$(MAKE_FLAGS)
ifeq ($(FLAGS),)
	FLAGS:=$(DEFINES)
endif

ifneq ($(findstring _TIME_STAMP_YEAR_,$(FLAGS)),_TIME_STAMP_YEAR_)
	include ../common/make/Timestamp.mk
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

ifeq ($(findstring CONFIG_REMOTECONFIG_MINIMUM,$(FLAGS)),CONFIG_REMOTECONFIG_MINIMUM)
	DEFINES+=-DCONFIG_NET_APPS_NO_MDNS
	DEFINES+=-DCONFIG_UDP_NO_OPTIMIZE
	DEFINES+=-DDISABLE_RTC
else
  ifeq ($(findstring NO_EMAC,$(FLAGS)),NO_EMAC)
  else
		ifeq ($(findstring RTL8201F,$(FLAGS)),RTL8201F)
     	DEFINES+=-DRTL8201F_LED1_LINK_ALL
    endif
  	DEFINES+=-DCONFIG_EMAC_HASH_MULTICAST_FILTER
  endif
  ifeq ($(findstring CONFIG_NET_ENABLE_PTP,$(FLAGS)),CONFIG_NET_ENABLE_PTP)
  	DEFINES+=-DHAVE_TIMEOFDAY
  else
  	ifeq ($(findstring CONFIG_TIME_USE_SYSTICK,$(FLAGS)),CONFIG_TIME_USE_SYSTICK)
  		DEFINES+=-DHAVE_TIMEOFDAY
  	else
   		DEFINES+=-DCONFIG_TIME_USE_TIMER
     	DEFINES+=-DHAVE_TIMEOFDAY
    endif
  endif
endif

ifeq ($(findstring CONFIG_FATFS_USE_RAM,$(FLAGS)),CONFIG_FATFS_USE_RAM)
	FATFS_MKFS=1
endif

ifeq ($(findstring CONFIG_FATFS_USE_SPI,$(FLAGS)),CONFIG_FATFS_USE_SPI)
	FATFS_MKFS=1 	
endif

ifdef FATFS_MKFS
	DEFINES+=-DCONFIG_FATFS_MKFS
endif

# Hardware Scenario    Flag Condition              Resulting Compiler Definitions (DEFINES)
# Using RTL8201F       ENET_LINK_CHECK is missing  -DRTL8201F_LED1_LINK_ALL  -DENET_LINK_CHECK_USE_INT (Uses Interrupts)
# Using RTL8201F       ENET_LINK_CHECK is present  -DRTL8201F_LED1_LINK_ALL
# Other Hardware       Any                         -DENET_LINK_CHECK_REG_POLL (Uses Polling)

ifneq ($(findstring RTL8201F,$(FLAGS)),)
	DEFINES+=-DRTL8201F_LED1_LINK_ALL
	ifeq ($(findstring ENET_LINK_CHECK,$(FLAGS)),)
		DEFINES+=-DENET_LINK_CHECK_USE_INT
	endif
else
	DEFINES+=-DENET_LINK_CHECK_REG_POLL
endif

$(info $$DEFINES [${DEFINES}])

DEFINES:= $(sort $(DEFINES))

$(info $$DEFINES [${DEFINES}])

