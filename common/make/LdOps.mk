LDLIBS := -Wl,--start-group $(LDLIBS) -Wl,--end-group

LDOPS = $(COPS) \
        -Wno-error=uninitialized \
        -Wl,--gc-sections \
        -Wl,--print-gc-sections \
        -Wl,--print-memory-usage \
        -Wl,--whole-archive \
        ../lib-clib/lib_gd32/libclib.a \
        -Wl,--no-whole-archive \
        -Wl,-Map=$(MAP)