$(info $$DEFINES [${DEFINES}])

ifeq ($(findstring NO_EMAC,$(DEFINES)),NO_EMAC)
else
	LIBS+=remoteconfig
endif

ifeq ($(findstring NODE_ARTNET,$(DEFINES)),NODE_ARTNET)
	ifeq ($(findstring ARTNET_VERSION=3,$(DEFINES)),ARTNET_VERSION=3)
		LIBS+=artnet
	else
		LIBS+=artnet e131
	endif
endif

ifeq ($(findstring NODE_E131,$(DEFINES)),NODE_E131)
	ifneq ($(findstring e131,$(LIBS)),e131)
		LIBS+=e131
	endif
endif

ifeq ($(findstring NODE_DDP_DISPLAY,$(DEFINES)),NODE_DDP_DISPLAY)
	LIBS+=ddp
endif

ifeq ($(findstring NODE_PP,$(DEFINES)),NODE_PP)
	LIBS+=pp
endif

DMX=

ifeq ($(findstring RDM_CONTROLLER,$(DEFINES)),RDM_CONTROLLER)
	LIBS+=rdm
	DMX=1
endif

ifeq ($(findstring NODE_RDMNET_LLRP_ONLY,$(DEFINES)),NODE_RDMNET_LLRP_ONLY)
	ifneq ($(findstring rdmnet,$(LIBS)),rdmnet)
		LIBS+=rdmnet
	endif
	ifneq ($(findstring e131,$(LIBS)),e131)
		LIBS+=e131
	endif
	ifneq ($(findstring RDM_CONTROLLER,$(DEFINES)),RDM_CONTROLLER)
		LIBS+=rdm
	endif
endif

ifdef RDM
	LIBS+=rdm
endif

ifeq ($(findstring e131,$(LIBS)),e131)
	LIBS+=uuid
endif

ifeq ($(findstring OUTPUT_DMX_MONITOR,$(DEFINES)),OUTPUT_DMX_MONITOR)
	LIBS+=dmxmonitor	
endif

ifeq ($(findstring OUTPUT_DMX_SEND,$(DEFINES)),OUTPUT_DMX_SEND)
	LIBS+=dmxsend dmx
endif

ifeq ($(findstring OUTPUT_DDP_PIXEL_MULTI,$(DEFINES)),OUTPUT_DDP_PIXEL_MULTI)
	LIBS+=ws28xxdmx ws28xx
else
	ifeq ($(findstring OUTPUT_DMX_PIXEL_MULTI,$(DEFINES)),OUTPUT_DMX_PIXEL_MULTI)
		LIBS+=ws28xxdmx ws28xx
	else
		ifeq ($(findstring OUTPUT_DMX_PIXEL,$(DEFINES)),OUTPUT_DMX_PIXEL)
			LIBS+=ws28xxdmx ws28xx
		endif
	endif
endif

ifeq ($(findstring OUTPUT_DDP_PIXEL,$(DEFINES)),OUTPUT_DDP_PIXEL)
	LIBS+=ws28xx
endif

LIBS+=configstore network lightset 

ifeq ($(findstring DISPLAY_UDF,$(DEFINES)),DISPLAY_UDF)
	LIBS+=displayudf
endif

LIBS+=flash properties display device hal

$(info $$LIBS [${LIBS}])