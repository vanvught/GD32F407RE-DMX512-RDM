$(info $$MAKE_FLAGS [${MAKE_FLAGS}])
$(info $$DEFINES [${DEFINES}])

COND=

ifneq ($(MAKE_FLAGS),)
	ifeq ($(findstring ESP8266,$(MAKE_FLAGS)), ESP8266)
		EXTRA_SRCDIR+=src/esp8266 src/params
		COND=1
	endif
	ifndef COND
		ifeq ($(findstring NO_EMAC,$(MAKE_FLAGS)), NO_EMAC)
			EXTRA_SRCDIR+=src/noemac
			COND=1
		endif
	endif
	ifndef COND
		EXTRA_SRCDIR+=src/net src/emac src/params
		EXTRA_SRCDIR+=src/apps/mdns src/apps/ntp src/apps/tftp
	endif
else
	EXTRA_SRCDIR+=src/net src/emac
	EXTRA_SRCDIR+=src/apps/mdns src/apps/ntp src/apps/tftp
endif
