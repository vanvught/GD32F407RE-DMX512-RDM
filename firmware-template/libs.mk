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

ifeq ($(findstring NODE_OSC_CLIENT,$(DEFINES)),NODE_OSC_CLIENT)
	LIBS+=osc
endif

ifeq ($(findstring NODE_OSC_SERVER,$(DEFINES)),NODE_OSC_SERVER)
	LIBS+=osc
endif

ifeq ($(findstring RDM_CONTROLLER,$(DEFINES)),RDM_CONTROLLER)
	RDM=1
	DMX=1
endif

ifeq ($(findstring NODE_RDMNET_LLRP_ONLY,$(DEFINES)),NODE_RDMNET_LLRP_ONLY)
	RDM=1
	ifneq ($(findstring e131,$(LIBS)),e131)
		LIBS+=e131
	endif
endif

ifdef RDM
	LIBS+=rdm
endif

ifeq ($(findstring OUTPUT_DMX_SEND,$(DEFINES)),OUTPUT_DMX_SEND)
	LIBS+= dmx
endif

ifeq ($(findstring OUTPUT_DMX_PIXEL,$(DEFINES)),OUTPUT_DMX_PIXEL)
	LIBS+=ws28xxdmx ws28xx
endif

LIBS+=configstore network 

ifeq ($(findstring DISPLAY_UDF,$(DEFINES)),DISPLAY_UDF)
	LIBS+=displayudf
endif

LIBS+=lightset flash properties display device hal

$(info $$LIBS [${LIBS}])
