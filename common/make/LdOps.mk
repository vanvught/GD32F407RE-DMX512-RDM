LDLIBS:=-Wl,--start-group $(LDLIBS) -lc -Wl,--end-group
LDOPS = $(COPS)\
        -Wno-error=uninitialized \
        -Wl,--gc-sections \
        -Wl,--print-gc-sections \
        -Wl,--print-memory-usage \
        -Wl,-u,memcpy \
		    -Wl,-u,memset \
        -Wl,-Map=$(MAP),--cref