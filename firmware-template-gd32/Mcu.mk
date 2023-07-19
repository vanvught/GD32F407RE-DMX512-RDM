ifndef LINKER
	$(error LINKER is not set)
endif

MCU=$(shell echo $(LINKER) | rev | cut -f1  -d '/'  | rev | cut -f1  -d '_' | tr a-w A-W)
MCU_LC=$(shell echo $(MCU) | tr A-Z a-z | rev | cut -c3- | rev )

$(info $$MCU [${MCU}])
$(info $$MCU_LC [${MCU_LC}])