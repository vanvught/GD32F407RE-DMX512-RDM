INCLUDES:= -I./include -I../include -I../lib-hal/include -I../lib-debug/include 
INCLUDES+=$(addprefix -I,$(EXTRA_INCLUDES))
INCLUDES+=-I../firmware-template-gd32/include
INCLUDES+=-I../firmware-template-gd32/template
INCLUDES+=-I../lib-gd32/${FAMILY}/${FAMILY_UC}_standard_peripheral/Include
INCLUDES+=-I../lib-gd32/${FAMILY}/CMSIS
INCLUDES+=-I../lib-gd32/${FAMILY}/CMSIS/GD/${FAMILY_UC}/Include
INCLUDES+=-I../lib-gd32/include