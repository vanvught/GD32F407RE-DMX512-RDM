$(info "mbedtls.mk")

ifeq ($(findstring CONFIG_USE_MBEDTLS,$(DEFINES)), CONFIG_USE_MBEDTLS)
	USEMBEDTLS=1
endif
ifeq ($(findstring CONFIG_USE_MBEDTLS,$(MAKE_FLAGS)), CONFIG_USE_MBEDTLS)
	USEMBEDTLSS=1
endif

MBEDTLSS_ROOT=../mbedtls

ifdef USEMBEDTLS
  INCLUDES+=-I$(MBEDTLSS_ROOT)/mbedtls/include 
endif

$(info $$INCLUDES [${INCLUDES}])