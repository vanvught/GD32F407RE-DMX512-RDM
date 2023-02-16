/*
 * stack_debug.c
 */


#include <stdint.h>

extern unsigned char _heap_end;
extern unsigned char _sp;

void stack_debug_init() {
	uint32_t *start = (uint32_t *) &_heap_end;
	uint32_t *end = (uint32_t *) &_sp;

	while (start < end) {
		*start = 0xABCDABCD;
		start++;
	}
}

#include <stdio.h>

void stack_debug_print(uint32_t line) {
	uint32_t *start = (uint32_t *) &_heap_end;
	uint32_t *end = (uint32_t *) &_sp;
	uint32_t size = end - start;

	uint32_t *p = (uint32_t *) &_heap_end;

	while (p < end) {
		if (*p != 0xABCDABCD) {
			break;
		}
		p++;
	}

	printf("%u: stack %p:%p:%p [%u:%u]\n", line, start, p, end, 4 * (p - start), ((p - start) * 100) / size);
}
